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
#include <liblec/lecui/widgets/text_field.h>

// leccore
#include <liblec/leccore/hash.h>

// STL
#include <chrono>

lecui::containers::pane& main_form::add_chat_pane(lecui::containers::pane& collaboration_pane, const lecui::rect& ref_rect) {
	// lambda functions
	auto send_message = [this]() {
		// make session message object
		collab::message msg;
		msg.unique_id = leccore::hash_string::uuid();
		msg.time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		msg.session_id = _current_session_unique_id;

		msg.sender_unique_id = _collab.unique_id();

		try {
			auto& message = get_text_field("home/collaboration_pane/chat_pane/message");
			msg.text = message.text();
		}
		catch (const std::exception&) {}

		std::string error;
		if (_collab.create_message(msg, error)) {
			try {
				_message_sent_just_now = msg.unique_id;
				auto& message = get_text_field("home/collaboration_pane/chat_pane/message");
				message.text().clear();
				update();
			}
			catch (const std::exception&) {}
		}
		else
			lecui::form::message(error);
	};

	// add chat pane (dynamic)
	auto& chat_pane = lecui::containers::pane::add(collaboration_pane, "chat_pane", 0.f);
	chat_pane
		.rect(lecui::rect(ref_rect)
			.top(ref_rect.get_top() + _margin)
			.width(350.f))
		.on_resize(lecui::resize_params().height_rate(100.f));

	// add chat title
	auto& title = lecui::widgets::icon::add(chat_pane);
	title
		.rect(title.rect()
			.height(45.f)
			.left(_margin)
			.right(chat_pane.size().get_width() - _margin))
		.png_resource(png_chat)
		.text("Session Chat")
		.description("Collaborate via text with other session subscribers");

	// add messages pane
	auto& messages_pane = lecui::containers::pane::add(chat_pane, "messages");
	messages_pane
		.rect(lecui::rect(chat_pane.size())
			.top(title.rect().bottom() + _margin / 3.f)
			.bottom(chat_pane.size().get_height() - 30.f - _margin / 2.f))
		.on_resize(lecui::resize_params()
			.width_rate(100.f)
			.height_rate(100.f));

	// make pane invisible
	messages_pane
		.border(0.f);
	messages_pane
		.color_fill().alpha(0);

	// add message text field
	auto& message = lecui::widgets::text_field::add(chat_pane, "message");
	message
		.rect(lecui::rect(message.rect())
			.left(title.rect().left())
			.width(title.rect().width() - message.rect().height() - _margin / 3.f)
			.top(messages_pane.rect().bottom())
			.height(message.rect().height()))
		.on_resize(lecui::resize_params()
			.width_rate(100.f)
			.y_rate(100.f))
		.prompt("Type message here")
		.maximum_length(200)
		.events().action = [send_message]() {
		send_message();
	};

	// add send icon
	auto& send_icon = lecui::widgets::icon::add(chat_pane, "send");
	send_icon
		.padding(2.f)
		.rect(lecui::rect()
			.width(message.rect().height())
			.height(message.rect().height())
			.snap_to(message.rect(), snap_type::right, _margin / 3.f))
		.on_resize(lecui::resize_params()
			.x_rate(100.f)
			.y_rate(100.f))
		.png_resource(png_send)
		.events().action = [send_message]() {
		send_message();
	};

	return chat_pane;
}
