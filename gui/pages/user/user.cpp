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
#include <liblec/lecui/containers/pane.h>
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/text_field.h>
#include <liblec/lecui/widgets/button.h>
#include <liblec/lecui/menus/context_menu.h>
#include <liblec/lecui/utilities/filesystem.h>

void main_form::user() {
	class user_form : public form {
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

			// add user image
			auto& user_image = lecui::widgets::image_view::add(home, "user_image");
			user_image
				.png_resource(png_user)
				.rect(lecui::rect()
					.width(128.f)
					.height(128.f)
					.place(ref_rect, 50.f, 0.f)
				)
				.corner_radius_x(128.f / 2.f)
				.corner_radius_y(128.f / 2.f);

			user_image.events().action = [&]() {
				lecui::open_file_params params;
				params
					.title("Select Image")
					.include_all_supported_types(true)
					.allow_multi_select(false)
					.file_types({
					{ "jpg", "JPG Image" },
					{ "jpeg", "JPEG Image" },
					{ "png", "PNG Image" },
					{ "bmp", "Bitmap Image" },
					});

				lecui::filesystem _file_system{ *this };
				std::vector<std::string> file = _file_system.open_file(params);

				if (!file.empty()) {
					try {
						auto& user_image = get_image_view("home/user_image");
						user_image
							.png_resource(0)
							.file(file[0]);

						update();
					}
					catch (const std::exception&) {}
				}
			};
			user_image.events().right_click = [&]() {
				lecui::context_menu::specs menu_specs;
				menu_specs.items.push_back({ "Remove", png_delete });

				auto selected = lecui::context_menu::context_menu()(* this, menu_specs);

				if (selected == "Remove") {
					try {
						auto& user_image = get_image_view("home/user_image");
						user_image
							.png_resource(png_user)
							.file("");

						update();
					}
					catch (const std::exception&) {}
				}
			};

			// add user name caption
			auto& username_caption = lecui::widgets::label::add(home);
			username_caption
				.text("Username")
				.font_size(_caption_font_size)
				.color_text(_caption_color)
				.rect(lecui::rect()
					.left(_margin)
					.width(ref_rect.width())
					.height(_main_form._caption_height)
					.snap_to(user_image.rect(), snap_type::bottom, 3.f * _margin));

			const std::set<char> allowed_set = {
				'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
				'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
				'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
				'.', '-', '_'
			};

			// add user name text field
			auto& username = lecui::widgets::text_field::add(home, "username");
			username
				.prompt("e.g. 'johndoe'")
				.allowed_characters(allowed_set)
				.maximum_length(20)	// to-do: remove magic number
				.rect(lecui::rect(username.rect())
					.width(username_caption.rect().width())
					.snap_to(username_caption.rect(), snap_type::bottom, _margin / 4.f))
				.events().action = [&]() {
				on_save();
			};

			// add display name caption
			auto& display_name_caption = lecui::widgets::label::add(home);
			display_name_caption
				.text("Display Name")
				.font_size(_caption_font_size)
				.color_text(_caption_color)
				.rect(lecui::rect(username_caption.rect())
					.snap_to(username.rect(), snap_type::bottom, _margin));

			// add session description text field
			auto& display_name = lecui::widgets::text_field::add(home, "display_name");
			display_name
				.prompt("e.g. 'John Doe'")
				.maximum_length(100)	// to-do: remove magic number
				.rect(lecui::rect(username.rect())
					.snap_to(display_name_caption.rect(), snap_type::bottom, _margin / 4.f))
				.events().action = [&]() { on_save(); };

			// add save button
			auto& save_user = lecui::widgets::button::add(home, "save_user");
			save_user
				.text("Save")
				.rect(lecui::rect(save_user.rect()).snap_to(display_name.rect(), snap_type::bottom, _margin))
				.events().action = [&]() { on_save(); };

			_page_man.show("home");
			return true;
		}

		void on_save() {
			try {
				auto& username = get_text_field("home/username");

				if (username.text().empty())
					return;

				if (username.text()[0] == '.' ||
					username.text()[0] == '-' ||
					username.text()[0] == '_') {
					message("First character in username can neither be a symbol nor a digit.");
				}

				// to-do: implement user saving
			}
			catch (const std::exception&) {}
		}

	public:
		user_form(const std::string& caption,
			main_form& main_form) :
			form(caption, main_form),
			_main_form(main_form) {}
		~user_form() {}
	};

	user_form fm(std::string(appname) + " - User", *this);
	std::string error;
	if (!fm.create(error))
		message(error);
}
