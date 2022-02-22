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

// lecui
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/password_field.h>
#include <liblec/lecui/widgets/button.h>

// leccore
#include <liblec/leccore/hash.h>

bool main_form::join_session(const collab::session& session) {
	class join_session_form : public form {
		lecui::controls _ctrls{ *this };
		lecui::page_manager _page_man{ *this };
		lecui::widget_manager _widget_man{ *this };
		lecui::appearance _apprnc{ *this };
		lecui::dimensions _dim{ *this };

		main_form& _main_form;
		const collab::session& _session;
		bool _join_successful = false;
		std::map<std::string, int>& _attempts_remaining;

		bool on_initialize(std::string& error) override {
			// size and stuff
			_ctrls
				.allow_resize(false)
				.allow_minimize(false);

			_apprnc
				.main_icon(ico_resource)
				.mini_icon(ico_resource)
				.caption_icon(get_dpi_scale() < 2.f ? icon_png_32 : icon_png_64)
				.theme(_main_form._setting_darktheme ? lecui::themes::dark : lecui::themes::light);
			_dim.set_size(lecui::size().width(300.f).height(230.f));

			return true;
		}

		bool on_layout(std::string& error) override {
			// add home page
			auto& home = _page_man.add("home");

			auto& ref_rect = lecui::rect()
				.left(_margin)
				.top(_margin)
				.width(home.size().get_width() - 2.f * _margin)
				.height(home.size().get_height() - 2.f * _margin);

			// add session name caption
			auto& session_name = lecui::widgets::label::add(home);
			session_name
				.text(_session.name)
				.font_size(_detail_font_size)
				.rect(lecui::rect()
					.left(_margin)
					.top(_margin)
					.width(ref_rect.width())
					.height(_main_form._detail_height));

			// add session description
			auto& session_description = lecui::widgets::label::add(home);
			session_description
				.text(_session.description)
				.font_size(_caption_font_size)
				.color_text(_caption_color)
				.rect(lecui::rect(session_name.rect())
					.height(_main_form._caption_height * 2.5f)
					.snap_to(session_name.rect(), snap_type::bottom, 0.f));

			// add session password pane
			auto& session_password_pane = lecui::containers::pane::add(home, "session_password_pane");
			session_password_pane
				.rect()
				.width(session_name.rect().width())
				.height(80.f)
				.snap_to(session_description.rect(), snap_type::bottom, _margin);

			{
				auto& ref_rect = lecui::rect()
					.width(session_password_pane.size().get_width())
					.height(session_password_pane.size().get_height());

				// add password caption
				auto& session_password_caption = lecui::widgets::label::add(session_password_pane);
				session_password_caption
					.text("Password")
					.font_size(_caption_font_size)
					.color_text(_caption_color)
					.rect(lecui::rect()
						.width(ref_rect.width())
						.height(_main_form._caption_height)
						.place(ref_rect, 50.f, 0.f));

				// add password field
				auto& session_password = lecui::widgets::password_field::add(session_password_pane, "session_password");
				session_password
					.prompt("Enter session password")
					.maximum_length(40)
					.rect(lecui::rect(session_password.rect())
						.width(session_password_caption.rect().width())
						.snap_to(session_password_caption.rect(), snap_type::bottom, _margin / 4.f));
				session_password
					.events().action = [&]() { on_join(); };

				// add attempts remaining dialog
				auto& attempts_remaining = lecui::widgets::label::add(session_password_pane, "attempts_remaining");
				attempts_remaining
					.font_size(_main_form._caption_font_size)
					.color_text(lecui::color().red(255).green(0).blue(0))
					.alignment(lecui::text_alignment::right)
					.rect(lecui::rect(session_password.rect())
						.height(_main_form._caption_height)
						.snap_to(session_password.rect(), snap_type::bottom, 0.f));

				if (_attempts_remaining[_session.unique_id] < 3) {
					const std::string attempts_string = std::to_string(_attempts_remaining[_session.unique_id]) +
						" attempt" + (_attempts_remaining[_session.unique_id] == 1 ?
							std::string("") : std::string("s")) + " remaining";

					attempts_remaining
						.text(attempts_string);
				}
			}

			// add join button
			auto& join_session = lecui::widgets::button::add(home, "join_session");
			join_session
				.text("Join")
				.rect(lecui::rect(join_session.rect()).snap_to(session_password_pane.rect(), snap_type::bottom, _margin))
				.events().action = [&]() { on_join(); };

			_page_man.show("home");
			return true;
		}

		void on_join() {
			try {
				auto& session_password = get_password_field("home/session_password_pane/session_password");

				if (session_password.text().empty())
					return;

				if (leccore::hash_string::sha256(session_password.text()) == _session.passphrase_hash) {
					// join accepted
					_join_successful = true;

					try {
						// reset counter for this session
						if (_attempts_remaining.count(_session.unique_id) != 0)
							_attempts_remaining.at(_session.unique_id) = 3;
					}
					catch (const std::exception&) {}

					// remove temporary session entry
					std::string error;
					if (!_main_form._collab.remove_temporary_session_entry(_session.unique_id, error)) {
						// possibly not in the temporary session list
					}

					close();
				}
				else {
					try {
						// reduce counter for this session
						_attempts_remaining[_session.unique_id]--;

						auto& attempts_remaining_label = get_label("home/session_password_pane/attempts_remaining");

						const std::string attempts_string = std::to_string(_attempts_remaining[_session.unique_id]) +
							" attempt" + (_attempts_remaining[_session.unique_id] == 1 ?
								std::string("") : std::string("s")) + " remaining";

						attempts_remaining_label.text(attempts_string);
						update();
					}
					catch (const std::exception&) {}

					if (_attempts_remaining[_session.unique_id] <= 0) {
						message("Too many invalid login attempts!");
						close();
					}
					else
						message("Invalid session password!");
				}
			}
			catch (const std::exception&) {}
		}

	public:
		join_session_form(const std::string& caption,
			main_form& main_form, const collab::session& session, std::map<std::string, int>& attempts_remaining) :
			form(caption, main_form),
			_main_form(main_form),
			_session(session),
			_attempts_remaining(attempts_remaining) {}
		~join_session_form() {}

		bool successful() {
			return _join_successful;
		}
	};

	static std::map<std::string, int> attempts_remaining;

	try {
		if (attempts_remaining.count(session.unique_id) == 0)
			attempts_remaining[session.unique_id] = 3;

		if (attempts_remaining.count(session.unique_id) != 0) {
			if (attempts_remaining[session.unique_id] <= 0) {
				message("Too many invalid login attempts to this session!");
				return false;
			}
		}
	}
	catch (const std::exception&) {}

	std::string error;
	join_session_form fm("Join Session", *this, session, attempts_remaining);
	if (!fm.create(error))
		message(error);

	return fm.successful();
}
