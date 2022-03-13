/*
** MIT License
**
** Copyright(c) 2021 Alec Musasa
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this softwareand associated documentation files(the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions :
**
** The above copyright noticeand this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/

#include "../../../../../gui.h"

// lecui
#include <liblec/lecui/widgets/rectangle.h>
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/html_editor.h>
#include <liblec/lecui/widgets/button.h>
#include <liblec/lecui/menus/context_menu.h>
#include <liblec/lecui/utilities/filesystem.h>

// leccore
#include <liblec/leccore/system.h>
#include <liblec/leccore/hash.h>

// STL
#include <sstream>
#include <iomanip>
#include <chrono>

void main_form::update_session_chat_files() {
	if (_current_session_unique_id.empty()) {
		_previous_files.clear();
		return;	// exit immediately, user isn't currently part of any session
	}

	// stop the timer
	_timer_man.stop("update_session_chat_files");

	std::vector<collab::file> files;
	std::string error;

	if (_collab.get_files(_current_session_unique_id, files, error)) {

		// check if anything has changed
		if (files != _previous_files) {
			_previous_files = files;
			log("Session " + shorten_unique_id(_current_session_unique_id) + ": files changed");

			for (const auto& it : files)
				_session_files[it.hash] = it;

			try {
				auto& content_pane = get_pane("home/collaboration_pane/files_pane/content");

				const auto ref_rect = lecui::rect(content_pane.size());

				float bottom_margin = 0.f;

				// K = unique_id, T = display name
				std::map<std::string, std::string> display_names;

				for (const auto& it : files) {
					auto& file = _session_files.at(it.hash);

					std::tm time = { };
					localtime_s(&time, &file.time);

					std::stringstream ss;
					ss << std::put_time(&time, "%d %B %Y, %H:%M");
					std::string send_date = ss.str();

					std::string display_name;

					// try to get this user's display name
					try {
						if (display_names.count(file.sender_unique_id) == 0) {
							// fetch from local database
							std::string _display_name;
							if (_collab.get_user_display_name(file.sender_unique_id, _display_name, error) && !_display_name.empty()) {
								// capture
								display_name = _display_name;

								// cache
								display_names[file.sender_unique_id] = display_name;
							}
							else {
								// use the shortened version of the user's unique id
								display_name = shorten_unique_id(file.sender_unique_id);
							}
						}
						else {
							// use the cached display name
							display_name = display_names.at(file.sender_unique_id);
						}
					}
					catch (const std::exception&) {
						// use the shortened version of the user's unique id
						display_name = shorten_unique_id(file.sender_unique_id);
					}

					const std::string file_name_text = "<strong>" + file.name + "</strong><span style = 'font-size: 8.0pt;'>" + file.extension + "</span>";

					const float content_margin = 10.f;

					const float description_height = _dim.measure_label(file.description, _font, _caption_font_size,
						lecui::text_alignment::left, lecui::paragraph_alignment::top,
						lecui::rect(ref_rect).width(ref_rect.width() - (2.f * content_margin))).height() +
						1.f;					// failsafe;

					const float file_name_height = _dim.measure_label(file_name_text, _font, _ui_font_size,
						lecui::text_alignment::left, lecui::paragraph_alignment::top,
						lecui::rect(ref_rect).width(ref_rect.width() - (2.f * content_margin) - 40.f - _margin)).height() +
						1.f;					// failsafe;

					const float file_pane_height =
						content_margin +		// top margin
						file_name_height +		// filename
						_caption_height +		// additional
						_caption_height +		// shared on
						(_margin / 2.f) +		// margin between shared on and description
						description_height +	// description
						content_margin;			// bottom margin

					auto& file_pane = lecui::containers::pane::add(content_pane, file.hash, content_margin);
					file_pane
						.rect(lecui::rect(ref_rect)
							.top(bottom_margin)
							.height(file_pane_height))
						.color_fill(_setting_darktheme ?
							lecui::color().red(35).green(45).blue(60) :
							lecui::color().red(255).green(255).blue(255));

					// update bottom margin
					bottom_margin = file_pane.rect().bottom() + _margin;

					// add rectangle for hit testing
					auto& hit_testing = lecui::widgets::rectangle::add(file_pane, file.hash + "_hit_testing");
					hit_testing
						.rect(lecui::rect(file_pane.size()))
						.corner_radius_x(5.f)
						.corner_radius_y(5.f)
						.color_fill(lecui::color().alpha(0))
						.color_border(lecui::color().alpha(0))
						.color_hot(lecui::defaults::color(_setting_darktheme ?
							lecui::themes::dark : lecui::themes::light, lecui::item::icon_hot).alpha(20));
					hit_testing
						.events().action = [&]() {
						std::string error;

						try {
							// close review info pane (it'll be recreated below)
							_page_man.close("home/collaboration_pane/files_pane/review_info");

							_previous_reviews.clear();	// to force a refresh in the case of an already selected file being selected afresh
							_current_session_file_hash = file.hash;

							auto& files_pane = get_pane("home/collaboration_pane/files_pane");
							auto& content_pane = get_pane("home/collaboration_pane/files_pane/content");

							// close review input pane
							_page_man.close("home/collaboration_pane/files_pane/review_input");

							std::tm time = { };
							localtime_s(&time, &file.time);

							std::stringstream ss;
							ss << std::put_time(&time, "%d %B %Y, %H:%M");
							std::string send_date = ss.str();

							std::string display_name;

							// try to get this user's display name
							try {
								// fetch from local database
								std::string _display_name;
								if (_collab.get_user_display_name(file.sender_unique_id, _display_name, error) && !_display_name.empty()) {
									// capture
									display_name = _display_name;
								}
								else {
									// use shortened version of user's unique id
									display_name = shorten_unique_id(file.sender_unique_id);
								}
							}
							catch (const std::exception&) {
								// use shortened version of user's unique id
								display_name = shorten_unique_id(file.sender_unique_id);
							}

							// create review info pane
							auto& review_info = lecui::containers::pane::add(files_pane, "review_info", 0.f);
							review_info
								.rect(lecui::rect(files_pane.size())
									.top(content_pane.rect().top())
									.left(content_pane.rect().right())
									.right(files_pane.size().get_width()))
								.on_resize(lecui::resize_params()
									.width_rate(100.f)
									.height_rate(100.f));

							// make pane invisible
							review_info
								.border(0.f);
							review_info
								.color_fill().alpha(0);

							auto ref_rect = lecui::rect(review_info.size());
							ref_rect.left() += _margin;
							ref_rect.right() -= _margin;
							ref_rect.top() += _margin;
							ref_rect.bottom() -= _margin;

							auto& title = lecui::widgets::label::add(review_info, "title");
							title
								.text("<strong>FILE REVIEWS</strong>")
								.rect(lecui::rect()
									.left(_margin)
									.width(ref_rect.width())
									.top(_margin)
									.height(title.rect().height()))
								.on_resize(lecui::resize_params().width_rate(100.f));

							// add file image
							auto& file_image = lecui::widgets::image_view::add(review_info, "file_image");
							file_image
								.rect(lecui::rect()
									.left(_margin)
									.top(title.rect().bottom() + 2.f * _margin)
									.width(40.f)
									.height(40.f))
								.png_resource(map_extension_to_resource(file.extension));

							auto& file_name = lecui::widgets::label::add(review_info, "file_name");
							file_name
								.text("<strong>" + file.name + "</strong><span style = 'font-size: 8.0pt;'>" + file.extension + "</span>")
								.rect(lecui::rect(file_image.rect())
									.top(file_image.rect().top())
									.height(file_name.rect().height())
									.left(file_image.rect().right() + _margin)
									.right(ref_rect.right()))
								.on_resize(lecui::resize_params().width_rate(100.f));

							auto& additional = lecui::widgets::label::add(review_info, "additional");
							additional
								.text("<strong>" + leccore::format_size(file.size, 2) + "</strong>, shared by <em>" + display_name + "</em>")
								.font_size(_caption_font_size)
								.color_text(_caption_color)
								.rect(lecui::rect(file_name.rect())
									.height(_caption_height)
									.snap_to(file_name.rect(), snap_type::bottom, 0.f))
								.on_resize(lecui::resize_params().width_rate(100.f));

							auto& shared_on = lecui::widgets::label::add(review_info, "shared_on");
							shared_on
								.text("Shared on " + send_date)
								.font_size(_caption_font_size)
								.color_text(_caption_color)
								.rect(lecui::rect(file_name.rect())
									.height(_caption_height)
									.snap_to(additional.rect(), snap_type::bottom, 0.f))
								.on_resize(lecui::resize_params().width_rate(100.f));

							auto& file_description = lecui::widgets::label::add(review_info, "file_description");
							file_description
								.text(file.description)
								.font_size(_caption_font_size)
								.rect(lecui::rect()
									.height(_caption_height)
									.snap_to(shared_on.rect(), snap_type::bottom, _margin / 2.f)
									.left(file_image.rect().left())
									.right(shared_on.rect().right()))
								.on_resize(lecui::resize_params().width_rate(100.f));

							// add review list pane
							auto& list = lecui::containers::pane::add(review_info, "list");
							list
								.rect(lecui::rect(review_info.size())
									.top(file_description.rect().bottom() + _margin)
									.bottom(review_info.size().get_height()))
								.on_resize(lecui::resize_params()
									.width_rate(100.f)
									.height_rate(100.f));

							// make pane invisible
							list
								.border(0.f);
							list
								.color_fill().alpha(0);
						}
						catch (const std::exception& e) {
							message(e.what());
						}
					};

					hit_testing
						.events().right_click = [&]() {
						std::string error;

						lecui::context_menu::specs menu_specs;
						menu_specs.items.push_back({ "Open", png_open_file });
						menu_specs.items.push_back({ "Save To ...", png_save_as });
						menu_specs.items.push_back({ "Review", png_review });

						auto selected = lecui::context_menu::context_menu()(*this, menu_specs);

						if (selected == "Open") {
							const auto destination_file = _files_staging_folder + "\\" + file.name + file.extension;

							// remove destination file if it already exists
							if (!leccore::file::remove(destination_file, error)) {}

							// extract the file
							if (!leccore::file::copy(_files_folder + "\\" + file.hash, destination_file, error))
								message("Error extracting file: " + error);
							else {
								if (!leccore::shell::open(destination_file, error))
									message("Error opening file: " + error);
							}
						}

						if (selected == "Save To ...") {
							// prompt user for folder to save
							lecui::filesystem fs(*this);
							const auto folder = fs.select_folder("Select Folder");

							if (!folder.empty()) {
								const auto destination_file = folder + "\\" + file.name + file.extension;

								// remove destination file if it already exists
								if (!leccore::file::remove(destination_file, error)) {}

								// extract the file
								if (!leccore::file::copy(_files_folder + "\\" + file.hash, destination_file, error))
									message("Error extracting file: " + error);
								else {
									if (!leccore::shell::view(destination_file, error))
										message("Error opening folder: " + error);
								}
							}
						}

						if (selected == "Review") {
							try {
								_current_session_file_hash.clear();

								auto& files_pane = get_pane("home/collaboration_pane/files_pane");
								auto& content_pane = get_pane("home/collaboration_pane/files_pane/content");

								// close review info pane
								_page_man.close("home/collaboration_pane/files_pane/review_info");

								std::tm time = { };
								localtime_s(&time, &file.time);

								std::stringstream ss;
								ss << std::put_time(&time, "%d %B %Y, %H:%M");
								std::string send_date = ss.str();

								std::string display_name;

								// try to get this user's display name
								try {
									// fetch from local database
									std::string _display_name;
									if (_collab.get_user_display_name(file.sender_unique_id, _display_name, error) && !_display_name.empty()) {
										// capture
										display_name = _display_name;
									}
									else {
										// use shortened version of user's unique id
										display_name = shorten_unique_id(file.sender_unique_id);
									}
								}
								catch (const std::exception&) {
									// use shortened version of user's unique id
									display_name = shorten_unique_id(file.sender_unique_id);
								}

								// create review input pane
								auto& review_input = lecui::containers::pane::add(files_pane, "review_input", 0.f);
								review_input
									.rect(lecui::rect(files_pane.size())
										.top(content_pane.rect().top())
										.left(content_pane.rect().right())
										.right(files_pane.size().get_width()))
									.on_resize(lecui::resize_params()
										.width_rate(100.f)
										.height_rate(100.f));

								// make pane invisible
								review_input
									.border(0.f);
								review_input
									.color_fill().alpha(0);

								auto ref_rect = lecui::rect(review_input.size());
								ref_rect.left() += _margin;
								ref_rect.right() -= _margin;
								ref_rect.top() += _margin;
								ref_rect.bottom() -= _margin;

								auto& title = lecui::widgets::label::add(review_input, "title");
								title
									.text("<strong>NEW FILE REVIEW</strong>")
									.rect(lecui::rect()
										.left(_margin)
										.width(ref_rect.width())
										.top(_margin)
										.height(title.rect().height()))
									.on_resize(lecui::resize_params().width_rate(100.f));

								// add file image
								auto& file_image = lecui::widgets::image_view::add(review_input, "file_image");
								file_image
									.rect(lecui::rect()
										.left(_margin)
										.top(title.rect().bottom() + 2.f * _margin)
										.width(40.f)
										.height(40.f))
									.png_resource(map_extension_to_resource(file.extension));

								auto& file_name = lecui::widgets::label::add(review_input, "file_name");
								file_name
									.text("<strong>" + file.name + "</strong><span style = 'font-size: 8.0pt;'>" + file.extension + "</span>")
									.rect(lecui::rect(file_image.rect())
										.top(file_image.rect().top())
										.height(file_name.rect().height())
										.left(file_image.rect().right() + _margin)
										.right(ref_rect.right()))
									.on_resize(lecui::resize_params().width_rate(100.f));

								auto& additional = lecui::widgets::label::add(review_input, "additional");
								additional
									.text("<strong>" + leccore::format_size(file.size, 2) + "</strong>, shared by <em>" + display_name + "</em>")
									.font_size(_caption_font_size)
									.color_text(_caption_color)
									.rect(lecui::rect(file_name.rect())
										.height(_caption_height)
										.snap_to(file_name.rect(), snap_type::bottom, 0.f))
									.on_resize(lecui::resize_params().width_rate(100.f));

								auto& shared_on = lecui::widgets::label::add(review_input, "shared_on");
								shared_on
									.text("Shared on " + send_date)
									.font_size(_caption_font_size)
									.color_text(_caption_color)
									.rect(lecui::rect(file_name.rect())
										.height(_caption_height)
										.snap_to(additional.rect(), snap_type::bottom, 0.f))
									.on_resize(lecui::resize_params().width_rate(100.f));

								auto& file_description = lecui::widgets::label::add(review_input, "file_description");
								file_description
									.text(file.description)
									.font_size(_caption_font_size)
									.rect(lecui::rect()
										.height(_caption_height)
										.snap_to(shared_on.rect(), snap_type::bottom, _margin / 2.f)
										.left(file_image.rect().left())
										.right(shared_on.rect().right()))
									.on_resize(lecui::resize_params().width_rate(100.f));

								// add input html editor
								auto& input = lecui::widgets::html_editor::add(review_input, "input");
								input
									.font(_font)
									.font_size(_review_font_size)
									.alignment(lecui::text_alignment::justified)
									.rect(lecui::rect()
										.left(_margin)
										.width(ref_rect.width())
										.top(file_description.rect().bottom() + _margin)
										.bottom(ref_rect.bottom() - 25.f - _margin))
									.on_resize(lecui::resize_params()
										.width_rate(100.f)
										.height_rate(100.f));

								auto do_add_review = [&]() {
									std::string error;

									try {
										auto& input = get_html_editor("home/collaboration_pane/files_pane/review_input/input");

										const auto& text = input.text();

										if (text.empty())
											return;

										// make review object
										collab::review review;

										// make review unique id
										review.unique_id = leccore::hash_string::uuid();

										// capture review time
										review.time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

										// capture session unique id
										review.session_id = _current_session_unique_id;

										// capture the file hash
										review.file_hash = file.hash;

										// capture the sender's unique id
										review.sender_unique_id = _collab.unique_id();

										// capture the review text
										review.text = text;

										if (_collab.create_review(review, error)) {
											// review created successfully

											// close review input pane
											_page_man.close("home/collaboration_pane/files_pane/review_input");
										}
									}
									catch (const std::exception& e) {
										message(e.what());
									}
								};

								// add button
								auto& add_review = lecui::widgets::button::add(review_input, "add_review");
								add_review
									.text("Add review")
									.rect(lecui::rect(add_review.rect())
										.width(input.rect().width())
										.height(25.f)
										.snap_to(input.rect(), snap_type::bottom, _margin))
									.on_resize(lecui::resize_params()
										.x_rate(50.f)
										.y_rate(100.f))
									.events().action = [do_add_review]() {
									do_add_review();
								};
							}
							catch (const std::exception& e) {
								message(e.what());
							}
						}
					};

					auto& file_image = lecui::widgets::image_view::add(file_pane, file.hash + "_file_image");
					file_image
						.rect(lecui::rect()
							.width(40.f)
							.height(40.f))
						.png_resource(map_extension_to_resource(file.extension));

					auto& file_name = lecui::widgets::label::add(file_pane, file.hash + "_file_name");
					file_name
						.text(file_name_text)
						.font_size(_ui_font_size)
						.rect(file_name.rect()
							.left(file_image.rect().right() + _margin)
							.right(file_pane.size().get_width())
							.height(file_name_height));

					auto& additional = lecui::widgets::label::add(file_pane, file.hash + "_additional");
					additional
						.text("<strong>" + leccore::format_size(file.size, 2) + "</strong>, shared by <em>" + display_name + "</em>")
						.font_size(_caption_font_size)
						.color_text(_caption_color)
						.rect(lecui::rect(file_name.rect())
							.height(_caption_height)
							.snap_to(file_name.rect(), snap_type::bottom, 0.f));

					auto& shared_on = lecui::widgets::label::add(file_pane, file.hash + "_shared_on");
					shared_on
						.text("Shared on " + send_date)
						.font_size(_caption_font_size)
						.color_text(_caption_color)
						.rect(lecui::rect(file_name.rect())
							.height(_caption_height)
							.snap_to(additional.rect(), snap_type::bottom, 0.f));

					auto& file_description = lecui::widgets::label::add(file_pane, file.hash + "_file_description");
					file_description
						.text(file.description)
						.font_size(_caption_font_size)
						.alignment(lecui::text_alignment::left)
						.rect(lecui::rect()
							.height(description_height)
							.snap_to(shared_on.rect(), snap_type::bottom, _margin / 2.f)
							.left(file_image.rect().left())
							.right(shared_on.rect().right()));
				}
			}
			catch (const std::exception&) {}
		}
	}

	// resume the timer (1200ms looping ...)
	_timer_man.add("update_session_chat_files", 1200, [&]() {
		update_session_chat_files();
		});
}
