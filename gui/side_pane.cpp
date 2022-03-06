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
#include <liblec/lecui/containers/side_pane.h>
#include <liblec/lecui/widgets/rectangle.h>
#include <liblec/lecui/widgets/icon.h>

void main_form::add_side_pane() {
	// add side pane
	auto& side_pane = lecui::containers::side_pane::add(*this, 42.f);

	const auto side_pane_rect = lecui::rect().size(side_pane.size());

	// add rectangle for use as side pane background
	auto& rectangle = lecui::widgets::rectangle::add(side_pane);
	rectangle
		.rect(lecui::rect()
			.width(side_pane.size().get_width())
			.height(side_pane.size().get_height()))
		.border(0.f)

		// base the color of the side bar off the background color of the form
		// slightly darker if in light them and slightly lighter if in dark theme
		.color_fill(_setting_darktheme ? _apprnc.get_background().lighten(5.f) : _apprnc.get_background().darken(5.f))
		.on_resize(lecui::resize_params()
			.height_rate(100.f));

	// define size of side bar icons
	const float icon_size = side_pane_rect.width();

	const float _icon_gap = 15.f;

	// add updates icon
	auto& updates_icon = lecui::widgets::icon::add(side_pane);
	updates_icon
		.tooltip("Updates")
		.rect(lecui::rect()
			.width(side_pane_rect.width())
			.height(side_pane_rect.width())
			.place(side_pane_rect, 50.f, 100.f)
			.move(side_pane_rect.get_left(), side_pane_rect.get_bottom() - 4.f * (icon_size + _icon_gap) + _icon_gap))
		.on_resize(lecui::resize_params()
			.y_rate(100.f))
		.png_resource(png_updates)
		.events().action = [&]() {
		if (_page_man.current() != "home") {
			// show home page
			_page_man.show("home");

			// close the back icon
			_page_man.close("status::left/back");
		}

		updates();
	};

	// add log icon
	auto& log_icon = lecui::widgets::icon::add(side_pane);
	log_icon
		.tooltip("Log")
		.rect(lecui::rect()
			.width(side_pane_rect.width())
			.height(side_pane_rect.width())
			.place(side_pane_rect, 50.f, 100.f)
			.move(side_pane_rect.get_left(), side_pane_rect.get_bottom() - 3.f * (icon_size + _icon_gap) + _icon_gap))
		.on_resize(lecui::resize_params()
			.y_rate(100.f))
		.png_resource(png_log)
		.events().action = [this]() {
		add_back_button();
		_page_man.show("log");
	};

	// add settings icon
	auto& settings_icon = lecui::widgets::icon::add(side_pane);
	settings_icon
		.tooltip("Settings")
		.rect(lecui::rect()
			.width(side_pane_rect.width())
			.height(side_pane_rect.width())
			.place(side_pane_rect, 50.f, 100.f)
			.move(side_pane_rect.get_left(), side_pane_rect.get_bottom() - 2.f * (icon_size + _icon_gap) + _icon_gap))
		.on_resize(lecui::resize_params()
			.y_rate(100.f))
		.png_resource(png_settings)
		.events().action = [&]() {
		try {
			add_back_button();
			_page_man.show("settings");
		}
		catch (const std::exception& e) { message(e.what()); }
	};

	// add help icon
	auto& help_icon = lecui::widgets::icon::add(side_pane);
	help_icon
		.tooltip("Help")
		.rect(lecui::rect()
			.width(side_pane_rect.width())
			.height(side_pane_rect.width())
			.place(side_pane_rect, 50.f, 100.f)
			.move(side_pane_rect.get_left(), side_pane_rect.get_bottom() - 1.f * (icon_size + _icon_gap) + _icon_gap))
		.on_resize(lecui::resize_params()
			.y_rate(100.f))
		.png_resource(png_help)
		.events().action = [this]() {
		add_back_button();
		_page_man.show("help");
	};
}
