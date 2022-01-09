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

#include "collab.h"

// leccore
#include <liblec/leccore/pc_info.h>
#include <liblec/leccore/hash.h>
#include <liblec/leccore/database.h>

#include <optional>

class collab::impl {
	liblec::leccore::database::connection* _p_con;

public:
	impl() : _p_con(nullptr) {}
	~impl() {
		if (_p_con) {
			delete _p_con;
			_p_con = nullptr;
		}
	}

	bool initialize(const std::string& database_file, std::string& error) {
		if (_p_con)
			return true;
		
		_p_con = new liblec::leccore::database::connection("sqlcipher", database_file, "");
		return _p_con->connect(error);
	}

	std::optional<std::reference_wrapper<liblec::leccore::database::connection>> get_connection() {
		return _p_con ? std::optional<std::reference_wrapper<liblec::leccore::database::connection>>{ *_p_con } : std::nullopt;
	}
};

collab::collab() : _d(*new impl()) {
	// make unique id from computer bios serial number
	std::string error;
	liblec::leccore::pc_info _pc_info;
	liblec::leccore::pc_info::pc_details _pc_details;
	if (_pc_info.pc(_pc_details, error)) {
		if (!_pc_details.bios_serial_number.empty())
			_unique_id = _pc_details.bios_serial_number;
	}

	// mask with a hash
	_unique_id = liblec::leccore::hash_string::sha256(_unique_id);
}

collab::~collab() {
	delete& _d;
}

const std::string& collab::unique_id() { return _unique_id; }

bool collab::initialize(const std::string& database_file, std::string& error) {
	return _d.initialize(database_file, error);
}

bool collab::save_user(const std::string& unique_id,
	const std::string& username,
	const std::string& display_name,
	const std::string& user_image,
	std::string& error) {
	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	// create table if it doesn't exist
	if (!con.execute("CREATE TABLE IF NOT EXISTS Users "
		"(UniqueID TEXT, Username TEXT, DisplayName TEXT, UserImage BLOB, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO Users VALUES(?, ?, ?, ?);",
		{ unique_id, username, display_name, liblec::leccore::database::blob{ user_image } },
		error))
		return false;

	return true;
}

bool collab::user_exists(const std::string& unique_id) {
	if (unique_id.empty())
		return false;

	std::string error;

	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	liblec::leccore::database::table results;
	if (!con.execute_query("SELECT * FROM Users WHERE UniqueID = ?;", { unique_id }, results, error))
		return false;

	return !results.data.empty();
}

bool collab::get_user(const std::string& unique_id, std::string& username, std::string& display_name,
	std::string& user_image, std::string& error) {
	username.clear();
	display_name.clear();
	user_image.clear();

	if (unique_id.empty())
		return false;

	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	liblec::leccore::database::table results;
	if (!con.execute_query("SELECT * FROM Users WHERE UniqueID = ?;", { unique_id }, results, error))
		return false;

	try {
		if (results.data[0].at("Username").has_value())
			username = liblec::leccore::database::get::text(results.data[0].at("Username"));

		if (results.data[0].at("DisplayName").has_value())
			display_name = liblec::leccore::database::get::text(results.data[0].at("DisplayName"));

		if (results.data[0].at("UserImage").has_value())
			user_image = liblec::leccore::database::get::blob(results.data[0].at("UserImage")).data;

		return true;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}
}

bool collab::edit_user(const std::string& unique_id, const std::string& username,
	const std::string& display_name, const std::string& user_image, std::string& error) {
	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	// insert data into table
	if (!con.execute("UPDATE Users SET Username = ?, DisplayName = ?, UserImage = ? WHERE UniqueID = ?",
		{ username, display_name, liblec::leccore::database::blob{ user_image }, unique_id },
		error))
		return false;

	return true;
}

bool collab::create_session(session& session, std::string& error) {
	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	// create table if it doesn't exist
	if (!con.execute("CREATE TABLE IF NOT EXISTS Sessions "
		"(UniqueID TEXT, Name TEXT, Description TEXT, PassphraseHash TEXT, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO Sessions VALUES(?, ?, ?, ?);",
		{ session.id, session.name, session.description, session.passphrase_hash },
		error))
		return false;

	return true;
}

bool collab::get_sessions(std::vector<session>& sessions, std::string& error) {
	sessions.clear();

	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	liblec::leccore::database::table results;
	if (!con.execute_query("SELECT UniqueID, Name, Description FROM Sessions;", {}, results, error))
		return false;

	for (auto& row : results.data) {
		collab::session session;

		try {
			if (row.at("UniqueID").has_value())
				session.id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Name").has_value())
				session.name = liblec::leccore::database::get::text(row.at("Name"));

			if (row.at("Description").has_value())
				session.description = liblec::leccore::database::get::text(row.at("Description"));

			sessions.push_back(session);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}
