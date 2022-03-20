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

#include "../../../gui.h"
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/line.h>
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/rectangle.h>

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
			.height(25.f))
		.on_resize(lecui::resize_params()
			.width_rate(100.f));

	const auto width = title.rect().width();

	// add app icon
	auto& app_icon = lecui::widgets::image_view::add(help);
	app_icon
		.png_resource(icon_png_256)
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
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).height(20.f).snap_to(app_icon.rect(), snap_type::bottom_left, _margin);

	// add copyright information
	auto& copyright = lecui::widgets::label::add(help);
	copyright
		.text("<span style = 'font-size: 8.0pt;'>© 2021 Alec Musasa</span>")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).snap_to(app_version.rect(), snap_type::bottom, 0.f);

	// add more information
	auto& more_info_caption = lecui::widgets::label::add(help);
	more_info_caption
		.text("<strong>For more info</strong>")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).snap_to(copyright.rect(), snap_type::bottom, _margin);

	auto& github_link = lecui::widgets::label::add(help);
	github_link
		.text("Visit https://github.com/alecmus/collab")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
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
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).snap_to(github_link.rect(), snap_type::bottom, _margin);

	auto& leccore_version = lecui::widgets::label::add(help);
	leccore_version
		.text(leccore::version())
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
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
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
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
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
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
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).snap_to(lecnet_version.rect(), snap_type::bottom, _margin);

	auto& freepik = lecui::widgets::label::add(help);
	freepik
		.text("Icons made by Freepik from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(addition_credits_caption.rect(), snap_type::bottom, _margin / 5.f);
	freepik
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.freepik.com", error))
			message(error);
	};

	auto& dmitri13 = lecui::widgets::label::add(help);
	dmitri13
		.text("Icons made by dmitri13 from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(freepik.rect(), snap_type::bottom, _margin / 5.f);
	dmitri13
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/dmitri13", error))
			message(error);
	};

	auto& goodware = lecui::widgets::label::add(help);
	goodware
		.text("Icons made by Good Ware from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(dmitri13.rect(), snap_type::bottom, _margin / 5.f);
	goodware
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/good-ware", error))
			message(error);
	};

	auto& dimitry_miroliubov = lecui::widgets::label::add(help);
	dimitry_miroliubov
		.text("Icons made by Dimitry Miroliubov from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(goodware.rect(), snap_type::bottom, _margin / 5.f);
	dimitry_miroliubov
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/dimitry-miroliubov", error))
			message(error);
	};

	auto& roman_kacerek = lecui::widgets::label::add(help);
	roman_kacerek
		.text("Icons made by Roman Kacerek from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(dimitry_miroliubov.rect(), snap_type::bottom, _margin / 5.f);
	roman_kacerek
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/roman-kacerek", error))
			message(error);
	};

	auto& vectorslab = lecui::widgets::label::add(help);
	vectorslab
		.text("Icons made by Vectorslab from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(roman_kacerek.rect(), snap_type::bottom, _margin / 5.f);
	vectorslab
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/vectorslab", error))
			message(error);
	};

	auto& berkahicon = lecui::widgets::label::add(help);
	berkahicon
		.text("Icons made by berkahicon from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.height(_caption_height)
		.width(width)
		.snap_to(vectorslab.rect(), snap_type::bottom, _margin / 5.f);
	berkahicon
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/berkahicon", error))
			message(error);
	};

	auto& pixel_perfect = lecui::widgets::label::add(help);
	pixel_perfect
		.text("Icons made by Pixel perfect from https://www.flaticon.com")
		.font_size(_caption_font_size)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_caption_height)
		.snap_to(berkahicon.rect(), snap_type::bottom, _margin / 5.f);
	pixel_perfect
		.events().action = [this]() {
		std::string error;
		if (!leccore::shell::open("https://www.flaticon.com/authors/pixel-perfect", error))
			message(error);
	};

	// add line
	auto& license_line = lecui::widgets::rectangle::add(help);
	license_line
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(lecui::rect(pixel_perfect.rect()));
	license_line.rect().top(license_line.rect().bottom());
	license_line.rect().top() += 1.f * _margin;
	license_line.rect().bottom() += 1.f * _margin;

	license_line.rect().height(.25f);
	license_line
		.border(.25f)
		.color_fill().alpha(0);

	// add license information
	auto& license_notice = lecui::widgets::label::add(help);
	license_notice
		.text("This app is free software released under the MIT License.")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(_ui_font_height + _margin)
		.snap_to(license_line.rect(), snap_type::bottom, _margin);

	// add dummy rectangle behind the license notice for UI scrolling to honor the margin below the label
	auto& rectangle = lecui::widgets::rectangle::add(help);
	rectangle
		.rect(license_notice.rect())
		.color_fill(lecui::color().alpha(0))
		.border(0.f)
		.on_resize(license_notice.on_resize());
}
