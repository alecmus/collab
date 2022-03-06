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
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/table_view.h>

#include <liblec/leccore/system.h>

void main_form::add_log_page() {
	auto& log = _page_man.add("log");

	const auto right = log.size().get_width();
	const auto bottom = log.size().get_height();

	// add page title
	auto& title = lecui::widgets::label::add(log);
	title
		.text("<strong>LOG</strong>")
		.rect(lecui::rect()
			.left(_margin)
			.right(right - _margin)
			.top(_margin)
			.height(25.f))
		.on_resize(lecui::resize_params()
			.width_rate(100.f));

	auto& log_table = lecui::widgets::table_view::add(log, "log_table");
	log_table
		.user_sort(false)
		.fixed_number_column(true)
		.rect(lecui::rect()
			.left(_margin)
			.right(right - _margin)
			.top(title.rect().bottom())
			.bottom(bottom - _margin))
		.on_resize(lecui::resize_params()
			.width_rate(100.f)
			.height_rate(100.f))
		.columns({
				{ "Time", 140 },
				{ "Event", 900 }
			});
}
