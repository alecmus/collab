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

// lecnet
#include <liblec/lecnet/tcp.h>

// serialize template to make collab::review serializable
template<class Archive>
void serialize(Archive& ar, collab::review& cls, const unsigned int version) {
	ar& cls.unique_id;
	ar& cls.time;
	ar& cls.session_id;
	ar& cls.file_hash;
	ar& cls.sender_unique_id;
	ar& cls.text;
}

// serialize template to make review_header_structure serializable
template<class Archive>
void serialize(Archive& ar, review_header_structure& cls, const unsigned int version) {
	ar& cls.unique_id;
	ar& cls.time;
	ar& cls.session_id;
	ar& cls.file_hash;
	ar& cls.sender_unique_id;
}

// serialize template to make review_broadcast_structure serializable
template<class Archive>
void serialize(Archive& ar, review_broadcast_structure& cls, const unsigned int version) {
	ar& cls.source_node_unique_id;
	ar& cls.ips;
	ar& cls.review_list;
}

bool serialize_review_broadcast_structure(const review_broadcast_structure& cls,
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

bool deserialize_review_broadcast_structure(const std::string& serialized,
	review_broadcast_structure& cls, std::string& error) {
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

class review_source : public liblec::lecnet::tcp::server_async_ssl {
	collab& _collab;

public:
	review_source(collab& collab) :
		_collab(collab) {}

private:
	// overrides
	void log(const std::string& time_stamp, const std::string& event) override {}
	std::string on_receive(const client_address& address, const std::string& data_received) override {
		return on_receive(data_received);
	}

	// overload
	// datareceived is simply the review unique id
	// data returned is simply the review text
	std::string on_receive(const std::string& review_unique_id) {
		collab::review review;

		std::string error;
		if (!_collab.get_review(review_unique_id, review, error))
			return std::string();	// return empty string. to-do: use a structure to return error back to sink
		else
			return review.text;
	}
};

void collab::impl::review_broadcast_sender_func(impl* p_impl) {
	// create a review source object
	liblec::lecnet::tcp::server::server_params params;
	params.port = REVIEW_TRANSFER_PORT;
	params.magic_number = review_transfer_magic_number;
	params.max_clients = 1;
	params.server_cert = p_impl->cert_folder() + "\\collab.source";
	params.server_cert_key = p_impl->cert_folder() + "\\collab.source";
	params.server_cert_key_password = "com.github.alecmus.collab.source";

	review_source source(p_impl->_collab);

	// start the source
	if (!source.start(params)) {
		// I mean, why would it fail?
	}

	while (source.starting())
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	if (source.running()) {
		p_impl->_log("Review source started");

		{
			liblec::auto_mutex lock(p_impl->_review_source_mutex);
			p_impl->_review_source_running = true;
		}

		// create a broadcast sender object
		liblec::lecnet::udp::broadcast::sender sender(REVIEW_BROADCAST_PORT);

		// loop until _stop_session_broadcast is false
		while (source.running()) {
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
				std::vector<review> local_review_list;

				// get review list from local database
				if (p_impl->_collab.get_reviews(current_session_unique_id, local_review_list, error)) {

					// make a revies broadcast object
					std::string serialized_review_list;
					review_broadcast_structure cls;

					// capture source node unique id
					cls.source_node_unique_id = p_impl->_collab.unique_id();

					// capture host ip addresses
					liblec::lecnet::tcp::get_host_ips(cls.ips);

					// capture local review header (excludes review text)
					cls.review_list.reserve(local_review_list.size());

					for (const auto& review : local_review_list) {
						review_header_structure header;
						header.unique_id = review.unique_id;
						header.session_id = review.session_id;
						header.time = review.time;
						header.file_hash = review.file_hash;
						header.sender_unique_id = review.sender_unique_id;

						cls.review_list.push_back(header);
					}

					// serialize the review broadcast object
					if (serialize_review_broadcast_structure(cls, serialized_review_list, error)) {

						// broadcast the serialized object
						unsigned long actual_count = 0;
						if (sender.send(serialized_review_list, 1, 0, actual_count, error)) {
							// broadcast successful
						}
					}
				}
			}

			// take a breath
			std::this_thread::sleep_for(std::chrono::milliseconds{ review_broadcast_cycle });
		}

		bool stopped_by_request = false;

		{
			liblec::auto_mutex lock(p_impl->_session_broadcast_mutex);
			stopped_by_request = p_impl->_stop_session_broadcast == true;
		}

		if (!stopped_by_request)
			p_impl->_log("Error: review source stopped");
	}
	else {
		p_impl->_log("Error: review source failed to start");

		liblec::auto_mutex lock(p_impl->_review_source_mutex);
		p_impl->_review_source_running = false;
	}

	// check if the source is running
	if (source.running()) {
		// close all connections
		source.close();

		// stop the source
		source.stop();
	}

	{
		liblec::auto_mutex lock(p_impl->_review_source_mutex);
		p_impl->_review_source_running = false;
	}
}

void collab::impl::review_broadcast_receiver_func(impl* p_impl) {
	// create broadcast receiver object
	liblec::lecnet::udp::broadcast::receiver receiver(REVIEW_BROADCAST_PORT, "0.0.0.0");

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
			if (receiver.run(review_receiver_cycle, error)) {
				// loop while running
				while (receiver.running())
					std::this_thread::sleep_for(std::chrono::milliseconds(1));

				// no longer running ... check if a datagram was received
				std::string serialized_review_list;
				if (receiver.get(serialized_review_list, error)) {
					// datagram received ... deserialize

					review_broadcast_structure cls;
					if (deserialize_review_broadcast_structure(serialized_review_list, cls, error)) {
						// deserialized successfully

						// check if data is coming from a different node
						if (cls.source_node_unique_id == p_impl->_collab.unique_id())
							continue;	// ignore this data

						// check if any review is missing in the local database
						for (const auto& it : cls.review_list) {
							if (it.session_id != current_session_unique_id)
								continue;	// ignore this data, it's for another session

							// check if review exists in the session (local database)
							if (!p_impl->_collab.review_exists(it.unique_id)) {
								p_impl->_log("New review found (UDP): '" + shorten_unique_id(it.unique_id) + "' (source node: " + shorten_unique_id(cls.source_node_unique_id) + ")");

								bool downloaded = false;	// flag to determine if review text has been downloaded
								std::string text;

								// get sink IP list
								std::vector<std::string> ips_client;
								liblec::lecnet::tcp::get_host_ips(ips_client);

								// select the ip to connect to
								const std::string selected_ip = select_ip(cls.ips, ips_client);

								// configure tcp/ip sink parameters
								liblec::lecnet::tcp::client::client_params params;
								params.address = selected_ip;
								params.port = REVIEW_TRANSFER_PORT;
								params.magic_number = review_transfer_magic_number;
								params.use_ssl = true;
								params.ca_cert_path = p_impl->cert_folder() + "\\collab.sink";

								// create tcp/ip sink object
								liblec::lecnet::tcp::client sink;

								if (sink.connect(params, error)) {
									while (sink.connecting())
										std::this_thread::sleep_for(std::chrono::milliseconds(1));

									if (sink.connected(error)) {
										// review unique_id

										p_impl->_log("Connected via TCP to " + selected_ip + " to download review '" + shorten_unique_id(it.unique_id));

										if (sink.send_data(it.unique_id, text, 10, nullptr, error)) {
											downloaded = true;
										}
										else {
											p_impl->_log("Error downloading review'" + shorten_unique_id(it.unique_id) + "': " + error);
											break;
										}

										// disconnect tcp sink
										sink.disconnect();
									}
									else
										p_impl->_log("TCP connection for downloading review '" + shorten_unique_id(it.unique_id) + "' from " + selected_ip + " failed: " + error);
								}
								else
									p_impl->_log("TCP connection for downloading review '" + shorten_unique_id(it.unique_id) + "' from " + selected_ip + " failed: " + error);

								if (downloaded) {
									// add this review to the local database (full review including text)
									collab::review review;

									// clone details from review broadcast structure
									review.unique_id = it.unique_id;
									review.time = it.time;
									review.session_id = it.session_id;
									review.file_hash = it.file_hash;
									review.sender_unique_id = it.sender_unique_id;

									// add the text downloaded via TCP to make a complete review structure
									review.text = text;

									if (p_impl->_collab.create_review(review, error)) {
										// review added successfully to the local database
										p_impl->_log("Review '" + shorten_unique_id(it.unique_id) + "' saved successfully");
									}
									else
										p_impl->_log("Entry failed for review '" + shorten_unique_id(it.unique_id) + "': " + error);
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
			std::this_thread::sleep_for(std::chrono::milliseconds{ review_receiver_cycle });
		}
	}
}

bool collab::impl::review_source_running() {
	liblec::auto_mutex lock(_review_source_mutex);
	return _review_source_running;
}

bool collab::create_review(const review& review, std::string& error) {
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
	if (!con.execute("CREATE TABLE IF NOT EXISTS FileReviews "
		"(UniqueID TEXT NOT NULL, "
		"Time REAL NOT NULL, "
		"SessionID TEXT NOT NULL, "
		"FileHash TEXT NOT NULL, "
		"SenderUniqueID TEXT NOT NULL, "
		"Text TEXT NOT NULL, PRIMARY KEY(UniqueID));",
		{}, error))
		return false;

	// insert data into table
	if (!con.execute("INSERT INTO FileReviews VALUES(?, ?, ?, ?, ?, ?);",
		{ review.unique_id, static_cast<double>(review.time), review.session_id, review.file_hash, review.sender_unique_id,
		review.text }, error))
		return false;

	return true;
}

bool collab::get_reviews(const std::string& session_unique_id,
	std::vector<review>& reviews, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	reviews.clear();

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
		"SELECT UniqueID, Time, SessionID, FileHash, SenderUniqueID, Text "
		"FROM FileReviews "
		"WHERE SessionID = ? ORDER BY Time DESC;",
		{ session_unique_id }, results, error))
		return false;

	for (auto& row : results.data) {
		collab::review review;

		try {
			if (row.at("UniqueID").has_value())
				review.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Time").has_value())
				review.time = static_cast<long long>(liblec::leccore::database::get::real(row.at("Time")));

			if (row.at("SessionID").has_value())
				review.session_id = liblec::leccore::database::get::text(row.at("SessionID"));

			if (row.at("FileHash").has_value())
				review.file_hash = liblec::leccore::database::get::text(row.at("FileHash"));

			if (row.at("SenderUniqueID").has_value())
				review.sender_unique_id = liblec::leccore::database::get::text(row.at("SenderUniqueID"));

			if (row.at("Text").has_value())
				review.text = liblec::leccore::database::get::text(row.at("Text"));

			reviews.push_back(review);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::get_reviews(const std::string& session_unique_id, const std::string& file_hash, std::vector<review>& reviews, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	reviews.clear();

	if (session_unique_id.empty() || file_hash.empty()) {
		error = "Session unique id or file hash not supplied";
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
		"SELECT UniqueID, Time, SessionID, FileHash, SenderUniqueID, Text "
		"FROM FileReviews "
		"WHERE SessionID = ? AND FileHash = ? "
		"ORDER BY Time DESC;",
		{ session_unique_id, file_hash }, results, error))
		return false;

	for (auto& row : results.data) {
		collab::review review;

		try {
			if (row.at("UniqueID").has_value())
				review.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Time").has_value())
				review.time = static_cast<long long>(liblec::leccore::database::get::real(row.at("Time")));

			if (row.at("SessionID").has_value())
				review.session_id = liblec::leccore::database::get::text(row.at("SessionID"));

			if (row.at("FileHash").has_value())
				review.file_hash = liblec::leccore::database::get::text(row.at("FileHash"));

			if (row.at("SenderUniqueID").has_value())
				review.sender_unique_id = liblec::leccore::database::get::text(row.at("SenderUniqueID"));

			if (row.at("Text").has_value())
				review.text = liblec::leccore::database::get::text(row.at("Text"));

			reviews.push_back(review);
		}
		catch (const std::exception& e) {
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::get_review(const std::string& unique_id, review& review, std::string& error) {
	liblec::auto_mutex lock(_d._database_mutex);

	review = {};

	if (unique_id.empty()) {
		error = "Review unique id not supplied";
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
		"SELECT UniqueID, Time, SessionID, FileHash, SenderUniqueID, Text "
		"FROM FileReviews "
		"WHERE UniqueID = ?;",
		{ unique_id }, results, error))
		return false;

	for (auto& row : results.data) {
		try {
			if (row.at("UniqueID").has_value())
				review.unique_id = liblec::leccore::database::get::text(row.at("UniqueID"));

			if (row.at("Time").has_value())
				review.time = static_cast<long long>(liblec::leccore::database::get::real(row.at("Time")));

			if (row.at("SessionID").has_value())
				review.session_id = liblec::leccore::database::get::text(row.at("SessionID"));

			if (row.at("FileHash").has_value())
				review.file_hash = liblec::leccore::database::get::text(row.at("FileHash"));

			if (row.at("SenderUniqueID").has_value())
				review.sender_unique_id = liblec::leccore::database::get::text(row.at("SenderUniqueID"));

			if (row.at("Text").has_value())
				review.text = liblec::leccore::database::get::text(row.at("Text"));

			break;	// only one row expected anyway
		}
		catch (const std::exception& e) {
			review = {};
			error = e.what();
			return false;
		}
	}

	return true;
}

bool collab::review_exists(const std::string& unique_id) {
	liblec::auto_mutex lock(_d._database_mutex);

	if (unique_id.empty())
		return false;	// Unique id not supplied

	// get optional object
	auto con_opt = _d.get_connection();

	if (!con_opt.has_value())
		return false;	// No database connection

	// get database connection object reference
	auto& con = con_opt.value().get();

	liblec::leccore::database::table results;

	std::string error;
	if (!con.execute_query(
		"SELECT UniqueID "
		"FROM FileReviews "
		"WHERE UniqueID = ?;",
		{ unique_id }, results, error))
		return false;

	return !results.data.empty();
}
