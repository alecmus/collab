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

// serialize template to make collab::session serializable
template<class Archive>
void serialize(Archive& ar, collab::session& cls, const unsigned int version) {
	ar& cls.unique_id;
	ar& cls.name;
	ar& cls.description;
	ar& cls.passphrase_hash;
}

// serialize template to make session_broadcast_structure serializable
template<class Archive>
void serialize(Archive& ar, session_broadcast_structure& cls, const unsigned int version) {
	ar& cls.source_node_unique_id;
	ar& cls.session_list;
}

bool serialize_session_broadcast_structure(const session_broadcast_structure& cls,
	std::string& serialized, std::string& error) {
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

bool deserialize_session_broadcast_structure(const std::string& serialized,
	session_broadcast_structure& cls, std::string& error) {
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

void collab::impl::session_broadcast_sender_func(impl* p_impl) {
	// create a broadcast sender object
	liblec::lecnet::udp::broadcast::sender sender(SESSION_BROADCAST_PORT);

	// loop until _stop_session_broadcast is false
	while (true) {
		{
			liblec::auto_mutex lock(p_impl->_session_broadcast_mutex);

			// check flag
			if (p_impl->_stop_session_broadcast)
				break;
		}

		std::string error;
		std::vector<session> local_session_list;

		// get session list from local database
		if (p_impl->_collab.get_local_sessions(local_session_list, error)) {

			// make a session broadcast object
			std::string serialized_session_list;
			session_broadcast_structure cls;
			cls.source_node_unique_id = p_impl->_collab.unique_id();
			cls.session_list = local_session_list;

			// serialize the session broadcast object
			if (serialize_session_broadcast_structure(cls, serialized_session_list, error)) {

				// broadcast the serialized object
				unsigned long actual_count = 0;
				if (sender.send(serialized_session_list, 1, 0, actual_count, error)) {
					// broadcast successful
				}
			}
		}

		// take a breath
		std::this_thread::sleep_for(std::chrono::milliseconds{ session_broadcast_cycle });
	}
}

void collab::impl::session_broadcast_receiver_func(impl* p_impl) {
	// create broadcast receiver object
	liblec::lecnet::udp::broadcast::receiver receiver(SESSION_BROADCAST_PORT, "0.0.0.0");

	// loop until _stop_session_broadcast is false
	while (true) {
		{
			liblec::auto_mutex lock(p_impl->_session_broadcast_mutex);

			// check flag
			if (p_impl->_stop_session_broadcast)
				break;
		}

		std::string error;

		// run the receiver
		if (receiver.run(session_receiver_cycle, error)) {
			// loop while running
			while (receiver.running())
				std::this_thread::sleep_for(std::chrono::milliseconds(1));

			// no longer running ... check if a datagram was received
			std::string serialized_session_list;
			if (receiver.get(serialized_session_list, error)) {
				// datagram received ... deserialize

				session_broadcast_structure cls;
				if (deserialize_session_broadcast_structure(serialized_session_list, cls, error)) {
					// deserialized successfully

					// check if data is coming from a different node
					if (cls.source_node_unique_id == p_impl->_collab.unique_id())
						continue;	// ignore this data

					std::vector<session> local_session_list;

					// get session list from local database
					if (!p_impl->_collab.get_sessions(local_session_list, error)) {
						// database may be empty or table may not exist, so ignore
					}

					// check if any session is missing in the local database
					for (const auto& it : cls.session_list) {
						bool found = false;

						for (const auto& m_it : local_session_list) {
							if (it.unique_id == m_it.unique_id) {
								found = true;
								break;
							}
						}

						if (!found) {
							p_impl->_log("Session received (UDP): '" + it.name + "' (source node: " + shorten_unique_id(cls.source_node_unique_id) + ")");

							// add this session to the local database
							if (p_impl->_collab.create_session(it, error)) {
								// session added successfully to the local database, add it to the temporary session list
								if (p_impl->_collab.create_temporary_session_entry(it.unique_id, error))
									p_impl->_log("Temporary session entry successful for '" + it.name + "'");
								else
									p_impl->_log("Creating temporary session entry for '" + it.name + "' failed: " + error);
							}
							else
								p_impl->_log("Creating session '" + it.name + "' failed: " + error);
						}
					}
				}
			}
		}

		receiver.stop();
	}
}

bool collab::create_session(const session& session, std::string& error) {
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
	if (!con.execute("CREATE TABLE IF NOT EXISTS Sessions "
		"(UniqueID TEXT, Name TEXT, Description TEXT, PassphraseHash TEXT, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO Sessions VALUES(?, ?, ?, ?);",
		{ session.unique_id, session.name, session.description, session.passphrase_hash },
		error))
		return false;

	return true;
}

bool collab::session_exists(const std::string& unique_id) {
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
	if (!con.execute_query("SELECT UniqueID FROM Sessions WHERE UniqueID = ?;", { unique_id }, results, error))
		return false;

	return !results.data.empty();
}

bool collab::remove_session(const std::string& unique_id, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	if (unique_id.empty()) {
		error = "Session unique id not supplied";
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
	if (!con.execute("DELETE FROM Sessions WHERE UniqueID = ?;", { unique_id }, error))
		return false;

	return true;
}

bool collab::get_session(const std::string& unique_id, session& session_info, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	session_info = {};

	if (unique_id.empty()) {
		error = "Session unique id not supplied";
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
	if (!con.execute_query("SELECT UniqueID, Name, Description, PassphraseHash FROM Sessions WHERE UniqueID = ?;", { unique_id }, results, error))
		return false;

	for (auto& row : results.data) {
		try {
			if (row.at("UniqueID").has_value())
				session_info.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Name").has_value())
				session_info.name = liblec::leccore::database::get::text(row.at("Name"));

			if (row.at("Description").has_value())
				session_info.description = liblec::leccore::database::get::text(row.at("Description"));

			if (row.at("PassphraseHash").has_value())
				session_info.passphrase_hash = liblec::leccore::database::get::text(row.at("PassphraseHash"));

			break;	// only one row expected anyway
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::get_sessions(std::vector<session>& sessions, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

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
	if (!con.execute_query("SELECT UniqueID, Name, Description, PassphraseHash FROM Sessions;", {}, results, error))
		return false;

	for (auto& row : results.data) {
		collab::session session;

		try {
			if (row.at("UniqueID").has_value())
				session.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Name").has_value())
				session.name = liblec::leccore::database::get::text(row.at("Name"));

			if (row.at("Description").has_value())
				session.description = liblec::leccore::database::get::text(row.at("Description"));

			if (row.at("PassphraseHash").has_value())
				session.passphrase_hash = liblec::leccore::database::get::text(row.at("PassphraseHash"));

			sessions.push_back(session);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::get_local_sessions(std::vector<session>& sessions, std::string& error) {
	sessions.clear();
	error.clear();
	std::vector<session> all_sessions;
	if (!get_sessions(all_sessions, error))
		return false;

	for (const auto& it : all_sessions) {
		if (!is_temporary_session_entry(it.unique_id))
			sessions.push_back(it);
	}

	return true;
}

bool collab::create_temporary_session_entry(const std::string& unique_id, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	if (unique_id.empty()) {
		error = "Session unique id not supplied";
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

	// create table if it doesn't exist
	if (!con.execute("CREATE TABLE IF NOT EXISTS TemporarySessions "
		"(UniqueID TEXT, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO TemporarySessions VALUES(?);",
		{ unique_id },
		error))
		return false;

	return true;
}

bool collab::is_temporary_session_entry(const std::string& unique_id) {
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
	if (!con.execute_query("SELECT * FROM TemporarySessions WHERE UniqueID = ?;", { unique_id }, results, error))
		return false;

	return !results.data.empty();
}

bool collab::remove_temporary_session_entry(const std::string& unique_id, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	if (unique_id.empty()) {
		error = "Session unique id not supplied";
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
	if (!con.execute("DELETE FROM TemporarySessions WHERE UniqueID = ?;", { unique_id }, error))
		return false;

	return true;
}

void collab::set_current_session_unique_id(const std::string& session_unique_id) {
	liblec::auto_mutex lock(_d._message_broadcast_mutex);
	_d._current_session_unique_id = session_unique_id;
}
