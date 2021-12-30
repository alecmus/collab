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

#include "../gui.h"
#include <liblec/lecui/containers/status_pane.h>
#include <liblec/lecui/widgets/rectangle.h>
#include <liblec/lecui/widgets/image_view.h>

void main_form::add_top_status_pane() {
	// add floating top status pane
	lecui::containers::status_pane_specs specs;
	specs
		.floating(true)
		.location(lecui::containers::status_pane_specs::pane_location::top)
		.thickness(36.f);
	auto& status_pane = lecui::containers::status_pane::add(*this, specs);
	
	auto& ref_rect = lecui::rect()
		.left(_margin / 2.f)
		.width(status_pane.size().get_width() - _margin)
		.height(status_pane.size().get_height());

	// add user icon
	auto& user_icon = lecui::widgets::image_view::add(status_pane, "user_icon");
	user_icon
		.rect(lecui::rect()
			.width(specs.thickness() - 6.f)
			.height(specs.thickness() - 6.f)
			.place(ref_rect, 100.f, 50.f))
		.on_resize(lecui::resize_params()
			.x_rate(100.f))
		.quality(lecui::image_quality::high)
		.corner_radius_x(user_icon.rect().width() / 2.f)
		.corner_radius_y(user_icon.rect().width() / 2.f)
		.png_resource(png_user)
		.tooltip("Click to view and edit user information, right click to change status");

	user_icon.events().action = [&]() {
		// to-do: add implementation
	};

	user_icon.events().right_click = [&]() {
		// to-do: add implementation
	};

	user_icon.badge().font_size(6.f).text(" ").color(_online);
}
