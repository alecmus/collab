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

// lecui
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/table_view.h>

// leccore
#include <liblec/leccore/system.h>
#include <liblec/leccore/app_version_info.h>
#include <liblec/leccore/hash.h>

// STL
#include <filesystem>
#include <sstream>

const lecui::size main_form::_design_size = lecui::size().width(1000.f).height(600.f);
const lecui::size main_form::_design_size_minimum = lecui::size().width(800.f).height(500.f);
const float main_form::_margin = 10.f;
const float main_form::_icon_size = 32.f;
const float main_form::_info_size = 20.f;
const float main_form::_title_font_size = 12.f;
const float main_form::_highlight_font_size = 14.f;
const float main_form::_detail_font_size = 10.f;
const float main_form::_ui_font_size = 9.f;
const float main_form::_caption_font_size = 8.f;
const float main_form::_review_font_size = 9.f;
const std::string main_form::_sample_text = "<u><strong>Aq</strong></u>";
const std::string main_form::_font = "Segoe UI";
const lecui::color main_form::_online = lecui::color().red(0).green(150).blue(0).alpha(180);
const lecui::color main_form::_busy = lecui::color().red(255).green(0).blue(0).alpha(180);

void main_form::on_close() {
	if (_installed)
		hide();
	else
		close();
}

void main_form::on_shutdown() {
	// remove the avatar so we can be able to delete the avatar file in the destructor
	set_avatar("");
}

void main_form::updates() {
	if (_check_update.checking() || _timer_man.running("update_check"))
		return;

	if (_timer_man.running("start_update_check"))
		_timer_man.stop("start_update_check");

	if (_download_update.downloading() || _timer_man.running("update_download"))
		return;

	std::string error, value;
	if (!_settings.read_value("updates", "readytoinstall", value, error)) {}

	if (!value.empty()) {
		// file integrity confirmed ... install update
		if (prompt("An update has already been downloaded and is ready to be installed.\nWould you like to apply the update now?")) {
			_restart_now = true;
			close();
		}
		return;
	}

	_update_check_initiated_manually = true;

	// create update status
	create_update_status();

	// start checking for updates
	_check_update.start();

	// start timer to keep progress of the update check
	_timer_man.add("update_check", 1500, [&]() { on_update_check(); });
}

void main_form::on_update_check() {
	if (_check_update.checking())
		return;

	// update status label
	try {
		auto& text = get_label("status::top/update_status").text();
		const size_t dot_count = std::count(text.begin(), text.end(), '.');

		if (dot_count == 0)
			text = "Checking for updates ...";
		else
			if (dot_count < 10)
				text += ".";
			else
				text = "Checking for updates ...";

		update();
	}
	catch (const std::exception&) {}

	// stop the update check timer
	_timer_man.stop("update_check");

	std::string error;
	if (!_check_update.result(_update_info, error)) {
		// update status label
		try {
			get_label("status::top/update_status").text("Error while checking for updates");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		if (!_setting_autocheck_updates || _update_check_initiated_manually)
			message("An error occurred while checking for updates:\n" + error);

		return;
	}

	// update found
	const std::string current_version(appversion);
	const int result = leccore::compare_versions(current_version, _update_info.version);
	if (result == -1) {
		// newer version available

		// update status label
		try {
			get_label("status::top/update_status").text("Update available: " + _update_info.version);
			update();
		}
		catch (const std::exception&) {}

		if (!_setting_autodownload_updates) {
			if (!prompt("<span style = 'font-size: 11.0pt;'>Update Available</span>\n\n"
				"Your version:\n" + current_version + "\n\n"
				"New version:\n<span style = 'color: rgb(0, 150, 0);'>" + _update_info.version + "</span>, " + _update_info.date + "\n\n"
				"<span style = 'font-size: 11.0pt;'>Description</span>\n" +
				_update_info.description + "\n\n"
				"Would you like to download the update now? (" +
				leccore::format_size(_update_info.size, 2) + ")\n\n"))
				return;
		}

		// create update download folder
		_update_directory = leccore::user_folder::temp() + "\\" + leccore::hash_string::uuid();

		if (!leccore::file::create_directory(_update_directory, error))
			return;	// to-do: perhaps try again one or two more times? But then again, why would this method fail?

		// update status label
		try {
			get_label("status::top/update_status").text("Downloading update ...");
			update();
		}
		catch (const std::exception&) {}

		// download update
		_download_update.start(_update_info.download_url, _update_directory);
		_timer_man.add("update_download", 1000, [&]() { on_update_download(); });
	}
	else {
		// update status label
		try {
			get_label("status::top/update_status").text("Latest version is already installed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		if (!_setting_autocheck_updates || _update_check_initiated_manually)
			message("The latest version is already installed.");
	}
}

void main_form::on_update_download() {
	leccore::download_update::download_info progress;
	if (_download_update.downloading(progress)) {
		// update status label
		try {
			auto& text = get_label("status::top/update_status").text();
			text = "Downloading update ...";

			if (progress.file_size > 0)
				text += " " + leccore::round_off::to_string(100. * (double)progress.downloaded / progress.file_size, 0) + "%";

			update();
		}
		catch (const std::exception&) {}
		return;
	}

	// stop the update download timer
	_timer_man.stop("update_download");

	auto delete_update_directory = [&]() {
		std::string error;
		if (!leccore::file::remove_directory(_update_directory, error)) {}
	};

	std::string error, fullpath;
	if (!_download_update.result(fullpath, error)) {
		// update status label
		try {
			get_label("status::top/update_status").text("Downloading update failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Download of update failed:\n" + error);
		delete_update_directory();
		return;
	}

	// update downloaded ... check file hash
	leccore::hash_file hash;
	hash.start(fullpath, { leccore::hash_file::algorithm::sha256 });

	while (hash.hashing()) {
		if (!keep_alive()) {
			// to-do: implement stopping mechanism
			//hash.stop()
			delete_update_directory();
			close_update_status();
			return;
		}
	}

	leccore::hash_file::hash_results results;
	if (!hash.result(results, error)) {
		// update status label
		try {
			get_label("status::top/update_status").text("Update file integrity check failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	try {
		const auto& result_hash = results.at(leccore::hash_file::algorithm::sha256);
		if (result_hash != _update_info.hash) {
			// update status label
			try {
				get_label("status::top/update_status").text("Update files seem to be corrupt");
				update();
				close_update_status();
			}
			catch (const std::exception&) {}

			// update file possibly corrupted
			message("Update downloaded but files seem to be corrupt and so cannot be installed. "
				"If the problem persists try downloading the latest version of the app manually.");
			delete_update_directory();
			return;
		}
	}
	catch (const std::exception& e) {
		error = e.what();

		// update status label
		try {
			get_label("status::top/update_status").text("Update file integrity check failed");
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded but file integrity check failed:\n" + error);
		delete_update_directory();
		return;
	}

	// save update location and update architecture
	const std::string update_architecture(architecture);
	if (!_settings.write_value("updates", "readytoinstall", fullpath, error) ||
		!_settings.write_value("updates", "architecture", update_architecture, error) ||
		!_settings.write_value("updates", "tempdirectory", _update_directory, error)) {

		// update status label
		try {
			get_label("status::top/update_status").text("Downloading update failed");	// to-do: improve
			update();
			close_update_status();
		}
		catch (const std::exception&) {}

		message("Update downloaded and verified but the following error occurred:\n" + error);
		delete_update_directory();
		return;
	}

	// update status label
	try {
		get_label("status::top/update_status")
			.text("v" + _update_info.version + " ready to be installed")
			.events().action = [this]() {
			if (prompt("Would you like to apply the update now?")) {
				_restart_now = true;
				close();
			}
		};
		update();
	}
	catch (const std::exception&) {}

	// file integrity confirmed ... install update
	if (prompt("Version " + _update_info.version + " is ready to be installed.\nWould you like to apply the update now?")) {
		_restart_now = true;
		close();
	}
}

bool main_form::installed() {
	// check if application is installed
	std::string error;
	leccore::registry reg(leccore::registry::scope::current_user);
	if (!reg.do_read("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + _install_guid_32 + "_is1",
		"InstallLocation", _install_location_32, error)) {
	}
	if (!reg.do_read("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + _install_guid_64 + "_is1",
		"InstallLocation", _install_location_64, error)) {
	}

	_installed = !_install_location_32.empty() || !_install_location_64.empty();

	auto portable_file_exists = []()->bool {
		try {
			std::filesystem::path path(".portable");
			return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
		}
		catch (const std::exception&) {
			return false;
		}
	};

	if (_installed) {
		// check if app is running from the install location
		try {
			const auto current_path = std::filesystem::current_path().string() + "\\";

			if (current_path != _install_location_32 &&
				current_path != _install_location_64) {
				if (portable_file_exists()) {
					_real_portable_mode = true;
					_installed = false;	// run in portable mode
				}
			}
		}
		catch (const std::exception&) {}
	}
	else {
		if (portable_file_exists())
			_real_portable_mode = true;
	}

	return _installed;
}

void main_form::create_update_status() {
	if (_update_details_displayed)
		return;

	_update_details_displayed = true;

	try {
		auto& update_status = get_label("status::top/update_status");
		update_status
			.text("Checking for updates");

		// update the ui
		update();
	}
	catch (const std::exception&) {
		// this shouldn't happen, seriously ... but added nonetheless for correctness
	}
}

void main_form::close_update_status() {
	// set timer for closing the update status
	_timer_man.add("update_status_timer", 3000, [this]() { on_close_update_status(); });
}

void main_form::on_close_update_status() {
	// stop close update status timer
	_timer_man.stop("update_status_timer");

	try {
		auto& update_status = get_label("status::top/update_status");
		update_status
			.text().clear();

		update();
	}
	catch (const std::exception&) {}

	_update_details_displayed = false;
}

void main_form::update_session_list() {
	// stop the timer
	_timer_man.stop("update_session_list");

	std::vector<collab::session> sessions;

	std::string error;
	if (_collab.get_sessions(sessions, error)) {
		// check if anything has changed
		if (sessions != _previous_sessions) {
			_previous_sessions = sessions;
			log("Sessions changed");

			try {
				auto& session_list = get_table_view("home/join_session_pane/session_list");

				// clear session list
				session_list
					.data().clear();

				// populate session list with latest data
				for (auto& session : sessions) {
					liblec::lecui::table_row row;
					row.insert(std::make_pair("UniqueID", session.unique_id));
					row.insert(std::make_pair("Name", session.name));
					row.insert(std::make_pair("Description", session.description));

					session_list.data().push_back(row);
				}

				// update
				update();
			}
			catch (const std::exception&) {}
		}
	}

	// resume the timer (5000ms looping ... room to breathe)
	_timer_man.add("update_session_list", 5000, [&]() {
		update_session_list();
		});
}

void main_form::log(const std::string& event) {
	liblec::auto_mutex lock(_log_mutex);

	liblec::log(event);

	// get the current time
	const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm time = { };
	localtime_s(&time, &now);

	// format time string
	std::stringstream ss;
	ss << std::put_time(&time, "%B %d, %H:%M:%S");

	// add to log queue
	_log_queue.push_back({ ss.str(), event });
}

void main_form::update_log() {
	// stop the timer
	_timer_man.stop("update_log");

	try {
		bool do_update = !_log_queue.empty();

		if (do_update) {
			auto& log_table = get_table_view("log/log_table");

			liblec::auto_mutex lock(_log_mutex);

			// retrieve log events in the queue and insert them into the log table
			for (const auto& info : _log_queue) {
				liblec::lecui::table_row row;
				row.insert(std::make_pair("Time", info.time));
				row.insert(std::make_pair("Event", info.event));

				log_table
					.data().push_back(row);
			}

			// clear the log queue
			_log_queue.clear();

			update();
		}
	}
	catch (const std::exception&) {}

	// resume the timer (1000ms looping ...)
	_timer_man.add("update_log", 1000, [&]() {
		update_log();
		});
}

int main_form::map_extension_to_resource(const std::string& extension) {
	if (extension == ".pdf")
		return png_pdf;

	if (extension == ".bmp" || extension == ".gif" || extension == ".png" || extension == ".jpg" || extension == ".jpeg")
		return png_image;

	if (extension == ".psd")
		return png_psd;

	if (extension == ".doc" || extension == ".docx")
		return png_doc;

	if (extension == ".xls" || extension == ".xlsx")
		return png_xls;

	if (extension == ".ppt" || extension == ".pptx")
		return png_ppt;

	if (extension == ".txt")
		return png_txt;

	if (extension == ".iso")
		return png_iso;

	if (extension == ".mp3" || extension == ".m4a" || extension == ".ogg" || extension == ".aac" || extension == ".wav" || extension == ".wma")
		return png_audio;

	if (extension == ".mp4" || extension == ".avi" || extension == ".3gp" || extension == ".wmv" || extension == ".mkv" || extension == ".webm")
		return png_video;

	if (extension == ".zip" || extension == ".rar" || extension == ".tar" || extension == ".gz" || extension == ".7z")
		return png_archive;

	if (extension == ".exe" || extension == ".dll" || extension == ".cmd" || extension == ".bat")
		return png_executable;

	return png_file;
}

main_form::main_form(const std::string& caption, bool restarted) :
	_cleanup_mode(restarted ? false : leccore::commandline_arguments::contains("/cleanup")),
	_update_mode(restarted ? false : leccore::commandline_arguments::contains("/update")),
	_recent_update_mode(restarted ? false : leccore::commandline_arguments::contains("/recentupdate")),
	_system_tray_mode(restarted ? false : leccore::commandline_arguments::contains("/systemtray")),
	_settings(installed() ? _reg_settings.base() : _ini_settings.base()),
	form(caption) {
	_installed = installed();

	if (!_installed)
		// don't allow system tray mode when running in portable mode
		_system_tray_mode = false;

	_reg_settings.set_registry_path("Software\\com.github.alecmus\\" + std::string(appname));
	_ini_settings.set_ini_path("");	// use app folder for ini settings

	if (_cleanup_mode || _update_mode || _recent_update_mode)
		force_instance();

	// caption event
	events().caption = [this]() {
		add_back_button();
		_page_man.show("help");
	};

	// initialize event
	events().initialize = [this](std::string& error) {
		return on_initialize(error);
	};

	// layout event
	events().layout = [this](std::string& error) {
		return on_layout(error);
	};

	// start event
	events().start = [this]() {
		return on_start();
	};

	// close event
	events().close = [this]() {
		return on_close();
	};

	// shutdown event
	events().shutdown = [this]() {
		return on_shutdown();
	};
}

main_form::~main_form() {
	// delete avatar file
	std::string error;
	if (!leccore::file::remove(_avatar_file, error)) {}

	if (_lock_file) {
		delete _lock_file;
		_lock_file = nullptr;
	}
}
