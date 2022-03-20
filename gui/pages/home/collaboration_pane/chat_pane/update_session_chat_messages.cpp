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
#include <liblec/lecui/widgets/label.h>

// STL
#include <sstream>
#include <sstream>
#include <iomanip>

void main_form::update_session_chat_messages() {
	if (_current_session_unique_id.empty()) {
		_previous_messages.clear();
		return;	// exit immediately, user isn't currently part of any session
	}

	// stop the timer
	_timer_man.stop("update_session_chat_messages");

	std::vector<collab::message> messages;

	std::string error;
	if (_collab.get_messages(_current_session_unique_id, messages, error)) {
		// check if anything has changed
		if (messages != _previous_messages) {
			_previous_messages = messages;
			log("Session " + shorten_unique_id(_current_session_unique_id) + ": messages changed");

			try {
				auto& messages_pane = get_pane("home/collaboration_pane/chat_pane/messages");

				const auto ref_rect = lecui::rect(messages_pane.size());

				float bottom_margin = 0.f;

				std::string previous_sender_unique_id;

				// K = unique_id, T = display name
				std::map<std::string, std::string> display_names;

				bool latest_message_arrived = false;

				struct day_struct {
					int day = 0;
					int month = 0;
					int year = 0;

					bool operator==(const day_struct& param) {
						return (day == param.day) && (month == param.month) && (year == param.year);
					}

					bool operator!=(const day_struct& param) {
						return !operator==(param);
					}
				} previous_day;

				for (const auto& msg : messages) {
					if (msg.unique_id == _message_sent_just_now)
						latest_message_arrived = true;

					std::tm time = { };
					localtime_s(&time, &msg.time);

					std::stringstream ss;
					ss << std::put_time(&time, "%H:%M");
					std::string send_time = ss.str();

					day_struct this_day;
					this_day.day = time.tm_mday;
					this_day.month = time.tm_mon;
					this_day.year = time.tm_year;

					const bool day_change = this_day != previous_day;
					previous_day = this_day;

					const bool continuation = (previous_sender_unique_id == msg.sender_unique_id);
					const bool own_message = msg.sender_unique_id == _collab.unique_id();

					if (continuation && !day_change)
						bottom_margin -= (.85f * _margin);

					std::string display_name;

					// try to get this user's display name
					try {
						if (display_names.count(msg.sender_unique_id) == 0) {
							// fetch from local database
							std::string _display_name;
							if (_collab.get_user_display_name(msg.sender_unique_id, _display_name, error) && !_display_name.empty()) {
								// capture
								display_name = _display_name;

								// cache
								display_names[msg.sender_unique_id] = display_name;
							}
							else {
								// use shortened version of user's unique id
								display_name = shorten_unique_id(msg.sender_unique_id);
							}
						}
						else {
							// use the cached display name
							display_name = display_names.at(msg.sender_unique_id);
						}
					}
					catch (const std::exception&) {
						// use shortened version of user's unique id
						display_name = shorten_unique_id(msg.sender_unique_id);
					}

					float font_size = _ui_font_size;

					const float content_margin = 10.f;

					// measure text height
					const float text_height = _dim.measure_label(msg.text, _font, font_size,
						lecui::text_alignment::left, lecui::paragraph_alignment::top,
						lecui::rect(ref_rect).width(ref_rect.width() - (2.f * content_margin))).height() +
						1.f;	// failsafe

					const float pane_height = (own_message || continuation) ?
						text_height + (2 * content_margin) :
						_caption_height + text_height + (2 * content_margin);

					if (day_change) {
						std::stringstream ss;
						ss << std::put_time(&time, "%B %d, %Y");
						std::string day_string = ss.str();

						// text size
						auto text_width = _dim.measure_label(day_string, _font, _caption_font_size,
							lecui::text_alignment::center, lecui::paragraph_alignment::top, ref_rect).width();

						auto& day_label_background = lecui::widgets::rectangle::add(messages_pane, day_string + "_rect");
						day_label_background
							.rect(lecui::rect(messages_pane.size())
								.top(bottom_margin + _margin / 2.f)
								.height(_caption_height + _margin / 2.f)
								.width(text_width + 2.f * _margin))
							.corner_radius_x(day_label_background.rect().height() / 2.f)
							.corner_radius_y(day_label_background.corner_radius_x())
							.color_fill(lecui::defaults::color(_setting_darktheme ? lecui::themes::dark : lecui::themes::light, lecui::element::text_field));

						auto& day_label = lecui::widgets::label::add(messages_pane, day_string);
						day_label
							.rect(lecui::rect(messages_pane.size())
								.top(bottom_margin + _margin / 2.f)
								.height(_caption_height))
							.alignment(lecui::text_alignment::center)
							.text(day_string)
							.font_size(_caption_font_size);

						day_label_background.rect().place(day_label.rect(), 50.f, 50.f);

						bottom_margin = day_label.rect().bottom() + _margin + _margin / 2.f;
					}

					// add message pane
					auto& pane = lecui::containers::pane::add(messages_pane, msg.unique_id, content_margin);
					pane
						.rect(lecui::rect(messages_pane.size())
							.top(bottom_margin)
							.height(pane_height))
						.color_fill(_setting_darktheme ?
							(own_message ?
								lecui::color().red(30).green(60).blue(100) :
								lecui::color().red(35).green(45).blue(60)) :
							(own_message ?
								lecui::color().red(245).green(255).blue(245) :
								lecui::color().red(255).green(255).blue(255)));
					pane
						.badge()
						.text(send_time)
						.color(lecui::color().alpha(0))
						.color_border(lecui::color().alpha(0))
						.color_text(_caption_color);

					// update bottom margin
					bottom_margin = pane.rect().bottom() + _margin;

					if (!(own_message || continuation)) {
						// add message label
						auto& label = lecui::widgets::label::add(pane, msg.unique_id + "_label");
						label
							.text("<strong>" + display_name + "</strong>")
							.rect(lecui::rect(pane.size()).height(_caption_height))
							.font_size(_caption_font_size);

						// add message text
						auto& text = lecui::widgets::label::add(pane, msg.unique_id + "_text");
						text
							.text(msg.text)
							.rect(lecui::rect(label.rect()).height(text_height).snap_to(label.rect(), snap_type::bottom, 0.f))
							.font_size(font_size);
					}
					else {
						// add message text
						auto& text = lecui::widgets::label::add(pane, msg.unique_id + "_text");
						text
							.text(msg.text)
							.rect(lecui::rect(pane.size()).height(text_height))
							.font_size(font_size);
					}

					previous_sender_unique_id = msg.sender_unique_id;
				}

				if (latest_message_arrived) {
					// clear
					_message_sent_just_now.clear();

					// scroll to the bottom
					messages_pane.scroll_vertically(-std::numeric_limits<float>::max());

					update();
				}
			}
			catch (const std::exception&) {}
		}
	}

	// resume the timer (1200ms looping ...)
	_timer_man.add("update_session_chat_messages", 1200, [&]() {
		update_session_chat_messages();
		});
}
