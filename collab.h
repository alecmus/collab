/*
** MIT License
**
** Copyright(c) 2021 Alec Musasa
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files(the "Software"), to deal
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

#include <string>
#include <vector>

/// <summary>Collaboration class.</summary>
class collab {
	std::string _unique_id;

	enum class node_status {
		unknown,
		available,
		unavailable,
		busy,
	};

	struct node {
		std::string id;
		std::string name;
		std::string display_name;
		std::vector<std::string> ip_address_list;
		std::string signed_in_session_id;
		node_status status;
	};
	
	struct token {

	};

public:
	struct session {
		std::string id;
		std::string name;
		std::string description;
		std::string passphrase_hash;
	};

	collab ();
	~collab ();

	const std::string& unique_id();
	
	// users

	bool save_user(const std::string& database_file,
		const std::string& unique_id,
		const std::string& username,
		const std::string& display_name,
		const std::string& user_image,
		std::string& error);

	bool user_exists(const std::string& database_file,
		const std::string& unique_id);

	bool get_user(const std::string& database_file,
		const std::string& unique_id,
		std::string& username,
		std::string& display_name,
		std::string& user_image,
		std::string& error);

	bool edit_user(const std::string& database_file,
		const std::string& unique_id,
		const std::string& username,
		const std::string& display_name,
		const std::string& user_image,
		std::string& error);

	// sessions

	bool create_session(const std::string& database_file,
		session& session,
		std::string& error);

	bool get_sessions(const std::string& database_file,
		std::vector<session>& sessions,
		std::string& error);
};
