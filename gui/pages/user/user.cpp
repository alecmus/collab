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
#include <liblec/lecui/widgets/image_view.h>
#include <liblec/lecui/widgets/label.h>
#include <liblec/lecui/widgets/text_field.h>
#include <liblec/lecui/widgets/button.h>
#include <liblec/lecui/menus/context_menu.h>
#include <liblec/lecui/utilities/filesystem.h>

// leccore
#include <liblec/leccore/image.h>
#include <liblec/leccore/system.h>

void main_form::user() {
	class user_form : public form {
		lecui::controls _ctrls{ *this };
		lecui::page_manager _page_man{ *this };
		lecui::widget_manager _widget_man{ *this };
		lecui::appearance _apprnc{ *this };
		lecui::dimensions _dim{ *this };

		main_form& _main_form;
		std::string _resized_profile_image;
		const bool _editing_mode = false;

		std::string _existing_username;
		std::string _existing_display_name;
		std::string _existing_user_image;

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

			if (_editing_mode) {
				if (!_main_form._collab.get_user(_main_form._database_file, _main_form._collab.unique_id(),
					_existing_username, _existing_display_name, _existing_user_image, error))
					return false;

				if (!_existing_user_image.empty()) {
					std::string fullpath = _main_form._folder + "\\rpi.jpg";
					if (!leccore::file::write(fullpath, _existing_user_image, error))
						return false;

					_resized_profile_image = fullpath;
				}
			}

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
				.png_resource(_resized_profile_image.empty() ? png_user : 0)
				.file(_resized_profile_image)
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
					// get the full path to the file
					std::string full_path = file[0];

					// save the image to a temporary file of size 256x256 pixels
					leccore::image image(full_path);

					// a simple name for the file
					_resized_profile_image = _main_form._folder + "\\rpi";	// rpi for resized profile image

					leccore::image::image_options options;
					options
						.size(leccore::size().width(256.f).height(256.f))
						.crop(true)
						.keep_aspect_ratio(true)
						.format(leccore::image::format::jpg)
						.quality(leccore::image_quality::medium);

					try {
						auto& user_image = get_image_view("home/user_image");
						user_image
							.png_resource(png_user)
							.file("");

						// this will cause the image file to be freed so we can overwrite it
						if (!_widget_man.refresh("home/user_image", error)) {}
					}
					catch (const std::exception&) {}

					// overwrite the image file (or create a new one if it doesn't exist)
					if (!image.save(_resized_profile_image, options, error)) {
						message("Error optimizing profile image: " + error);
					}
					else {
						try {
							// change the image
							auto& user_image = get_image_view("home/user_image");
							user_image
								.png_resource(0)
								.file(_resized_profile_image);

							update();
						}
						catch (const std::exception&) {}
					}
				}
			};
			user_image.events().right_click = [&]() {
				lecui::context_menu::specs menu_specs;
				menu_specs.items.push_back({ "Remove", png_delete });

				auto selected = lecui::context_menu::context_menu()(*this, menu_specs);

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
				.text(_existing_username)
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
				.text(_existing_display_name)
				.prompt("e.g. 'John Doe'")
				.maximum_length(100)	// to-do: remove magic number
				.rect(lecui::rect(username.rect())
					.snap_to(display_name_caption.rect(), snap_type::bottom, _margin / 4.f))
				.events().action = [&]() { on_save(); };

			// add save button
			auto& save_user = lecui::widgets::button::add(home, "save_user");
			save_user
				.text(_editing_mode ? "Save Changes" : "Save")
				.rect(lecui::rect(save_user.rect()).width(90.f).snap_to(display_name.rect(), snap_type::bottom, _margin))
				.events().action = [&]() { on_save(); };

			_page_man.show("home");
			return true;
		}

		void on_save() {
			try {
				auto& username = get_text_field("home/username");
				auto& display_name = get_text_field("home/display_name");
				auto& user_image = get_image_view("home/user_image");

				if (username.text().empty() || display_name.text().empty())
					return;

				if (username.text()[0] == '.' ||
					username.text()[0] == '-' ||
					username.text()[0] == '_') {
					message("First character in username can neither be a symbol nor a digit.");
				}

				// read user_image data
				std::string error;
				std::string user_image_data;

				if (!user_image.png_resource() && !user_image.file().empty()) {
					if (!leccore::file::read(_resized_profile_image, user_image_data, error)) {
						message("Failed to read user image: " + error);
						return;
					}
				}

				if (_editing_mode) {
					// edit existing user
					if (!_main_form._collab.edit_user(_main_form._database_file, _main_form._collab.unique_id(),
						username.text(), display_name.text(), user_image_data, error)) {
						message(error);
						return;
					}
				}
				else {
					// save new user
					if (!_main_form._collab.save_user(_main_form._database_file, _main_form._collab.unique_id(),
						username.text(), display_name.text(), user_image_data, error)) {
						message(error);
						return;
					}
				}

				_main_form.set_user_image_icon(user_image_data);

				close();
			}
			catch (const std::exception&) {}
		}

		void on_shutdown() override {
			try {
				// remove the resized profile image so we can be able to delete it in the destructor
				auto& user_image = get_image_view("home/user_image");
				user_image
					.png_resource(png_user)
					.file("");

				std::string error;
				if (!_widget_man.refresh("home/user_image", error)) {}
			}
			catch (const std::exception&) {}
		}

	public:
		user_form(const std::string& caption,
			main_form& main_form, const bool& editing_mode) :
			form(caption, main_form),
			_main_form(main_form),
			_editing_mode(editing_mode) {}
		~user_form() {
			// delete resized profile image
			std::string error;
			if (!leccore::file::remove(_resized_profile_image, error)) {}
		}
	};

	const bool editing_mode = _collab.user_exists(_database_file, _collab.unique_id());

	user_form fm(editing_mode ? std::string(appname) + " - Edit User" : std::string(appname) + " - User", *this, editing_mode);
	std::string error;
	if (!fm.create(error))
		message(error);
}
