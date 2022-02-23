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
#include "helper_functions.h"

// leccore
#include <liblec/leccore/pc_info.h>
#include <liblec/leccore/hash.h>
#include <liblec/leccore/encode.h>
#include <liblec/leccore/database.h>

// lecnet
#include <liblec/lecnet/udp.h>

// boost

// for serializing vectors
#include <boost\serialization\vector.hpp>

// include headers that implement a archive in simple text format
#include <boost\archive\text_oarchive.hpp>
#include <boost\archive\text_iarchive.hpp>

// STL
#include <optional>
#include <thread>
#include <future>
#include <sstream>

enum ports {
	SESSION_BROADCAST_PORT = 30030,
	MESSAGE_BROADCAST_PORT
};

constexpr int session_broadcast_cycle = 1200;	// in milliseconds
constexpr int session_receiver_cycle = 1500;	// in milliseconds

constexpr int message_broadcast_cycle = 1200;	// in milliseconds
constexpr int message_receiver_cycle = 1500;	// in milliseconds

constexpr int message_broadcast_limit = 10;		// only broadcast the latest 10 messages

struct session_broadcast_structure {
	std::string source_node_unique_id;
	std::vector<collab::session> session_list;
};

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

struct message_broadcast_structure {
	std::string source_node_unique_id;
	std::vector<collab::message> message_list;
};

// serialize template to make collab::message serializable
template<class Archive>
void serialize(Archive& ar, collab::message& cls, const unsigned int version) {
	ar& cls.unique_id;
	ar& cls.time;
	ar& cls.session_id;
	ar& cls.sender_unique_id;
	ar& cls.text;
}

// serialize template to make message_broadcast_structure serializable
template<class Archive>
void serialize(Archive& ar, message_broadcast_structure& cls, const unsigned int version) {
	ar& cls.source_node_unique_id;
	ar& cls.message_list;
}

bool serialize_message_broadcast_structure(const message_broadcast_structure& cls,
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

bool deserialize_message_broadcast_structure(const std::string& serialized,
	message_broadcast_structure& cls, std::string& error) {
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

class collab::impl {
	liblec::leccore::database::connection* _p_con;
	collab& _collab;
	std::future<void> _session_broadcast_sender;
	std::future<void> _session_broadcast_receiver;
	std::future<void> _message_broadcast_sender;
	std::future<void> _message_broadcast_receiver;
	bool _stop_session_broadcast = false;

public:
	// concurrency control related to the session broadcast thread
	liblec::mutex _session_broadcast_mutex;

	// concurrency control related to the local database
	liblec::mutex _database_mutex;

	// concurrency control related to the message broadcast thread
	liblec::mutex _message_broadcast_mutex;
	std::string _current_session_unique_id;

	impl(collab& collab) :
		_p_con(nullptr),
		_collab(collab) {}
	~impl() {
		// stop session broadcast thread
		{
			liblec::auto_mutex lock(_session_broadcast_mutex);
			_stop_session_broadcast = true;
		}

		if (_session_broadcast_sender.valid()) {
			// wait for the thread to exit
			while (true) {
				bool thread_running = _session_broadcast_sender.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

				if (!thread_running)
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}
		}

		if (_session_broadcast_receiver.valid()) {
			// wait for the thread to exit
			while (true) {
				bool thread_running = _session_broadcast_receiver.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

				if (!thread_running)
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}
		}

		if (_message_broadcast_sender.valid()) {
			// wait for the thread to exit
			while (true) {
				bool thread_running = _message_broadcast_sender.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

				if (!thread_running)
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}
		}

		if (_message_broadcast_receiver.valid()) {
			// wait for the thread to exit
			while (true) {
				bool thread_running = _message_broadcast_receiver.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

				if (!thread_running)
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}
		}

		if (_p_con) {
			delete _p_con;
			_p_con = nullptr;
		}
	}

	bool initialize(const std::string& database_file, std::string& error) {
		if (_p_con)
			return true;
		
		// make database connection object
		_p_con = new liblec::leccore::database::connection("sqlcipher", database_file, "");

		// connect to the database
		if (!_p_con->connect(error))
			return false;

		// remove all temporary sessions from the local database
		std::vector<session> sessions;

		if (_collab.get_sessions(sessions, error)) {}

		for (const auto& it : sessions) {
			if (_collab.is_temporary_session_entry(it.unique_id)) {
				// remove session
				if (_collab.remove_session(it.unique_id, error)) {}

				// remove from temporary session list
				if (_collab.remove_temporary_session_entry(it.unique_id, error)) {}
			}
		}

		// start threads
		try {
			_session_broadcast_sender = std::async(std::launch::async, session_broadcast_sender_func, this);
			_session_broadcast_receiver = std::async(std::launch::async, session_broadcast_receiver_func, this);
			_message_broadcast_sender = std::async(std::launch::async, message_broadcast_sender_func, this);
			_message_broadcast_receiver = std::async(std::launch::async, message_broadcast_receiver_func, this);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}

		return true;
	}

	std::optional<std::reference_wrapper<liblec::leccore::database::connection>> get_connection() {
		return _p_con ? std::optional<std::reference_wrapper<liblec::leccore::database::connection>>{ *_p_con } : std::nullopt;
	}

	/// <summary>Session broadcast sender function.</summary>
	/// <param name="p_impl">A pointer to the implementation object.</param>
	/// <remarks>Run on a different thread.</remarks>
	static void session_broadcast_sender_func(impl* p_impl) {
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

	static void session_broadcast_receiver_func(impl* p_impl) {
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
								// add this session to the local database
								if (p_impl->_collab.create_session(it, error)) {
									// session added successfully to the local database, add it to the temporary session list
									if (p_impl->_collab.create_temporary_session_entry(it.unique_id, error)) {}
								}
							}
						}
					}
				}
			}

			receiver.stop();
		}
	}

	/// <summary>Message message broadcast sender function.</summary>
	/// <param name="p_impl">A pointer to the implementation object.</param>
	/// <remarks>Run on a different thread.</remarks>
	static void message_broadcast_sender_func(impl* p_impl) {
		// create a broadcast sender object
		liblec::lecnet::udp::broadcast::sender sender(MESSAGE_BROADCAST_PORT);

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

			std::string error;
			std::vector<message> local_message_list;

			// get message list from local database
			if (p_impl->_collab.get_latest_messages(current_session_unique_id, local_message_list, message_broadcast_limit, error)) {

				// make a message broadcast object
				std::string serialized_message_list;
				message_broadcast_structure cls;
				cls.source_node_unique_id = p_impl->_collab.unique_id();
				cls.message_list = local_message_list;

				// serialize the message broadcast object
				if (serialize_message_broadcast_structure(cls, serialized_message_list, error)) {

					// broadcast the serialized object
					unsigned long actual_count = 0;
					if (sender.send(serialized_message_list, 1, 0, actual_count, error)) {
						// broadcast successful
					}
				}
			}

			// take a breath
			std::this_thread::sleep_for(std::chrono::milliseconds{ message_broadcast_cycle });
		}
	}

	static void message_broadcast_receiver_func(impl* p_impl) {
		// create broadcast receiver object
		liblec::lecnet::udp::broadcast::receiver receiver(MESSAGE_BROADCAST_PORT, "0.0.0.0");

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

			std::string error;

			// run the receiver
			if (receiver.run(message_receiver_cycle, error)) {
				// loop while running
				while (receiver.running())
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

				// no longer running ... check if a datagram was received
				std::string serialized_message_list;
				if (receiver.get(serialized_message_list, error)) {
					// datagram received ... deserialize

					message_broadcast_structure cls;
					if (deserialize_message_broadcast_structure(serialized_message_list, cls, error)) {
						// deserialized successfully

						// check if data is coming from a different node
						if (cls.source_node_unique_id == p_impl->_collab.unique_id())
							continue;	// ignore this data

						std::vector<message> local_message_list;

						// get message list from local database
						if (!p_impl->_collab.get_messages(current_session_unique_id, local_message_list, error)) {
							// database may be empty or table may not exist, so ignore
						}

						// check if any message is missing in the local database
						for (const auto& it : cls.message_list) {
							bool found = false;

							for (const auto& m_it : local_message_list) {
								if (it.unique_id == m_it.unique_id) {
									found = true;
									break;
								}
							}

							if (!found) {
								// add this message to the local database
								if (p_impl->_collab.create_message(it, error)) {
									// message added successfully to the local database
								}
							}
						}
					}
				}
			}

			receiver.stop();
		}
	}
};

collab::collab() : _d(*new impl(*this)) {
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

bool collab::get_user(const std::string& unique_id, std::string& username, std::string& display_name,
	std::string& user_image, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	username.clear();
	display_name.clear();
	user_image.clear();

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
		{ username, display_name, liblec::leccore::database::blob{ user_image }, unique_id },
		error))
		return false;

	return true;
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
	if (!con.execute_query("SELECT * FROM Sessions WHERE UniqueID = ?;", { unique_id }, results, error))
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

bool collab::create_message(const message& message, std::string& error) {
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
	if (!con.execute("CREATE TABLE IF NOT EXISTS SessionMessages "
		"(UniqueID TEXT NOT NULL, "
		"Time REAL NOT NULL, "
		"SessionID TEXT NOT NULL, "
		"SenderUniqueID TEXT NOT NULL, "
		"Message TEXT NOT NULL, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO SessionMessages VALUES(?, ?, ?, ?, ?);",
		{ message.unique_id, static_cast<double>(message.time), message.session_id, message.sender_unique_id, message.text },
		error))
		return false;

	return true;
}

bool collab::get_messages(const std::string& session_unique_id,
	std::vector<message>& messages, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	messages.clear();

	if (session_unique_id.empty()) {
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

	if (!con.execute_query(
		"SELECT UniqueID, Time, SessionID, SenderUniqueID, Message "
		"FROM SessionMessages WHERE SessionID = ? ORDER BY Time ASC;",
		{ session_unique_id }, results, error))
		return false;

	for (auto& row : results.data) {
		collab::message msg;

		try {
			if (row.at("UniqueID").has_value())
				msg.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Time").has_value())
				msg.time = static_cast<long long>(liblec::leccore::database::get::real(row.at("Time")));

			if (row.at("SessionID").has_value())
				msg.session_id = liblec::leccore::database::get::text(row.at("SessionID"));

			if (row.at("SenderUniqueID").has_value())
				msg.sender_unique_id = liblec::leccore::database::get::text(row.at("SenderUniqueID"));

			if (row.at("Message").has_value())
				msg.text = liblec::leccore::database::get::text(row.at("Message"));

			messages.push_back(msg);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::get_latest_messages(const std::string& session_unique_id, std::vector<message>& messages,
	int number, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	messages.clear();

	if (session_unique_id.empty()) {
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

	if (number > 0) {
		if (!con.execute_query(
			"SELECT UniqueID, Time, SessionID, SenderUniqueID, Message "
			"FROM SessionMessages WHERE SessionID = ? "
			"ORDER BY Time DESC "
			"LIMIT ?;",
			{ session_unique_id, number }, results, error))
			return false;
	}
	else {
		if (!con.execute_query(
			"SELECT UniqueID, Time, SessionID, SenderUniqueID, Message "
			"FROM SessionMessages WHERE SessionID = ? ORDER BY Time DESC;",
			{ session_unique_id }, results, error))
			return false;
	}

	for (auto& row : results.data) {
		collab::message msg;

		try {
			if (row.at("UniqueID").has_value())
				msg.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Time").has_value())
				msg.time = static_cast<long long>(liblec::leccore::database::get::real(row.at("Time")));

			if (row.at("SessionID").has_value())
				msg.session_id = liblec::leccore::database::get::text(row.at("SessionID"));

			if (row.at("SenderUniqueID").has_value())
				msg.sender_unique_id = liblec::leccore::database::get::text(row.at("SenderUniqueID"));

			if (row.at("Message").has_value())
				msg.text = liblec::leccore::database::get::text(row.at("Message"));

			messages.push_back(msg);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}
