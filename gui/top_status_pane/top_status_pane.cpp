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
#include <liblec/lecui/containers/status_pane.h>
#include <liblec/lecui/widgets/label.h>
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

	// add avatar
	auto& avatar = lecui::widgets::image_view::add(status_pane, "avatar");
	avatar
		.rect(lecui::rect()
			.width(specs.thickness() - 6.f)
			.height(specs.thickness() - 6.f)
			.place(ref_rect, 100.f, 50.f))
		.on_resize(lecui::resize_params()
			.x_rate(100.f))
		.quality(lecui::image_quality::high)
		.corner_radius_x(avatar.rect().width() / 2.f)
		.corner_radius_y(avatar.rect().width() / 2.f)
		.png_resource(png_user)
		.tooltip("Click to view and edit user information, right click to change status");

	avatar.events().action = [&]() { user(); };

	avatar.events().right_click = [&]() {
		// to-do: add implementation
	};

	avatar.badge().font_size(6.f).text(" ").color(_online);

	// add session unique id label
	auto& session_id = lecui::widgets::label::add(status_pane, "session_id");
	session_id
		.rect(lecui::rect(ref_rect)
			.right(avatar.rect().left() - _margin))
		.alignment(lecui::text_alignment::right)
		.paragraph_alignment(lecui::paragraph_alignment::bottom)
		.font_size(_caption_font_size)
		.color_text(_caption_color)
		.on_resize(lecui::resize_params()
			.x_rate(100.f));
}

void main_form::set_avatar(const std::string& image_data) {
	try {
		std::string error;

		auto& avatar = get_image_view("status::top/avatar");
		avatar
			.png_resource(png_user)
			.file("");

		// refresh so image is released if it exists
		if (!_widget_man.refresh("status::top/avatar", error)) {}

		if (!image_data.empty()) {
			// save to file
			if (leccore::file::write(_avatar_file, image_data, error)) {
				avatar
					.png_resource(0)
					.file(_avatar_file);

				if (!_widget_man.refresh("status::top/avatar", error)) {}
			}
		}
	}
	catch (const std::exception&) {}
}
