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
#include <liblec/lecui/widgets/icon.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/text_field.h>
#include <liblec/lecui/widgets/button.h>
#include <liblec/lecui/utilities/filesystem.h>

// leccore
#include <liblec/leccore/hash.h>

// STL
#include <filesystem>
#include <thread>

lecui::containers::pane& main_form::add_files_pane(lecui::containers::pane& collaboration_pane, const lecui::rect& ref_rect) {
	// lambda functions
	auto do_add_file = [this]() {
		try {
			class add_file_form : public form {
				lecui::controls _ctrls{ *this };
				lecui::page_manager _page_man{ *this };
				lecui::widget_manager _widget_man{ *this };
				lecui::appearance _apprnc{ *this };
				lecui::dimensions _dim{ *this };

				main_form& _main_form;
				const std::string& _full_path;

				void on_add() {
					try {
						auto& file_name = get_text_field("home/file_name");
						auto& file_description = get_text_field("home/file_description");
						auto& status = get_label("home/status");

						if (file_name.text().empty() || file_description.text().empty())
							return;

						collab::file file;

						// capture send time
						file.time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

						// capture session id
						file.session_id = _main_form._current_session_unique_id;

						// capture file sender id
						file.sender_unique_id = _main_form._collab.unique_id();

						// capture file extension
						file.extension = std::filesystem::path(_full_path).extension().string();

						// make extension lowercase
						for (auto& it : file.extension)
							it = tolower(it);

						// capture user supplied file name
						file.name = file_name.text();

						{
							// check if user supplied file name has an extension
							auto idx = file.name.rfind(".");

							if (idx != std::string::npos) {
								std::string extension = file.name.substr(idx);

								for (auto& it : extension)
									it = tolower(it);

								// check if the user supplied extension is the same as the actual file's type
								if (extension == file.extension) {
									// same extension ... remove from file name for saving to database coz extension will be save separately anyway
									file.name.erase(idx);
								}
							}
						}

						// capture file size
						file.size = std::filesystem::file_size(_full_path);

						// capture file description
						file.description = file_description.text();

						// capture file hash
						leccore::hash_file hash_file;
						hash_file.start(_full_path, { leccore::hash_file::algorithm::sha256 });

						std::string file_size_string;

						try {
							file_size_string = leccore::format_size(std::filesystem::file_size(_full_path));
						}
						catch (const std::exception&) {}

						// prevent quitting
						prevent_quit();

						// disable controls
						std::string error;
						if (!_widget_man.disable("home/file_name", error)) {}
						if (!_widget_man.disable("home/file_description", error)) {}
						if (!_widget_man.disable("home/add", error)) {}

						// set status text to hashing
						status.text("Hashing file, please wait . .");

						update();

						unsigned long long count = 0;

						while (hash_file.hashing()) {
							if (count % 40 == 0) {
								// little bit of lazy animation using dots
								if (status.text().length() >= 65)
									status.text("Hashing file, please wait . .");
								else
									status.text() += " .";

								update();
							}

							count++;

							if (!keep_alive()) {
								allow_quit();
								return;
							}

							std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
						}

						// enable controls
						if (!_widget_man.enable("home/file_name", error)) {}
						if (!_widget_man.enable("home/file_description", error)) {}
						if (!_widget_man.enable("home/add", error)) {}

						// clear status text
						status.text().clear();

						allow_quit();

						update();

						leccore::hash_file::hash_results results;
						if (!hash_file.result(results, error)) {
							message("Error hashing file: " + error);
							return;
						}

						file.hash = results.at(leccore::hash_file::algorithm::sha256);

						// check if file already exists in this session
						if (_main_form._collab.file_exists(file.hash, _main_form._current_session_unique_id)) {
							collab::file existing_file;

							if (_main_form._collab.get_file(file.hash, _main_form._current_session_unique_id, existing_file, error))
								message("This file already exists in this session under the following name:\n"
									"<strong>" + existing_file.name + "</strong>"
									"<span style = 'font-size: 8.0pt;'>" + existing_file.extension + "</span>");
							else
								message("This file already exists in this session");
							
							return;
						}

						// check if file already exists in another session
						if (!_main_form._collab.file_exists(file.hash)) {
							// copy the file to the collab folder
							if (!leccore::file::copy(_full_path, _main_form._files_folder + "\\" + file.hash, error)) {
								message("Error copying file: " + error);
								return;
							}
						}

						// save the file to the database
						if (!_main_form._collab.create_file(file, error)) {
							message("Error saving to database: " + error);
							return;
						}

						// file saved successfully ... close this form
						close();
					}
					catch (const std::exception& e) {
						allow_quit();
						message(e.what());
					}
				}

			public:
				add_file_form(const std::string& caption,
					main_form& main_form, const std::string& full_path) :
					form(caption, main_form),
					_main_form(main_form),
					_full_path(full_path) {
					// initialize event
					events().initialize = [this](std::string& error) {
						// size and stuff
						_ctrls
							.allow_resize(false)
							.allow_minimize(false);

						_apprnc
							.main_icon(ico_resource)
							.mini_icon(ico_resource)
							.caption_icon(get_dpi_scale() < 2.f ? icon_png_32 : icon_png_64)
							.theme(_main_form._setting_darktheme ? lecui::themes::dark : lecui::themes::light);
						_dim.set_size(lecui::size().width(300.f).height(220.f));

						return true;
					};

					// layout event
					events().layout = [this](std::string& error) {
						// add home page
						auto& home = _page_man.add("home");

						auto& ref_rect = lecui::rect()
							.left(_margin)
							.top(_margin)
							.width(home.size().get_width() - 2.f * _margin)
							.height(home.size().get_height() - 2.f * _margin);

						// add file name caption
						auto& file_name_caption = lecui::widgets::label::add(home);
						file_name_caption
							.text("File Name")
							.font_size(_caption_font_size)
							.color_text(_main_form._caption_color)
							.rect(lecui::rect()
								.left(_margin)
								.width(ref_rect.width())
								.height(_main_form._caption_height));

						// add file name text field
						auto& file_name = lecui::widgets::text_field::add(home, "file_name");
						file_name
							.prompt("e.g. 'ImportantFile.pdf'")
							.maximum_length(40)	// to-do: remove magic number
							.rect(lecui::rect(file_name.rect())
								.width(file_name_caption.rect().width())
								.snap_to(file_name_caption.rect(), snap_type::bottom, _margin / 4.f))
							.events().action = [&]() { on_add(); };

						if (!get_filename_from_full_path(_full_path, file_name.text())) {}

						// add file description caption
						auto& file_description_caption = lecui::widgets::label::add(home);
						file_description_caption
							.text("File Description")
							.font_size(_caption_font_size)
							.color_text(_main_form._caption_color)
							.rect(lecui::rect(file_name_caption.rect())
								.snap_to(file_name.rect(), snap_type::bottom, _margin));

						// add file description text field
						auto& file_description = lecui::widgets::text_field::add(home, "file_description");
						file_description
							.prompt("e.g. 'Some important document'")
							.maximum_length(100)	// to-do: remove magic number
							.rect(lecui::rect(file_name.rect())
								.snap_to(file_description_caption.rect(), snap_type::bottom, _margin / 4.f))
							.events().action = [&]() { on_add(); };

						std::string file_size_string;

						try {
							file_size_string = leccore::format_size(std::filesystem::file_size(_full_path));
						}
						catch (const std::exception&) {}

						// add file size
						auto& file_size = lecui::widgets::label::add(home);
						file_size
							.text("Size: " + file_size_string)
							.font_size(_caption_font_size)
							.color_text(_main_form._caption_color)
							.rect(lecui::rect(file_name_caption.rect())
								.snap_to(file_description.rect(), snap_type::bottom, _margin));

						// add status label
						auto& status = lecui::widgets::label::add(home, "status");
						status
							.rect(lecui::rect(file_size.rect())
								.height(status.rect().height())
								.snap_to(file_size.rect(), snap_type::bottom, _margin));

						// add 'add' button
						auto& add = lecui::widgets::button::add(home, "add");
						add
							.text("Add")
							.rect(lecui::rect(add.rect()).snap_to(status.rect(), snap_type::bottom, _margin))
							.events().action = [&]() { on_add(); };

						_page_man.show("home");
						return true;
					};
				}

				~add_file_form() {}
			};

			// open a file
			lecui::open_file_params params;
			params
				.allow_multi_select(false)
				.default_type("")
				.title("Select File to Share")
				.include_all_supported_types(true);

			lecui::filesystem fs(*this);
			auto files = fs.open_file(params);

			for (const auto& full_path : files) {
				add_file_form fm("Add File", *this, full_path);
				std::string error;
				if (!fm.create(error))
					message(error);

				break;	// expecting a single file anyway
			}
		}
		catch (const std::exception&) {}
	};

	// add files pane (dynamic)
	auto& files_pane = lecui::containers::pane::add(collaboration_pane, "files_pane", 0.f);
	files_pane
		.rect(lecui::rect(ref_rect)
			.left(ref_rect.get_left() + _margin)
			.top(ref_rect.get_top() + _margin))
		.on_resize(lecui::resize_params().height_rate(100.f).width_rate(100.f).min_width(files_pane.rect().width()));

	// add files title
	auto& title = lecui::widgets::icon::add(files_pane);
	title
		.rect(title.rect()
			.height(45.f)
			.left(_margin)
			.right(files_pane.size().get_width() - _margin))
		.png_resource(png_files)
		.text("Session Files")
		.description("Collaborate via digital content sharing and reviewing");

	// add content pane
	auto& content_pane = lecui::containers::pane::add(files_pane, "content");
	content_pane
		.rect(lecui::rect(files_pane.size())
			.width(300.f)
			.top(title.rect().bottom() + _margin / 3.f)
			.bottom(files_pane.size().get_height() - 30.f - _margin / 2.f))
		.on_resize(lecui::resize_params()
			.height_rate(100.f));

	// make pane invisible
	content_pane
		.border(0.f);
	content_pane
		.color_fill().alpha(0);

	// add button
	auto& add_file = lecui::widgets::button::add(files_pane, "add_file");
	add_file
		.text("Add file")
		.rect(lecui::rect(add_file.rect())
			.width(content_pane.rect().width() - 2.f * 10.f)
			.height(25.f)
			.snap_to(content_pane.rect(), snap_type::bottom, 0.f))
		.on_resize(lecui::resize_params()
			.y_rate(100.f))
		.events().action = [this, do_add_file]() {
		if (_collab.file_source_running())
			do_add_file();
		else
			message("Error: file source is not running");
	};

	return files_pane;
}
