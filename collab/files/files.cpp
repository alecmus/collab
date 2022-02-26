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

// serialize template to make collab::file serializable
template<class Archive>
void serialize(Archive& ar, collab::file& cls, const unsigned int version) {
	ar& cls.hash;
	ar& cls.time;
	ar& cls.session_id;
	ar& cls.sender_unique_id;
	ar& cls.name;
	ar& cls.extension;
	ar& cls.description;
	ar& cls.size;
}

// serialize template to make file_broadcast_structure serializable
template<class Archive>
void serialize(Archive& ar, file_broadcast_structure& cls, const unsigned int version) {
	ar& cls.source_node_unique_id;
	ar& cls.file_list;
}

bool serialize_file_broadcast_structure(const file_broadcast_structure& cls, std::string& serialized, std::string& error) {
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

bool deserialize_file_broadcast_structure(const std::string& serialized, file_broadcast_structure& cls, std::string& error) {
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

void collab::impl::file_broadcast_sender_func(impl* p_impl) {
	// create a broadcast sender object
	liblec::lecnet::udp::broadcast::sender sender(FILE_BROADCAST_PORT);

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
			std::vector<file> local_file_list;

			// get file list from local database
			if (p_impl->_collab.get_files(current_session_unique_id, local_file_list, error)) {

				// make a file broadcast object
				std::string serialized_file_list;
				file_broadcast_structure cls;
				cls.source_node_unique_id = p_impl->_collab.unique_id();
				cls.file_list = local_file_list;

				// serialize the file broadcast object
				if (serialize_file_broadcast_structure(cls, serialized_file_list, error)) {

					// broadcast the serialized object
					unsigned long actual_count = 0;
					if (sender.send(serialized_file_list, 1, 0, actual_count, error)) {
						// broadcast successful
					}
				}
			}
		}

		// take a breath
		std::this_thread::sleep_for(std::chrono::milliseconds{ file_broadcast_cycle });
	}
}

void collab::impl::file_broadcast_receiver_func(impl* p_impl) {
	// create broadcast receiver object
	liblec::lecnet::udp::broadcast::receiver receiver(FILE_BROADCAST_PORT, "0.0.0.0");

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
			if (receiver.run(file_receiver_cycle, error)) {
				// loop while running
				while (receiver.running())
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

				// no longer running ... check if a datagram was received
				std::string serialized_file_list;
				if (receiver.get(serialized_file_list, error)) {
					// datagram received ... deserialize

					file_broadcast_structure cls;
					if (deserialize_file_broadcast_structure(serialized_file_list, cls, error)) {
						// deserialized successfully

						// check if data is coming from a different node
						if (cls.source_node_unique_id == p_impl->_collab.unique_id())
							continue;	// ignore this data

						std::vector<file> local_file_list;

						// get file list from local database
						if (!p_impl->_collab.get_files(current_session_unique_id, local_file_list, error)) {
							// database may be empty or table may not exist, so ignore
						}

						// check if any file is missing in the local database
						for (const auto& it : cls.file_list) {
							if (it.session_id != current_session_unique_id)
								continue;	// ignore this data

							bool found = false;

							for (const auto& m_it : local_file_list) {
								if (it.hash == m_it.hash) {
									found = true;
									break;
								}
							}

							if (!found) {
								// to-do: download file
								bool downloaded = true;

								if (downloaded) {
									// add this file to the local database
									if (p_impl->_collab.create_file(it, error)) {
										// file added successfully to the local database
									}
								}
							}
						}
					}
				}
			}

			receiver.stop();
		}
		else {
			// take a breath
			std::this_thread::sleep_for(std::chrono::milliseconds{ file_receiver_cycle });
		}
	}
}

bool collab::create_file(const file& file, std::string& error) {
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
	if (!con.execute("CREATE TABLE IF NOT EXISTS SessionFiles "
		"(Hash TEXT NOT NULL, "
		"Time REAL NOT NULL, "
		"SessionID TEXT NOT NULL, "
		"SenderUniqueID TEXT NOT NULL, "
		"Name TEXT NOT NULL, "
		"Extension TEXT NOT NULL, "
		"Description TEXT NOT NULL, "
		"Size REAL NOT NULL, PRIMARY KEY(Hash));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO SessionFiles VALUES(?, ?, ?, ?, ?, ?, ?, ?);",
		{ file.hash, static_cast<double>(file.time), file.session_id, file.sender_unique_id,
		file.name, file.extension, file.description, static_cast<double>(file.size) }, error))
		return false;

	return true;
}

bool collab::get_files(const std::string& session_unique_id, std::vector<file>& files, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	files.clear();

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
		"SELECT Hash, Time, SessionID, SenderUniqueID, Name, Extension, Description, Size "
		"FROM SessionFiles "
		"WHERE SessionID = ? ORDER BY Time DESC;",
		{ session_unique_id }, results, error))
		return false;

	for (auto& row : results.data) {
		collab::file file;

		try {
			if (row.at("Hash").has_value())
				file.hash = liblec::leccore::database::get::text(row.at("Hash"));

			if (row.at("Time").has_value())
				file.time = static_cast<long long>(liblec::leccore::database::get::real(row.at("Time")));

			if (row.at("SessionID").has_value())
				file.session_id = liblec::leccore::database::get::text(row.at("SessionID"));

			if (row.at("SenderUniqueID").has_value())
				file.sender_unique_id = liblec::leccore::database::get::text(row.at("SenderUniqueID"));

			if (row.at("Name").has_value())
				file.name = liblec::leccore::database::get::text(row.at("Name"));

			if (row.at("Extension").has_value())
				file.extension = liblec::leccore::database::get::text(row.at("Extension"));

			if (row.at("Description").has_value())
				file.description = liblec::leccore::database::get::text(row.at("Description"));

			if (row.at("Size").has_value())
				file.size = static_cast<long long>(liblec::leccore::database::get::real(row.at("Size")));

			files.push_back(file);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::user_has_files_in_session(const std::string& user_unique_id, const std::string& session_unique_id) {
	liblec::auto_mutex lock(_d._database_mutex);

	if (user_unique_id.empty() || session_unique_id.empty())
		return false;	// User or Session unique id not supplied

	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value())
		return false;	// No database connection

	// get database connection object reference
	auto& con = con_opt.value().get();

	liblec::leccore::database::table results;

	std::string error;
	if (!con.execute_query(
		"SELECT Time "
		"FROM SessionFiles "
		"WHERE SenderUniqueID = ? AND SessionID = ?;",
		{ user_unique_id, session_unique_id }, results, error))
		return false;

	return !results.data.empty();
}
