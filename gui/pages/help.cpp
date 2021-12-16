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
#include <liblec/lecui/widgets/line.h>
#include <liblec/lecui/widgets/image_view.h>

#include <liblec/leccore/system.h>

void main_form::add_help_page() {
	auto& help = _page_man.add("help");

	const auto right = help.size().get_width();
	const auto bottom = help.size().get_height();

	// add page title
	auto& title = lecui::widgets::label::add(help);
	title
		.text("<strong>ABOUT THIS APP</strong>")
		.rect(lecui::rect()
			.left(_margin)
			.right(right - _margin)
			.top(_margin)
			.height(25.f));

	const auto width = title.rect().width();

	// add app icon
	auto& app_icon = lecui::widgets::image_view::add(help);
	app_icon
		.png_resource(png_icon_256)
		.rect()
		.width(128.f)
		.height(128.f)
		.snap_to(title.rect(), snap_type::bottom_left, 0.f);

	// add app version label
	auto& app_version = lecui::widgets::label::add(help);
	app_version
		.text("<span style = 'font-size: 9.0pt;'>" +
			std::string(appname) + " " + std::string(appversion) + " (" + std::string(architecture) + "), " + std::string(appdate) +
			"</span>")
		.rect().width(width).height(20.f).snap_to(app_icon.rect(), snap_type::bottom_left, _margin);

	// add copyright information
	auto& copyright = lecui::widgets::label::add(help);
	copyright
		.text("<span style = 'font-size: 8.0pt;'>© 2021 Alec Musasa</span>")
		.rect().width(width).snap_to(app_version.rect(), snap_type::bottom, 0.f);

	// add more information
	auto& more_info_caption = lecui::widgets::label::add(help);
	more_info_caption
		.text("<strong>For more info</strong>")
		.rect().width(width).snap_to(copyright.rect(), snap_type::bottom, _margin);

	auto& github_link = lecui::widgets::label::add(help);
	github_link
		.text("Visit https://github.com/alecmus/collab")
		.rect().width(width).snap_to(more_info_caption.rect(), snap_type::bottom, 0.f);
	github_link
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://github.com/alecmus/collab", error))
			message(error);
	};

	// add libraries used
	auto& libraries_used_caption = lecui::widgets::label::add(help);
	libraries_used_caption
		.text("<strong>Libraries used</strong>")
		.rect().width(width).snap_to(github_link.rect(), snap_type::bottom, _margin);

	auto& leccore_version = lecui::widgets::label::add(help);
	leccore_version
		.text(leccore::version())
		.rect().width(width).snap_to(libraries_used_caption.rect(), snap_type::bottom, 0.f);
	leccore_version
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://github.com/alecmus/leccore", error))
			message(error);
	};

	auto& lecui_version = lecui::widgets::label::add(help);
	lecui_version
		.text(lecui::version())
		.rect().width(width).snap_to(leccore_version.rect(), snap_type::bottom, 0.f);
	lecui_version
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://github.com/alecmus/lecui", error))
			message(error);
	};

	auto& lecnet_version = lecui::widgets::label::add(help);
	lecnet_version
		.text(lecnet::version())
		.rect().width(width).snap_to(lecui_version.rect(), snap_type::bottom, 0.f);
	lecnet_version
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://github.com/alecmus/lecnet", error))
			message(error);
	};

	// add additional credits
	auto& addition_credits_caption = lecui::widgets::label::add(help);
	addition_credits_caption
		.text("<strong>Additional credits</strong>")
		.rect().width(width).snap_to(lecnet_version.rect(), snap_type::bottom, _margin);

	auto& freepik = lecui::widgets::label::add(help);
	freepik
		.text("Icons made by Freepik from https://www.flaticon.com")
		.rect().width(width).snap_to(addition_credits_caption.rect(), snap_type::bottom, 0.f);
	freepik
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.freepik.com", error))
			message(error);
	};

	auto& dmitri13 = lecui::widgets::label::add(help);
	dmitri13
		.text("Icons made by dmitri13 from https://www.flaticon.com")
		.rect().width(width).snap_to(freepik.rect(), snap_type::bottom, 0.f);
	dmitri13
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/dmitri13", error))
			message(error);
	};

	// add line
	auto& license_line = lecui::widgets::line::add(help);
	license_line
		.thickness(0.25f)
		.rect(lecui::rect(dmitri13.rect()));
	license_line.rect().top(license_line.rect().bottom());
	license_line.rect().top() += 5.f * _margin;
	license_line.rect().bottom() += 5.f * _margin;

	license_line
		.points(
			{
				lecui::point().x(0.f).y(0.f),
				lecui::point().x(license_line.rect().width()).y(0.f)
			});

	// add license information
	auto& license_notice = lecui::widgets::label::add(help);
	license_notice
		.text("This app is free software released under the MIT License.")
		.rect().width(width).snap_to(license_line.rect(), snap_type::bottom, _margin);
}
