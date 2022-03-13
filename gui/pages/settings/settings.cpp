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
#include <liblec/lecui/widgets/toggle.h>
#include <liblec/lecui/widgets/line.h>
#include <liblec/lecui/widgets/rectangle.h>
#include <liblec/lecui/utilities/filesystem.h>

void main_form::add_settings_page() {
	auto& settings = _page_man.add("settings");

	const auto right = settings.size().get_width();
	const auto bottom = settings.size().get_height();

	// add page title
	auto& title = lecui::widgets::label::add(settings);
	title
		.text("<strong>APP SETTINGS</strong>")
		.rect(lecui::rect()
			.left(_margin)
			.right(right - _margin)
			.top(_margin)
			.height(25.f))
		.on_resize(lecui::resize_params().width_rate(100.f));

	const auto width = title.rect().width();

	// add general section
	auto& general_caption = lecui::widgets::label::add(settings);
	general_caption
		.text("<strong>General</strong>")
		.on_resize(lecui::resize_params().width_rate(100.f))
		.rect().width(width).snap_to(title.rect(), snap_type::bottom, _margin);

	auto& general_line = lecui::widgets::rectangle::add(settings);
	general_line
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(general_caption.rect());

	general_line.rect().top(general_line.rect().bottom());
	general_line.rect().height(.25f);
	general_line
		.border(.25f)
		.color_fill().alpha(0);

	// add dark theme toggle button
	auto& darktheme_caption = lecui::widgets::label::add(settings);
	darktheme_caption
		.text("Dark theme")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(20.f)
		.snap_to(general_line.rect(), snap_type::bottom, _margin);

	auto& darktheme = lecui::widgets::toggle::add(settings);
	darktheme
		.text("On").text_off("Off").tooltip("Change the UI theme").on(_setting_darktheme)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(darktheme_caption.rect().width()).snap_to(darktheme_caption.rect(), snap_type::bottom, 0.f);
	darktheme.events().toggle = [&](bool on) { on_darktheme(on); };

	// add auto start with windows toggle button
	auto& autostart_label = lecui::widgets::label::add(settings);
	autostart_label
		.text("Start automatically with Windows")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(darktheme.rect())
		.rect().snap_to(darktheme.rect(), snap_type::bottom, 2.f * _margin);

	auto& autostart = lecui::widgets::toggle::add(settings, "autostart");
	autostart.text("Yes").text_off("No")
		.tooltip("Select whether to automatically start the app when the user signs into Windows").on(_setting_autostart)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(darktheme.rect())
		.rect().snap_to(autostart_label.rect(), snap_type::bottom, 0.f);
	autostart.events().toggle = [&](bool on) { on_autostart(on); };

	// add location of collab files
	auto& location_caption = lecui::widgets::label::add(settings);
	location_caption
		.text("Location of files")
		.on_resize(lecui::resize_params().width_rate(100.f))
		.rect(autostart.rect())
		.rect().snap_to(autostart.rect(), snap_type::bottom, 2.f * _margin);

	auto& location = lecui::widgets::label::add(settings, "location");
	location
		.text(_folder)
		.tooltip("Click to change location")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).snap_to(location_caption.rect(), snap_type::bottom, 0.f);

	if (_installed)
		// changing location only allowed for installed version
		location.events().action = [&]() { on_select_location(); };

	// add updates section
	auto& updates_caption = lecui::widgets::label::add(settings);
	updates_caption
		.text("<strong>Updates</strong>")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect().width(width).snap_to(location.rect(), snap_type::bottom, 3.f * _margin);

	auto& updates_line = lecui::widgets::rectangle::add(settings);
	updates_line
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(updates_caption.rect());

	updates_line.rect().top(updates_line.rect().bottom());
	updates_line.rect().height(.25f);
	updates_line
		.border(.25f)
		.color_fill().alpha(0);

	// add auto check updates toggle button
	auto& autocheck_updates_caption = lecui::widgets::label::add(settings);
	autocheck_updates_caption
		.text("Auto-check")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect()
		.width(width)
		.height(20.f)
		.snap_to(updates_line.rect(), snap_type::bottom, _margin);

	auto& autocheck_updates = lecui::widgets::toggle::add(settings);
	autocheck_updates
		.text("Yes").text_off("No").tooltip("Select whether to automatically check for updates").on(_setting_autocheck_updates)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(darktheme.rect())
		.rect().snap_to(autocheck_updates_caption.rect(), snap_type::bottom, 0.f);
	autocheck_updates.events().toggle = [&](bool on) { on_autocheck_updates(on); };

	// add auto download updates toggle button
	auto& autodownload_updates_caption = lecui::widgets::label::add(settings);
	autodownload_updates_caption
		.text("Auto-download")
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(autocheck_updates_caption.rect())
		.rect().snap_to(autocheck_updates.rect(), snap_type::bottom, 2.f * _margin);

	auto& autodownload_updates = lecui::widgets::toggle::add(settings, "autodownload_updates");
	autodownload_updates
		.text("Yes").text_off("No").tooltip("Select whether to automatically download updates").on(_setting_autodownload_updates)
		.on_resize(lecui::resize_params()
			.width_rate(100.f))
		.rect(autocheck_updates.rect())
		.rect().snap_to(autodownload_updates_caption.rect(), snap_type::bottom, 0.f);

	autodownload_updates.events().toggle = [&](bool on) { on_autodownload_updates(on); };
}

void main_form::on_darktheme(bool on) {
	std::string error;
	if (!_settings.write_value("", "darktheme", on ? "on" : "off", error)) {
		message("Error saving dark theme setting: " + error);
		// to-do: set toggle button to saved setting (or default if unreadable)
	}
	else {
		if (_setting_darktheme != on) {
			if (prompt("Would you like to restart the app now for the changes to take effect?")) {
				_restart_now = true;
				close();
			}
		}
	}
}

void main_form::on_autostart(bool on) {
	std::string error;
	if (!_settings.write_value("", "autostart", on ? "yes" : "no", error)) {
		message("Error saving auto-start setting: " + error);
		// to-do: set toggle button to saved setting (or default if unreadable)
	}
	else
		_setting_autostart = on;

	if (_setting_autostart) {
		std::string command;
#ifdef _WIN64
		command = "\"" + _install_location_64 + "collab64.exe\"";
#else
		command = "\"" + _install_location_32 + "collab32.exe\"";
#endif
		command += " /systemtray";

		leccore::registry reg(leccore::registry::scope::current_user);
		if (!reg.do_write("Software\\Microsoft\\Windows\\CurrentVersion\\Run", "collab", command, error)) {}
	}
	else {
		leccore::registry reg(leccore::registry::scope::current_user);
		if (!reg.do_delete("Software\\Microsoft\\Windows\\CurrentVersion\\Run", "collab", error)) {}
	}
}

void main_form::on_autocheck_updates(bool on) {
	std::string error;
	if (!_settings.write_value("updates", "autocheck", on ? "yes" : "no", error)) {
		message("Error saving auto-check for updates setting: " + error);
		// to-do: set toggle button to saved setting (or default if unreadable)
	}
	else
		_setting_autocheck_updates = on;

	// disable autodownload_updates toggle button if autocheck_updates is off
	if (_setting_autocheck_updates)
		_widget_man.enable("settings/autodownload_updates", error);
	else
		_widget_man.disable("settings/autodownload_updates", error);

	update();
}

void main_form::on_autodownload_updates(bool on) {
	std::string error;
	if (!_settings.write_value("updates", "autodownload", on ? "yes" : "no", error)) {
		message("Error saving auto-download updates setting: " + error);
		// to-do: set toggle button to saved setting (or default if unreadable)
	}
	else
		_setting_autodownload_updates = on;
}

void main_form::on_select_location() {
	std::string folder = lecui::filesystem(*this)
		.select_folder("Select where to place the \"Collab\" folder");

	if (!folder.empty()) {
		folder += "\\Collab";

		std::string error;
		if (!_settings.write_value("", "folder", folder, error))
			message("Error saving folder location: " + error);
		else {
			const auto old_folder = _folder;
			_folder = folder;
			const auto new_folder = _folder;

			try {
				// change location label
				get_label("settings/location").text(_folder);
				update();
			}
			catch (const std::exception&) {}

			// to-do: add option to move files from older folder
		}
	}
}
