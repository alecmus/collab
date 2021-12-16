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

#include <algorithm>

void main_form::on_start() {
	if (_installed) {
		std::string error;
		if (!_tray_icon.add(ico_resource, std::string(appname) + " " +
			std::string(appversion) + " (" + std::string(architecture) + ")",
			{
			{ "<strong>Show Collab</strong>", [this]() {
				if (minimized())
					restore();
				else
					show();
			} },
			{ "" },
			{ "Settings", [this]() {
				add_back_button();
				_page_man.show("settings");

				if (minimized())
					restore();
				else
					show();
			}
			},
			{ "Updates", [this]() {
				if (_page_man.current() != "home") {
					// show home page
					_page_man.show("home");

					// close the back icon
					_page_man.close("status::left/back");
				}

				updates();

				if (minimized())
					restore();
				else
					show();
			} },
			{ "About", [this]() {
				add_back_button();
				_page_man.show("help");

				if (minimized())
					restore();
				else
					show();
			}
			},
			{ "" },
			{ "Exit", [this]() { close(); } }
			},
			"Show Collab", error)) {
		}
	}

	std::string error;
	// disable autodownload_updates toggle button if autocheck_updates is off
	if (_setting_autocheck_updates)
		_widget_man.enable("settings/autodownload_updates", error);
	else
		_widget_man.disable("settings/autodownload_updates", error);

	if (_installed)
		_widget_man.enable("settings/autostart", error);
	else
		_widget_man.disable("settings/autostart", error);

	_splash.remove();
}
