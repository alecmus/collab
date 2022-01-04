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
#include <liblec/lecui/widgets/text_field.h>
#include <liblec/lecui/widgets/password_field.h>
#include <liblec/lecui/widgets/strength_bar.h>
#include <liblec/lecui/widgets/button.h>

// leccore
#include <liblec/leccore/hash.h>

void main_form::new_session() {
	class new_session_form : public form {
		lecui::controls _ctrls{ *this };
		lecui::page_manager _page_man{ *this };
		lecui::widget_manager _widget_man{ *this };
		lecui::appearance _apprnc{ *this };
		lecui::dimensions _dim{ *this };

		main_form& _main_form;

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
			_dim.set_size(lecui::size().width(300.f).height(340.f));

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
			auto& session_name_caption = lecui::widgets::label::add(home);
			session_name_caption
				.text("Session Name")
				.font_size(_caption_font_size)
				.color_text(_caption_color)
				.rect(lecui::rect()
					.left(_margin)
					.width(ref_rect.width())
					.height(_main_form._caption_height));

			// add session name text field
			auto& session_name = lecui::widgets::text_field::add(home, "session_name");
			session_name
				.prompt("e.g. 'Collab'")
				.maximum_length(40)	// to-do: remove magic number
				.rect(lecui::rect(session_name.rect())
					.width(session_name_caption.rect().width())
					.snap_to(session_name_caption.rect(), snap_type::bottom, _margin / 4.f))
				.events().action = [&]() { on_save(); };

			// add session description caption
			auto& session_description_caption = lecui::widgets::label::add(home);
			session_description_caption
				.text("Session Description")
				.font_size(_caption_font_size)
				.color_text(_caption_color)
				.rect(lecui::rect(session_name_caption.rect())
					.snap_to(session_name.rect(), snap_type::bottom, _margin));

			// add session description text field
			auto& session_description = lecui::widgets::text_field::add(home, "session_description");
			session_description
				.prompt("e.g. 'Group collaboration'")
				.maximum_length(100)	// to-do: remove magic number
				.rect(lecui::rect(session_name.rect())
					.snap_to(session_description_caption.rect(), snap_type::bottom, _margin / 4.f))
				.events().action = [&]() { on_save(); };

			// add session password pane
			auto& session_password_pane = lecui::containers::pane::add(home, "session_password_pane");
			session_password_pane
				.rect()
				.width(session_name.rect().width())
				.height(160.f)
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
					.prompt("Enter a secure password")
					.maximum_length(40)
					.rect(lecui::rect(session_password.rect())
						.width(session_password_caption.rect().width())
						.snap_to(session_password_caption.rect(), snap_type::bottom, _margin / 4.f));
				session_password
					.events().action = [&]() { on_save(); };
				session_password
					.events().change = [&](const std::string& text) {
					const auto quality = leccore::password_quality(text);

					try {
						auto& session_password_strength = get_strength_bar("home/session_password_pane/session_password_strength");
						auto& session_password_strength_caption = get_label("home/session_password_pane/session_password_strength_caption");

						session_password_strength.percentage(quality.strength);
						session_password_strength_caption.text("Password quality " + leccore::round_off::to_string(quality.strength, 1) + "%");
						update();
					}
					catch (const std::exception&) {

					}
				};

				// add confirm password caption
				auto& confirm_session_password_caption = lecui::widgets::label::add(session_password_pane);
				confirm_session_password_caption
					.text("Confirm password")
					.font_size(_caption_font_size)
					.color_text(_caption_color)
					.rect(lecui::rect(session_password_caption.rect()).snap_to(session_password.rect(), snap_type::bottom, _margin));

				// add confirm password field
				auto& confirm_session_password = lecui::widgets::password_field::add(session_password_pane, "confirm_session_password");
				confirm_session_password
					.prompt("Confirm your password")
					.maximum_length(40)
					.rect(lecui::rect(session_password.rect())
						.snap_to(confirm_session_password_caption.rect(), snap_type::bottom, _margin / 4.f))
					.events().action = [&]() { on_save(); };

				// add password strength caption
				auto& session_password_strength_caption = lecui::widgets::label::add(session_password_pane, "session_password_strength_caption");
				session_password_strength_caption
					.text("Password quality")
					.alignment(lecui::text_alignment::center)
					.font_size(_caption_font_size)
					.color_text(_caption_color)
					.rect(lecui::rect(confirm_session_password_caption.rect()).snap_to(confirm_session_password.rect(), snap_type::bottom, _margin));

				// add password strength bar
				auto& session_password_strength = lecui::widgets::strength_bar::add(session_password_pane, "session_password_strength");
				session_password_strength
					.levels({
						{ 50.f, lecui::color().red(255)} ,
						{ 80.f, lecui::color().red(255).green(165) },
						{ 100.f, lecui::color().green(128) } })
					.percentage(0.f)
					.rect()
					.width(confirm_session_password.rect().width())
					.snap_to(session_password_strength_caption.rect(), snap_type::bottom, _margin / 4.f);

				// make bar standout against similar pane background
				if (_main_form._setting_darktheme)
					session_password_strength.color_fill().darken(25.f);
				else
					session_password_strength.color_fill().lighten(50.f);
			}

			// add save button
			auto& save_session = lecui::widgets::button::add(home, "save_session");
			save_session
				.text("Save")
				.rect(lecui::rect(save_session.rect()).snap_to(session_password_pane.rect(), snap_type::bottom, _margin))
				.events().action = [&]() { on_save(); };

			_page_man.show("home");
			return true;
		}

		void on_save() {
			try {
				auto& session_name = get_text_field("home/session_name");
				auto& session_description = get_text_field("home/session_description");
				auto& session_password = get_password_field("home/session_password_pane/session_password");
				auto& confirm_session_password = get_password_field("home/session_password_pane/confirm_session_password");

				if (session_name.text().empty() ||
					session_description.text().empty() ||
					session_password.text().empty() ||
					confirm_session_password.text().empty())
					return;

				if (session_password.text() == confirm_session_password.text()) {
					collab::session session;
					session.id = leccore::hash_string::uuid();
					session.name = session_name.text();
					session.description = session_description.text();
					session.passphrase_hash = leccore::hash_string::sha256(session_password.text());
					
					std::string error;
					if (_main_form._collab.create_session(_main_form._database_file, session, error)) {
						message("Session created successfully!");
						close();
					}
					else
						message(error);
				}
				else
					message("Passwords do not match!");
			}
			catch (const std::exception&) {}
		}

	public:
		new_session_form(const std::string& caption,
			main_form& main_form) :
			form(caption, main_form),
			_main_form(main_form) {}
		~new_session_form() {}
	};

	new_session_form fm(std::string(appname) + " - New Session", *this);
	std::string error;
	if (!fm.create(error))
		message(error);
}
