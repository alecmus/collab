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
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/html_view.h>

// STL
#include <sstream>
#include <iomanip>

void main_form::update_file_reviews() {
	if (_current_session_unique_id.empty() || _current_session_file_hash.empty()) {
		_previous_reviews.clear();
		return;	// exit immediately, user isn't currently part of any session or there is no current session file
	}

	// stop the timer
	_timer_man.stop("update_file_reviews");

	std::vector<collab::review> reviews;
	std::string error;
	int panes_not_rendered = 0;
	std::vector<std::string> pane_list;

	if (_collab.get_reviews(_current_session_unique_id, _current_session_file_hash, reviews, error)) {
		// check if anything has changed
		if (reviews != _previous_reviews) {
			_previous_reviews = reviews;
			log("Session " + shorten_unique_id(_current_session_unique_id) + ": reviews changed");

			try {
				auto& list = get_pane("home/collaboration_pane/files_pane/review_info/list");

				const auto ref_rect = lecui::rect(list.size());

				float bottom = 0.f;

				// K = unique_id, T = display name
				struct user_info {
					std::string display_name;
					std::string user_image_file;
				};

				std::map<std::string, user_info> cached_user_info;

				for (const auto& review : reviews) {
					std::tm time = { };
					localtime_s(&time, &review.time);

					std::stringstream ss;
					ss << std::put_time(&time, "%d %B %Y, %H:%M");
					std::string send_date = ss.str();

					user_info local_user_info;

					// try to get this user's display name
					try {
						if (cached_user_info.count(review.sender_unique_id) == 0) {
							// fetch from local database
							collab::user user;
							if (_collab.get_user(review.sender_unique_id, user, error) && !user.display_name.empty()) {
								if (!user.user_image.empty()) {
									local_user_info.user_image_file = _files_staging_folder + "\\" + user.unique_id + ".jpg";

									// save to file
									if (leccore::file::write(local_user_info.user_image_file, user.user_image, error)) {}
								}

								// capture
								local_user_info.display_name = user.display_name;

								// cache
								cached_user_info[review.sender_unique_id] = local_user_info;
							}
							else {
								// use the shortened version of the user's unique id
								local_user_info.display_name = shorten_unique_id(review.sender_unique_id);
							}
						}
						else {
							// use the cached display name
							local_user_info = cached_user_info.at(review.sender_unique_id);
						}
					}
					catch (const std::exception&) {
						// use the shortened version of the user's unique id
						local_user_info.display_name = shorten_unique_id(review.sender_unique_id);
					}

					// add review
					auto& pane = lecui::containers::pane::add(list, review.unique_id, 0.f);
					pane
						.rect(lecui::rect(list.size()).top(bottom).height(350.f))
						.on_resize(lecui::resize_params().width_rate(100.f))
						.color_fill(_setting_darktheme ?
							lecui::color().red(35).green(45).blue(60) :
							lecui::color().red(255).green(255).blue(255));

					// add pane list
					pane_list.push_back("home/collaboration_pane/files_pane/review_info/list/" + review.unique_id);

					const float content_margin = 10.f;

					auto ref_rect = lecui::rect(pane.rect().size());
					ref_rect.left() += content_margin;
					ref_rect.top() += content_margin;
					ref_rect.right() -= content_margin;
					ref_rect.bottom() -= content_margin;

					// add user profile image
					auto& user_image = lecui::widgets::image_view::add(pane, review.unique_id + "_user_image");
					user_image
						.rect(lecui::rect(ref_rect)
							.width(40.f)
							.height(40.f))
						.png_resource(local_user_info.user_image_file.empty() ? png_user : 0)
						.file(local_user_info.user_image_file)
						.corner_radius_x(user_image.rect().width() / 2.f)
						.corner_radius_y(user_image.rect().width() / 2.f);

					auto& title = lecui::widgets::label::add(pane, review.unique_id + "_title");
					title
						.text("Reviewed by")
						.font_size(_caption_font_size)
						.color_text(_caption_color)
						.rect(lecui::rect(ref_rect)
							.left(user_image.rect().right() + _margin)
							.height(_caption_height))
						.on_resize(lecui::resize_params().width_rate(100.f));

					auto& user_name = lecui::widgets::label::add(pane, review.unique_id + "_user_name");
					user_name
						.text("<strong>" + local_user_info.display_name + "</strong>")
						.font_size(_ui_font_size)
						.rect(lecui::rect(ref_rect)
							.left(user_image.rect().right() + _margin)
							.top(title.rect().bottom())
							.height(_ui_font_height))
						.on_resize(lecui::resize_params().width_rate(100.f));

					auto& reviewed_on = lecui::widgets::label::add(pane, review.unique_id + "_reviewed_on");
					reviewed_on
						.text(send_date)
						.font_size(_caption_font_size)
						.color_text(_caption_color)
						.rect(lecui::rect(user_name.rect())
							.snap_to(user_name.rect(), snap_type::bottom, 0.f))
						.on_resize(lecui::resize_params().width_rate(100.f));

					auto& text = lecui::widgets::html_view::add(pane, review.unique_id + "_text");
					text
						.text(review.text)
						.font(_font)
						.font_size(_review_font_size)
						.alignment(lecui::text_alignment::justified)
						.rect(lecui::rect(ref_rect)
							.top(reviewed_on.rect().bottom() + _margin / 2.f)

							// ignore the content margin to the sides and bottom since the html viewer already has a content margin of its own
							.left(0.f)
							.right(pane.size().get_width())
							.bottom(pane.size().get_height()))
						.on_resize(lecui::resize_params().width_rate(100.f));

					// make html viewer invisible
					text
						.border(0.f);
					text
						.color_fill().alpha(0);

					if (!pane.rendered())
						panes_not_rendered++;

					// figure out if pane width has changed from the start
					// note that zero will always be returned if the pane hasn't been rendered yet
					// hence the need to use the 'panes_not_rendered' flag
					const auto change_in_width = pane.change_in_size().get_width();

					// determine optimal height of html viewer
					const float html_text_width = text.rect().width() -
						(2.f * 10.f) +	// less the html view's content margin
						change_in_width;	// this line is fire

					// rect with same width as our html text but whose height has room to breathe
					const auto bounding_rect = lecui::rect().width(html_text_width).height(std::numeric_limits<float>::max());

					auto optimal_rect = _dim.measure_html_widget(text.text(), text.font(), text.font_size(),
						text.alignment(), false, bounding_rect);

					// compute optimal height of entire pane
					const float optimal_pane_height = text.rect().top() + optimal_rect.height();

					// set the height of the pane to exactly that
					pane.rect().height(optimal_pane_height);

					// readjust height of html view to the computed optimal
					text.rect().height(optimal_rect.height());

					// update tracker
					bottom = pane.rect().bottom() + _margin;
				}
			}
			catch (const std::exception&) {
				_previous_reviews.clear();	// exception may be because review_info pane is currently closed ... so we need to keep trying until it's available
			}
		}
	}

	if (panes_not_rendered) {
		// hide all panes in the pane list
		for (const auto& path : pane_list)
			if (!_widget_man.hide(path, error)) {}

		// force reviews to be reloaded
		_previous_reviews.clear();

		// update as soon as possible
		_timer_man.add("update_file_reviews", 0, [&]() {
			update_file_reviews();
			});
	}
	else {
		// show all panes in the pane list
		for (const auto& path : pane_list)
			if (!_widget_man.show(path, error)) {}

		if (pane_list.size())
			update();

		// resume the timer (1200ms looping ...)
		_timer_man.add("update_file_reviews", 1200, [&]() {
			update_file_reviews();
			});
	}
}
