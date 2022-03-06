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

#include "../impl.h"

#include <set>

// serialize template to make collab::user serializable
template<class Archive>
void serialize(Archive& ar, collab::user& cls, const unsigned int version) {
	ar& cls.unique_id;
	ar& cls.username;
	ar& cls.display_name;
	ar& cls.user_image;
}

bool serialize_user_structure(const collab::user& cls, std::string& serialized, std::string& error) {
	error.clear();

	std::stringstream ss;

	try {
		boost::archive::text_oarchive oa(ss);
		oa& cls;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}

	// encode to base64
	serialized = liblec::leccore::base64::encode(ss.str());
	return true;
}

bool deserialize_user_structure(const std::string& serialized, collab::user& cls, std::string& error) {
	std::stringstream ss;

	// decode from base64
	ss << liblec::leccore::base64::decode(serialized);

	try {
		boost::archive::text_iarchive ia(ss);
		ia& cls;
		return true;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}
}

void collab::impl::user_broadcast_sender_func(impl* p_impl) {
	// create a broadcast sender object
	liblec::lecnet::udp::broadcast::sender sender(USER_BROADCAST_PORT);

	// loop until _stop_session_broadcast is false
	while (true) {
		{
			liblec::auto_mutex lock(p_impl->_session_broadcast_mutex);

			// check flag
			if (p_impl->_stop_session_broadcast)
				break;
		}

		std::string error;
		collab::user user;

		// get user from local database
		if (p_impl->_collab.user_exists(p_impl->_collab.unique_id()) && p_impl->_collab.get_user(p_impl->_collab.unique_id(), user, error)) {

			// serialize the user object
			std::string serialized_user;
			if (serialize_user_structure(user, serialized_user, error)) {

				// broadcast the serialized object
				unsigned long actual_count = 0;
				if (sender.send(serialized_user, 1, 0, actual_count, error)) {
					// broadcast successful
				}
			}
		}

		// take a breath
		std::this_thread::sleep_for(std::chrono::milliseconds{ user_broadcast_cycle });
	}
}

void collab::impl::user_broadcast_receiver_func(impl* p_impl) {
	// create broadcast receiver object
	liblec::lecnet::udp::broadcast::receiver receiver(USER_BROADCAST_PORT, "0.0.0.0");

	// for tracking users that have already been received so that a user is not attended to more than once per session
	std::set<std::string> received_users;

	// loop until _stop_session_broadcast is false
	while (true) {
		{
			liblec::auto_mutex lock(p_impl->_session_broadcast_mutex);

			// check flag
			if (p_impl->_stop_session_broadcast)
				break;
		}

		std::string current_session_unique_id;

		{
			liblec::auto_mutex lock(p_impl->_message_broadcast_mutex);
			current_session_unique_id = p_impl->_current_session_unique_id;
		}

		if (!current_session_unique_id.empty()) {
			std::string error;

			// run the receiver
			if (receiver.run(user_receiver_cycle, error)) {
				// loop while running
				while (receiver.running())
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

				// no longer running ... check if a datagram was received
				std::string serialized_user;
				if (receiver.get(serialized_user, error)) {
					// datagram received ... deserialize

					collab::user cls;
					if (deserialize_user_structure(serialized_user, cls, error)) {
						// deserialized successfully

						if (cls.unique_id == p_impl->_collab.unique_id() ||		// check if data is coming from a different node
							received_users.count(cls.unique_id)) {				// don't attend to same user more than once per session
							// ignore this data
						}
						else {
							// check if user has message in current session
							if (!p_impl->_collab.user_has_messages_in_session(cls.unique_id, current_session_unique_id)) {
								// ignore this data
							}
							else {
								// add to received user list
								received_users.insert(cls.unique_id);

								p_impl->_log("New user found (UDP): " + cls.display_name + " '" + cls.username + "' (unique id: " + shorten_unique_id(cls.unique_id) + ")");

								if (p_impl->_collab.user_exists(cls.unique_id)) {
									// edit user
									if (p_impl->_collab.edit_user(cls.unique_id, cls, error)) {
										// user edited successfully
										p_impl->_log("Editing user: '" + shorten_unique_id(cls.unique_id) + "' successful");
									}
									else
										p_impl->_log("Error editing user: '" + shorten_unique_id(cls.unique_id) + "': " + error);
								}
								else {
									// save user to local database
									if (p_impl->_collab.save_user(cls, error)) {
										// user added successfully to the local database
										p_impl->_log("Saving user: '" + shorten_unique_id(cls.unique_id) + "' successful");
									}
									else
										p_impl->_log("Error saving user: '" + shorten_unique_id(cls.unique_id) + "': " + error);
								}
							}
						}
					}
				}
			}

			receiver.stop();
		}
		else {
			// clear received user list so it's refreshed per session
			received_users.clear();

			// take a breath
			std::this_thread::sleep_for(std::chrono::milliseconds{ user_receiver_cycle });
		}
	}
}

bool collab::save_user(const collab::user& user,
	std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

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
		"(UniqueID TEXT NOT NULL, Username TEXT NOT NULL, DisplayName TEXT NOT NULL, UserImage BLOB, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO Users VALUES(?, ?, ?, ?);",
		{ user.unique_id, user.username, user.display_name, liblec::leccore::database::blob{ user.user_image } },
		error))
		return false;

	return true;
}

bool collab::user_exists(const std::string& unique_id) {
	liblec::auto_mutex lock(_d._database_mutex);

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

bool collab::get_user(const std::string& unique_id, collab::user& user, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	user.unique_id.clear();
	user.username.clear();
	user.display_name.clear();
	user.user_image.clear();

	if (unique_id.empty()) {
		error = "User unique id not supplied";
		return false;
	}

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
		for (const auto& row : results.data) {
			if (row.at("UniqueID").has_value())
				user.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Username").has_value())
				user.username = liblec::leccore::database::get::text(row.at("Username"));

			if (row.at("DisplayName").has_value())
				user.display_name = liblec::leccore::database::get::text(row.at("DisplayName"));

			if (row.at("UserImage").has_value())
				user.user_image = liblec::leccore::database::get::blob(row.at("UserImage")).data;

			break;	// only one row expected anyway
		}

		return true;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}
}

bool collab::get_user_display_name(const std::string& unique_id, std::string& display_name, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	display_name.clear();

	if (unique_id.empty()) {
		error = "User unique id not supplied";
		return false;
	}

	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value()) {
		error = "No database connection";
		return false;
	}

	// get database connection object reference
	auto& con = con_opt.value().get();

	liblec::leccore::database::table results;
	if (!con.execute_query("SELECT DisplayName FROM Users WHERE UniqueID = ?;", { unique_id }, results, error))
		return false;

	try {
		for (const auto& row : results.data) {
			if (row.at("DisplayName").has_value())
				display_name = liblec::leccore::database::get::text(row.at("DisplayName"));

			break;	// only one row expected anyway
		}

		return true;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}
}

bool collab::edit_user(const std::string& unique_id, const collab::user& user, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	if (unique_id.empty()) {
		error = "User unique id not supplied";
		return false;
	}

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
		{ user.username, user.display_name, liblec::leccore::database::blob{ user.user_image }, unique_id },
		error))
		return false;

	return true;
}
