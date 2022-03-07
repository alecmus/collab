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

#pragma once

#include "version_info.h"
#include "resource.h"
#include "collab/collab.h"
#include "helper_functions.h"

// lecui
#include <liblec/lecui/instance.h>
#include <liblec/lecui/controls.h>
#include <liblec/lecui/appearance.h>
#include <liblec/lecui/utilities/timer.h>
#include <liblec/lecui/utilities/splash.h>
#include <liblec/lecui/utilities/tray_icon.h>
#include <liblec/lecui/widgets/widget.h>
#include <liblec/lecui/containers/page.h>

// leccore
#include <liblec/leccore/settings.h>
#include <liblec/leccore/web_update.h>
#include <liblec/leccore/file.h>

// lecnet
#include <liblec/lecnet.h>

// STL
#include <functional>

using namespace liblec;
using snap_type = lecui::rect::snap_type;

#ifdef _WIN64
#define architecture	"64bit"
#else
#define architecture	"32bit"
#endif

// the main form
class main_form : public lecui::form {
	const std::string _instance_guid = "{5AFD8741-0231-431C-B765-8DA09A22346B}";
	const std::string _install_guid_32 = "{45F2F3E8-AE98-4459-8F4F-836B4A79AD22}";
	const std::string _install_guid_64 = "{1F4BBE94-5058-4D4E-A6E8-4F1F7A562BD1}";
	const std::string _update_xml_url = "https://raw.githubusercontent.com/alecmus/collab/master/latest_update.xml";

	static const float _margin;
	static const float _icon_size;
	static const float _info_size;

	// font sizes, in points
	static const float _title_font_size;
	static const float _highlight_font_size;
	static const float _detail_font_size;
	static const float _ui_font_size;
	static const float _caption_font_size;

	// computed ideal label heights, in pixels
	float _title_height = 0.f;
	float _highlight_height = 0.f;
	float _detail_height = 0.f;
	float _ui_font_height = 0.f;
	float _caption_height = 0.f;

	static const std::string _sample_text;
	static const std::string _font;
	static const lecui::color _caption_color;
	static const lecui::color _online;
	static const lecui::color _busy;

	lecui::controls _ctrls{ *this };
	lecui::page_manager _page_man{ *this };
	lecui::appearance _apprnc{ *this };
	lecui::dimensions _dim{ *this };
	lecui::instance_manager _instance_man{ *this, _instance_guid };
	lecui::widget_manager _widget_man{ *this };
	lecui::timer_manager _timer_man{ *this };
	lecui::splash _splash{ *this };

	bool _restart_now = false;

	// 1. If application is installed and running from an install directory this will be true.
	// 2. If application is installed and not running from an install directory this will also
	// be true unless there is a .portable file in the same directory.
	// 3. If application is not installed then portable mode will be used whether or not a .portable
	// file exists in the same directory.
	bool _installed;
	bool _real_portable_mode;
	bool _system_tray_mode;
	std::string _install_location_32, _install_location_64;
	leccore::settings& _settings;
	leccore::registry_settings _reg_settings{ leccore::registry::scope::current_user };
	leccore::ini_settings _ini_settings{ "collab.ini" };
	bool _setting_darktheme = false;
	bool _setting_autocheck_updates = true;
	leccore::check_update _check_update{ _update_xml_url };
	leccore::check_update::update_info _update_info;
	bool _setting_autodownload_updates = false;
	bool _update_check_initiated_manually = false;
	leccore::download_update _download_update;
	std::string _update_directory;
	bool _setting_autostart = false;
	std::string _folder, _files_folder, _files_staging_folder;

	const bool _cleanup_mode;
	const bool _update_mode;
	const bool _recent_update_mode;

	lecui::tray_icon _tray_icon{ *this };

	bool _update_details_displayed = false;

	leccore::file::exclusive_lock* _lock_file = nullptr;

	std::string _database_file;
	std::string _avatar_file;
	collab _collab;	// collaboration object
	std::string _current_session_unique_id;
	std::string _message_sent_just_now;

	// concurrency control related to the log
	liblec::mutex _log_mutex;

	struct event_info {
		std::string time;
		std::string event;
	};

	std::vector<event_info> _log_queue;
	std::map<std::string, collab::file> _session_files;

	bool on_initialize(std::string& error) override;
	bool on_layout(std::string& error) override;
	void on_start() override;
	void on_close() override;
	void on_shutdown() override;
	void add_side_pane();
	void add_top_status_pane();
	void add_back_button();
	void add_home_page();
	void add_help_page();
	void add_settings_page();
	void add_log_page();

	void updates();
	void on_update_check();
	void on_update_download();
	bool installed();
	void create_update_status();
	void close_update_status();
	void on_close_update_status();

	void on_darktheme(bool on);
	void on_autostart(bool on);
	void on_autocheck_updates(bool on);
	void on_autodownload_updates(bool on);
	void on_select_location();

	void new_session();
	void user();
	bool join_session(const collab::session& session);

	void set_avatar(const std::string& image_data);
	void update_session_list();
	void update_session_chat_messages();
	void update_session_chat_files();

	void log(const std::string& event);
	void update_log();

public:
	main_form(const std::string& caption, bool restarted);
	~main_form();
	bool restart_now() {
		return _restart_now;
	}
};
