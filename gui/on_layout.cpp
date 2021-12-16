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
#include "../helper_functions.h"
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/table_view.h>
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/containers/side_pane.h>
#include <liblec/lecui/widgets/icon.h>
#include <liblec/lecui/widgets/rectangle.h>

// leccore
#include <liblec/leccore/system.h>

bool main_form::on_layout(std::string& error) {
	// add side pane
	add_side_pane();

	// add pages
	add_home_page();
	add_settings_page();
	add_help_page();

	_page_man.show("home");
	return true;
}

void main_form::add_back_button() {
	// to-do check if back button already exists
	if (false)
		return;

	// add back button
	auto& status_pane = get_status_pane("status::left");

	auto ref_rect = lecui::rect().size(status_pane.size());

	// add settings icon
	auto& back_icon = lecui::widgets::icon::add(status_pane, "back");
	back_icon
		.rect(lecui::rect()
			.width(ref_rect.width())
			.height(ref_rect.width())
			.place(ref_rect, 50.f, 0.f))
		.png_resource(_setting_darktheme ? png_back_dark : png_back_light)
		.events().action = [this]() {
		// show previous page
		_page_man.show(_page_man.previous());

		if (_page_man.current() == "home") {
			// close the back icon
			_page_man.close("status::left/back");
		}
	};
}
