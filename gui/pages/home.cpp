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

#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/widgets/icon.h>
#include <liblec/lecui/widgets/table_view.h>
#include <liblec/lecui/widgets/image_view.h>

#include <liblec/leccore/system.h>

void main_form::add_home_page() {
	auto& home = _page_man.add("home");

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
			.events().action = []() {};
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
			.x_rate(60.f)
			.y_rate(10.f));

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
