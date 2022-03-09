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
#include <liblec/lecui/widgets/rectangle.h>
#include <liblec/lecui/widgets/table_view.h>
#include <liblec/lecui/widgets/icon.h>
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/menus/context_menu.h>
#include <liblec/lecui/utilities/filesystem.h>

// leccore
#include <liblec/leccore/system.h>
#include <liblec/leccore/app_version_info.h>
#include <liblec/leccore/hash.h>
#include <liblec/leccore/file.h>

// STL
#include <filesystem>
#include <sstream>

const float main_form::_margin = 10.f;
const float main_form::_icon_size = 32.f;
const float main_form::_info_size = 20.f;
const float main_form::_title_font_size = 12.f;
const float main_form::_highlight_font_size = 14.f;
const float main_form::_detail_font_size = 10.f;
const float main_form::_ui_font_size = 9.f;
const float main_form::_caption_font_size = 8.f;
const std::string main_form::_sample_text = "<u><strong>Aq</strong></u>";
const std::string main_form::_font = "Segoe UI";
const lecui::color main_form::_online = lecui::color().red(0).green(150).blue(0).alpha(180);
const lecui::color main_form::_busy = lecui::color().red(255).green(0).blue(0).alpha(180);

int map_extension_to_resource(const std::string& extension) {
	if (extension == ".pdf")
		return png_pdf;
		
	if (extension == ".png")
		return png_png;
	
	if (extension == ".jpg" || extension == ".jpeg")
		return png_jpg;
	
	if (extension == ".bmp")
		return png_bmp;
	
	if (extension == ".gif")
		return png_gif;

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
		auto& text = get_label("home/update_status").text();
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
			get_label("home/update_status").text("Error while checking for updates");
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
			get_label("home/update_status").text("Update available: " + _update_info.version);
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
			get_label("home/update_status").text("Downloading update ...");
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
			get_label("home/update_status").text("Latest version is already installed");
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
			auto& text = get_label("home/update_status").text();
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
			get_label("home/update_status").text("Downloading update failed");
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
			get_label("home/update_status").text("Update file integrity check failed");
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
				get_label("home/update_status").text("Update files seem to be corrupt");
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
			get_label("home/update_status").text("Update file integrity check failed");
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
			get_label("home/update_status").text("Downloading update failed");	// to-do: improve
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
		get_label("home/update_status")
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
		auto& home = get_page("home");
		lecui::rect ref = lecui::rect()
			.left(_margin)
			.top(_margin)
			.width(home.size().get_width() - 2.f * _margin)
			.height(home.size().get_height() - 2.f * _margin);

		// add update status label
		auto& update_status = lecui::widgets::label::add(home, "update_status");
		update_status
			.text("Checking for updates")
			.color_text(lecui::color().red(100).green(100).blue(100))
			.font_size(8.f)
			.on_resize(lecui::resize_params()
				.y_rate(100.f))
			.rect().height(15.f).width(ref.width())
			.place(ref, 0.f, 100.f);

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
		auto& home = get_page("home");
		auto& update_status_specs = get_label("home/update_status");

		// close update status label
		_widget_man.close("home/update_status");
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

void main_form::update_session_chat_messages() {
	if (_current_session_unique_id.empty()) {
		_previous_messages.clear();
		return;	// exit immediately, user isn't currently part of any session
	}

	// stop the timer
	_timer_man.stop("update_session_chat_messages");

	std::vector<collab::message> messages;

	std::string error;
	if (_collab.get_messages(_current_session_unique_id, messages, error)) {
		// check if anything has changed
		if (messages != _previous_messages) {
			_previous_messages = messages;
			log("Session " + shorten_unique_id(_current_session_unique_id) + ": messages changed");

			try {
				auto& messages_pane = get_pane("home/collaboration_pane/chat_pane/messages");

				const auto ref_rect = lecui::rect(messages_pane.size());

				float bottom_margin = 0.f;

				std::string previous_sender_unique_id;

				// K = unique_id, T = display name
				std::map<std::string, std::string> display_names;

				bool latest_message_arrived = false;

				struct day_struct {
					int day = 0;
					int month = 0;
					int year = 0;

					bool operator==(const day_struct& param) {
						return (day == param.day) && (month == param.month) && (year == param.year);
					}

					bool operator!=(const day_struct& param) {
						return !operator==(param);
					}
				} previous_day;

				for (const auto& msg : messages) {
					if (msg.unique_id == _message_sent_just_now)
						latest_message_arrived = true;

					std::tm time = { };
					localtime_s(&time, &msg.time);

					std::stringstream ss;
					ss << std::put_time(&time, "%H:%M");
					std::string send_time = ss.str();

					day_struct this_day;
					this_day.day = time.tm_mday;
					this_day.month = time.tm_mon;
					this_day.year = time.tm_year;

					const bool day_change = this_day != previous_day;
					previous_day = this_day;

					const bool continuation = (previous_sender_unique_id == msg.sender_unique_id);
					const bool own_message = msg.sender_unique_id == _collab.unique_id();

					if (continuation && !day_change)
						bottom_margin -= (.85f * _margin);

					std::string display_name;

					// try to get this user's display name
					try {
						if (display_names.count(msg.sender_unique_id) == 0) {
							// fetch from local database
							std::string _display_name;
							if (_collab.get_user_display_name(msg.sender_unique_id, _display_name, error) && !_display_name.empty()) {
								// capture
								display_name = _display_name;

								// cache
								display_names[msg.sender_unique_id] = display_name;
							}
							else {
								// use the first ten characters in the user's unique id
								display_name.clear();

								for (size_t i = 0; i < 10; i++)
									display_name += msg.sender_unique_id[i];
							}
						}
						else {
							// use the cached display name
							display_name = display_names.at(msg.sender_unique_id);
						}
					}
					catch (const std::exception&) {
						// use shortened version of user's unique id
						display_name = shorten_unique_id(msg.sender_unique_id);
					}

					float font_size = _ui_font_size;

					const float content_margin = 10.f;

					// measure text height
					const float text_height = _dim.measure_label(msg.text, _font, font_size,
						lecui::text_alignment::left, lecui::paragraph_alignment::top,
						lecui::rect(ref_rect).width(ref_rect.width() - (2.f * content_margin))).height() +
						1.f;	// failsafe

					const float pane_height = (own_message || continuation) ?
						text_height + (2 * content_margin) :
						_caption_height + text_height + (2 * content_margin);

					if (day_change) {
						std::stringstream ss;
						ss << std::put_time(&time, "%B %d, %Y");
						std::string day_string = ss.str();

						// text size
						auto text_width = _dim.measure_label(day_string, _font, _caption_font_size,
							lecui::text_alignment::center, lecui::paragraph_alignment::top, ref_rect).width();

						auto& day_label_background = lecui::widgets::rectangle::add(messages_pane, day_string + "_rect");
						day_label_background
							.rect(lecui::rect(messages_pane.size())
								.top(bottom_margin + _margin / 2.f)
								.height(_caption_height + _margin / 2.f)
								.width(text_width + 2.f * _margin))
							.corner_radius_x(day_label_background.rect().height() / 2.f)
							.corner_radius_y(day_label_background.corner_radius_x())
							.color_fill(lecui::defaults::color(_setting_darktheme ? lecui::themes::dark : lecui::themes::light, lecui::item::text_field));

						auto& day_label = lecui::widgets::label::add(messages_pane, day_string);
						day_label
							.rect(lecui::rect(messages_pane.size())
								.top(bottom_margin + _margin / 2.f)
								.height(_caption_height))
							.alignment(lecui::text_alignment::center)
							.text(day_string)
							.font_size(_caption_font_size);

						day_label_background.rect().place(day_label.rect(), 50.f, 50.f);

						bottom_margin = day_label.rect().bottom() + _margin + _margin / 2.f;
					}

					// add message pane
					auto& pane = lecui::containers::pane::add(messages_pane, msg.unique_id, content_margin);
					pane
						.rect(lecui::rect(messages_pane.size())
							.top(bottom_margin)
							.height(pane_height))
						.color_fill(_setting_darktheme ?
							(own_message ?
								lecui::color().red(30).green(60).blue(100) :
								lecui::color().red(35).green(45).blue(60)) :
							(own_message ?
								lecui::color().red(245).green(255).blue(245) :
								lecui::color().red(255).green(255).blue(255)));
					pane
						.badge()
						.text(send_time)
						.color(lecui::color().alpha(0))
						.color_border(lecui::color().alpha(0))
						.color_text(_caption_color);

					// update bottom margin
					bottom_margin = pane.rect().bottom() + _margin;

					if (!(own_message || continuation)) {
						// add message label
						auto& label = lecui::widgets::label::add(pane, msg.unique_id + "_label");
						label
							.text("<strong>" + display_name + "</strong>")
							.rect(lecui::rect(pane.size()).height(_caption_height))
							.font_size(_caption_font_size);

						// add message text
						auto& text = lecui::widgets::label::add(pane, msg.unique_id + "_text");
						text
							.text(msg.text)
							.rect(lecui::rect(label.rect()).height(text_height).snap_to(label.rect(), snap_type::bottom, 0.f))
							.font_size(font_size);
					}
					else {
						// add message text
						auto& text = lecui::widgets::label::add(pane, msg.unique_id + "_text");
						text
							.text(msg.text)
							.rect(lecui::rect(pane.size()).height(text_height))
							.font_size(font_size);
					}

					previous_sender_unique_id = msg.sender_unique_id;
				}

				if (latest_message_arrived) {
					// clear
					_message_sent_just_now.clear();

					// scroll to the bottom
					messages_pane.scroll_vertically(-std::numeric_limits<float>::max());

					update();
				}
			}
			catch (const std::exception&) {}
		}
	}

	// resume the timer (1200ms looping ...)
	_timer_man.add("update_session_chat_messages", 1200, [&]() {
		update_session_chat_messages();
		});
}

void main_form::update_session_chat_files() {
	if (_current_session_unique_id.empty()) {
		_previous_files.clear();
		return;	// exit immediately, user isn't currently part of any session
	}

	// stop the timer
	_timer_man.stop("update_session_chat_files");

	std::vector<collab::file> files;
	std::string error;

	if (_collab.get_files(_current_session_unique_id, files, error)) {

		// check if anything has changed
		if (files != _previous_files) {
			_previous_files = files;
			log("Session " + shorten_unique_id(_current_session_unique_id) + ": files changed");

			for (const auto& it : files)
				_session_files[it.hash] = it;

			try {
				auto& content_pane = get_pane("home/collaboration_pane/files_pane/content");

				const auto ref_rect = lecui::rect(content_pane.size());

				float bottom_margin = 0.f;

				// K = unique_id, T = display name
				std::map<std::string, std::string> display_names;

				for (const auto& it : files) {
					auto& file = _session_files.at(it.hash);

					std::tm time = { };
					localtime_s(&time, &file.time);

					std::stringstream ss;
					ss << std::put_time(&time, "%d %B %Y, %H:%M");
					std::string send_date = ss.str();

					std::string display_name;

					// try to get this user's display name
					try {
						if (display_names.count(file.sender_unique_id) == 0) {
							// fetch from local database
							std::string _display_name;
							if (_collab.get_user_display_name(file.sender_unique_id, _display_name, error) && !_display_name.empty()) {
								// capture
								display_name = _display_name;

								// cache
								display_names[file.sender_unique_id] = display_name;
							}
							else {
								// use the first ten characters in the user's unique id
								display_name.clear();

								for (size_t i = 0; i < 10; i++)
									display_name += file.sender_unique_id[i];
							}
						}
						else {
							// use the cached display name
							display_name = display_names.at(file.sender_unique_id);
						}
					}
					catch (const std::exception&) {
						// use the first ten characters in the user's unique id
						display_name.clear();

						for (size_t i = 0; i < 10; i++)
							display_name += file.sender_unique_id[i];
					}

					const std::string file_name_text = "<strong>" + file.name + "</strong><span style = 'font-size: 8.0pt;'>" + file.extension + "</span>";

					const float content_margin = 10.f;

					const float description_height = _dim.measure_label(file.description, _font, _caption_font_size,
						lecui::text_alignment::left, lecui::paragraph_alignment::top,
						lecui::rect(ref_rect).width(ref_rect.width() - (2.f * content_margin))).height() +
						1.f;					// failsafe;

					const float file_name_height = _dim.measure_label(file_name_text, _font, _ui_font_size,
						lecui::text_alignment::left, lecui::paragraph_alignment::top,
						lecui::rect(ref_rect).width(ref_rect.width() - (2.f * content_margin) - 40.f - _margin)).height() +
						1.f;					// failsafe;

					const float file_pane_height =
						content_margin +		// top margin
						file_name_height +		// filename
						_caption_height +		// additional
						_caption_height +		// shared on
						(_margin / 2.f) +		// margin between shared on and description
						description_height +	// description
						content_margin;			// bottom margin

					auto& file_pane = lecui::containers::pane::add(content_pane, file.hash, content_margin);
					file_pane
						.rect(lecui::rect(ref_rect)
							.top(bottom_margin)
							.height(file_pane_height))
						.color_fill(_setting_darktheme ?
							lecui::color().red(35).green(45).blue(60) :
							lecui::color().red(255).green(255).blue(255));

					// update bottom margin
					bottom_margin = file_pane.rect().bottom() + _margin;

					// add rectangle for hit testing
					auto& hit_testing = lecui::widgets::rectangle::add(file_pane, file.hash + "_hit_testing");
					hit_testing
						.rect(lecui::rect(file_pane.size()))
						.corner_radius_x(5.f)
						.corner_radius_y(5.f)
						.color_fill(lecui::color().alpha(0))
						.color_border(lecui::color().alpha(0))
						.color_hot(lecui::defaults::color(_setting_darktheme ?
							lecui::themes::dark : lecui::themes::light, lecui::item::icon_hot).alpha(20));
					hit_testing
						.events().action = [&]() {
						std::string error;

						const auto destination_file = _files_staging_folder + "\\" + file.name + file.extension;

						// remove destination file if it already exists
						if (!leccore::file::remove(destination_file, error)) {}

						// extract the file
						if (!leccore::file::copy(_files_folder + "\\" + file.hash, destination_file, error))
							message("Error extracting file: " + error);
						else {
							if (!leccore::shell::open(destination_file, error))
								message("Error opening file: " + error);
						}
					};
					
					hit_testing
						.events().right_click = [&]() {
						std::string error;

						lecui::context_menu::specs menu_specs;
						menu_specs.items.push_back({ "Open", png_open_file });
						menu_specs.items.push_back({ "Save To ...", png_save_as });

						auto selected = lecui::context_menu::context_menu()(*this, menu_specs);

						if (selected == "Open") {
							const auto destination_file = _files_staging_folder + "\\" + file.name + file.extension;

							// remove destination file if it already exists
							if (!leccore::file::remove(destination_file, error)) {}

							// extract the file
							if (!leccore::file::copy(_files_folder + "\\" + file.hash, destination_file, error))
								message("Error extracting file: " + error);
							else {
								if (!leccore::shell::open(destination_file, error))
									message("Error opening file: " + error);
							}
						}

						if (selected == "Save To ...") {
							// prompt user for folder to save
							lecui::filesystem fs(*this);
							const auto folder = fs.select_folder("Select Folder");

							if (!folder.empty()) {
								const auto destination_file = folder + "\\" + file.name + file.extension;

								// remove destination file if it already exists
								if (!leccore::file::remove(destination_file, error)) {}

								// extract the file
								if (!leccore::file::copy(_files_folder + "\\" + file.hash, destination_file, error))
									message("Error extracting file: " + error);
								else {
									if (!leccore::shell::view(destination_file, error))
										message("Error opening folder: " + error);
								}
							}
						}
					};

					auto& file_image = lecui::widgets::icon::add(file_pane, file.hash + "_file_image");
					file_image
						.rect(lecui::rect()
							.width(40.f)
							.height(40.f))
						.png_resource(map_extension_to_resource(file.extension))
						.padding(0.f);

					auto& file_name = lecui::widgets::label::add(file_pane, file.hash + "_file_name");
					file_name
						.text(file_name_text)
						.font_size(_ui_font_size)
						.rect(file_name.rect()
							.left(file_image.rect().right() + _margin)
							.right(file_pane.size().get_width())
							.height(file_name_height));

					auto& additional = lecui::widgets::label::add(file_pane, file.hash + "_additional");
					additional
						.text("<strong>" + leccore::format_size(file.size, 2) + "</strong>, shared by <em>" + display_name + "</em>")
						.font_size(_caption_font_size)
						.color_text(_caption_color)
						.rect(lecui::rect(file_name.rect())
							.height(_caption_height)
							.snap_to(file_name.rect(), snap_type::bottom, 0.f));

					auto& shared_on = lecui::widgets::label::add(file_pane, file.hash + "_shared_on");
					shared_on
						.text("Shared on " + send_date)
						.font_size(_caption_font_size)
						.color_text(_caption_color)
						.rect(lecui::rect(file_name.rect())
							.height(_caption_height)
							.snap_to(additional.rect(), snap_type::bottom, 0.f));

					auto& file_description = lecui::widgets::label::add(file_pane, file.hash + "_file_description");
					file_description
						.text(file.description)
						.font_size(_caption_font_size)
						.rect(lecui::rect()
							.height(description_height)
							.snap_to(shared_on.rect(), snap_type::bottom, _margin / 2.f)
							.left(file_image.rect().left())
							.right(shared_on.rect().right()));
				}
			}
			catch (const std::exception&) {}
		}
	}

	// resume the timer (1200ms looping ...)
	_timer_man.add("update_session_chat_files", 1200, [&]() {
		update_session_chat_files();
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
