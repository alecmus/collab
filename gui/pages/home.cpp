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

#include "../../gui.h"
#include "../../helper_functions.h"

// lecui
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/widgets/icon.h>
#include <liblec/lecui/widgets/table_view.h>
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/text_field.h>
#include <liblec/lecui/menus/context_menu.h>

// leccore
#include <liblec/leccore/system.h>

void main_form::add_home_page() {
	auto& home = _page_man.add("home");

	// compute label heights
	const lecui::rect page_rect = { 0.f, home.size().get_width(), 0.f, home.size().get_height() };
	_title_height = _dim.measure_label(_sample_text, _font, _title_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();
	_highlight_height = _dim.measure_label(_sample_text, _font, _highlight_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();
	_detail_height = _dim.measure_label(_sample_text, _font, _detail_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();
	_caption_height = _dim.measure_label(_sample_text, _font, _caption_font_size, lecui::text_alignment::center, lecui::paragraph_alignment::top, page_rect).height();

	auto& ref_rect = lecui::rect()
		.left(_margin)
		.top(_margin)
		.width(home.size().get_width() - 2.f * _margin)
		.height(home.size().get_height() - 2.f * _margin);

	// add create new session pane
	auto& new_session_pane = lecui::containers::pane::add(home, "new_session_pane");
	new_session_pane
		.rect(lecui::rect()
			.width(500.f)
			.height(80.f)
			.place(ref_rect, 50.f, 0.f))
		.on_resize(lecui::resize_params()
			.x_rate(50.f)
			.y_rate(0.f));

	{
		auto& ref_rect = lecui::rect()
			.left(0.f)
			.top(0.f)
			.width(new_session_pane.size().get_width())
			.height(new_session_pane.size().get_height());

		// add create new session icon
		auto& icon = lecui::widgets::icon::add(new_session_pane);
		icon
			.rect(icon.rect().width(ref_rect.width()).place(ref_rect, 0.f, 50.f))
			.png_resource(png_new_session)
			.text("Create New Session")
			.description("Make a new session that other users on the local area network can join")
			.events().action = [&]() {
			new_session();
		};
	}

	// add join session pane
	auto& join_session_pane = lecui::containers::pane::add(home, "join_session_pane");
	join_session_pane
		.rect(lecui::rect()
			.left(new_session_pane.rect().left())
			.width(new_session_pane.rect().width())
			.top(new_session_pane.rect().bottom() + _margin)
			.bottom(ref_rect.bottom()))
		.on_resize(lecui::resize_params()
			.x_rate(50.f)
			.y_rate(0.f)
			.height_rate(100.f));

	{
		auto& ref_rect = lecui::rect()
			.left(0.f)
			.top(0.f)
			.width(join_session_pane.size().get_width())
			.height(join_session_pane.size().get_height());

		// add join session icon
		auto& icon = lecui::widgets::icon::add(join_session_pane);
		icon
			.rect(icon.rect().width(ref_rect.width()).place(ref_rect, 0.f, 0.f))
			.png_resource(png_join_session)
			.text("Join Existing Session")
			.description("Join an existing session to collaborate with other users on the local area network");

		// add session list
		auto& session_list = lecui::widgets::table_view::add(join_session_pane, "session_list");
		session_list
			.rect(lecui::rect()
				.left(icon.rect().left())
				.width(icon.rect().width())
				.top(icon.rect().bottom() + _margin)
				.bottom(ref_rect.bottom()))
			.on_resize(lecui::resize_params()
				.x_rate(50.f)
				.height_rate(100.f))
			.fixed_number_column(true)
			.columns({
				{ "Name", 120 },
				{ "Description", 300 }
				});

		session_list
			.events().context_menu = [&](const std::vector<lecui::table_row>& rows) {
			lecui::context_menu::specs menu_specs;
			menu_specs.items.push_back({ "Join", png_join_session });

			auto selected = lecui::context_menu::context_menu()(*this, menu_specs);

			if (selected == "Join") {
				try {
					const std::string unique_id = lecui::get::text(rows[0].at("UniqueID"));

					collab::session session;

					std::string error;
					if (!_collab.get_session(unique_id, session, error)) {
						message(error);
						return;
					}

					// to-do: implement session joining for first item in selection
					if (join_session(session)) {
						std::string error;

						// hide new session pane and join session pane
						_widget_man.hide("home/new_session_pane", error);
						_widget_man.hide("home/join_session_pane", error);

						// set collaboration session name
						try {
							auto& collaboration_pane = get_pane("home/collaboration_pane");

							auto& session_name = get_label("home/collaboration_pane/session_name");
							auto& session_description = get_label("home/collaboration_pane/session_description");
							auto& session_id = get_label("home/collaboration_pane/session_id");

							session_name.text(session.name);
							session_description.text(session.description);
							session_id.text("Session ID: " + session.unique_id);

							// add chat pane (dynamic)
							auto& ref_rect = lecui::rect()
								.left(0.f)
								.top(0.f)
								.width(collaboration_pane.size().get_width())
								.height(collaboration_pane.size().get_height());

							auto& chat_pane = lecui::containers::pane::add(collaboration_pane, "chat_pane", 0.f);
							chat_pane
								.rect(lecui::rect()
									.width(400.f)
									.top(session_description.rect().bottom() + _margin)
									.bottom(ref_rect.bottom() - 20.f))
								.on_resize(lecui::resize_params().height_rate(100.f));

							{
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
									.maximum_length(200);

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
									.events().action = [this]() {
									// to-do: implement sending message
								};
							}
						}
						catch (const std::exception&) {}

						// show collaboration pane
						_widget_man.show("home/collaboration_pane", error);
					}
				}
				catch (const std::exception&) {}
			}
		};
	}

	// add collaboration pane (to be hidden by default)
	auto& collaboration_pane = lecui::containers::pane::add(home, "collaboration_pane", 0.f);
	collaboration_pane
		.rect(ref_rect)
		.on_resize(lecui::resize_params()
			.width_rate(100.f)
			.height_rate(100.f));

	// make pane invisible
	collaboration_pane
		.border(0.f);
	collaboration_pane
		.color_fill().alpha(0);

	{
		auto& ref_rect = lecui::rect()
			.left(0.f)
			.top(0.f)
			.width(collaboration_pane.size().get_width())
			.height(collaboration_pane.size().get_height());

		// add back icon
		auto& back_icon = lecui::widgets::icon::add(collaboration_pane, "back");
		back_icon
			.rect(lecui::rect()
				.width(_title_height + _caption_height)
				.height(_title_height + _caption_height))
			.png_resource(_setting_darktheme ? png_back_dark : png_back_light)
			.events().action = [this]() {
			std::string error;

			// hide collaboration pane
			_widget_man.hide("home/collaboration_pane", error);

			// close chat pane
			_widget_man.close("home/collaboration_pane/chat_pane");

			// hide new session pane and join session pane
			_widget_man.show("home/new_session_pane", error);
			_widget_man.show("home/join_session_pane", error);
		};

		// add session name label
		auto& session_name = lecui::widgets::label::add(collaboration_pane, "session_name");
		session_name
			.rect(lecui::rect(session_name.rect())
				.left(back_icon.rect().right() + _margin / 2.f)
				.height(_title_height)
				.width(ref_rect.width()))
			.font_size(_title_font_size)
			.on_resize(lecui::resize_params()
				.width_rate(100.f));

		// add session description label
		auto& session_description = lecui::widgets::label::add(collaboration_pane, "session_description");
		session_description
			.rect(lecui::rect(session_name.rect())
				.height(_caption_height)
				.snap_to(session_name.rect(), snap_type::bottom, 0.f))
			.font_size(_caption_font_size)
			.color_text(_caption_color)
			.on_resize(lecui::resize_params()
				.width_rate(100.f));

		// add session unique id label
		auto& session_id = lecui::widgets::label::add(collaboration_pane, "session_id");
		session_id
			.rect(lecui::rect(session_description.rect())
				.place(ref_rect, 100.f, 100.f))
			.alignment(lecui::text_alignment::right)
			.font_size(_caption_font_size)
			.color_text(_caption_color)
			.on_resize(lecui::resize_params()
				.width_rate(100.f)
				.y_rate(100.f));
	}

	// add overlay images
	auto& large_overlay = lecui::widgets::image_view::add(home, "large_overlay");
	large_overlay
		.png_resource(icon_png_512)
		.opacity(3.f)
		.rect(lecui::rect()
			.width(256.f)
			.height(256.f)
			.place(ref_rect, 70.f, 0.f))
		.on_resize(lecui::resize_params()
			.x_rate(70.f)
			.y_rate(40.f));

	auto& medium_overlay = lecui::widgets::image_view::add(home, "medium_overlay");
	medium_overlay
		.png_resource(icon_png_512)
		.opacity(5.f)
		.rect(lecui::rect()
			.width(192.f)
			.height(192.f)
			.place(ref_rect, 100.f, 100.f))
		.on_resize(lecui::resize_params()
			.x_rate(100.f)
			.y_rate(100.f));

	auto& small_overlay = lecui::widgets::image_view::add(home, "small_overlay");
	small_overlay
		.png_resource(icon_png_512)
		.opacity(5.f)
		.rect(lecui::rect()
			.width(128.f)
			.height(128.f)
			.place(ref_rect, 20.f, 70.f))
		.on_resize(lecui::resize_params()
			.x_rate(-10.f)
			.y_rate(-20.f));
}
