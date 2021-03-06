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

#include "collab.h"
#include "../helper_functions.h"

// leccore
#include <liblec/leccore/pc_info.h>
#include <liblec/leccore/hash.h>
#include <liblec/leccore/encode.h>
#include <liblec/leccore/database.h>

// lecnet
#include <liblec/lecnet/udp.h>

// STL
#include <optional>
#include <thread>
#include <future>
#include <sstream>

// boost

// for serializing vectors
#include <boost\serialization\vector.hpp>

// include headers that implement a archive in simple text format
#include <boost\archive\text_oarchive.hpp>
#include <boost\archive\text_iarchive.hpp>

enum ports {
	SESSION_BROADCAST_PORT = 30030,
	MESSAGE_BROADCAST_PORT,
	USER_BROADCAST_PORT,
	FILE_BROADCAST_PORT,
	REVIEW_BROADCAST_PORT,
};

enum tcp_ports {
	FILE_TRANSFER_PORT = 55554,
	REVIEW_TRANSFER_PORT,
};

constexpr int file_transfer_magic_number = 173;
constexpr int file_chunk_size = 1024 * 1024;	// the size of each file chunk used in file transfer

constexpr int review_transfer_magic_number = 181;

constexpr int session_broadcast_cycle = 1200;	// in milliseconds
constexpr int session_receiver_cycle = 1500;	// in milliseconds

constexpr int message_broadcast_cycle = 1200;	// in milliseconds
constexpr int message_receiver_cycle = 1500;	// in milliseconds

constexpr int user_broadcast_cycle = 1200;		// in milliseconds
constexpr int user_receiver_cycle = 1500;		// in milliseconds

constexpr int file_broadcast_cycle = 1200;		// in milliseconds
constexpr int file_receiver_cycle = 1500;		// in milliseconds

constexpr int review_broadcast_cycle = 1200;	// in milliseconds
constexpr int review_receiver_cycle = 1500;	// in milliseconds

constexpr int message_broadcast_limit = 10;		// only broadcast the latest 10 messages

struct session_broadcast_structure {
	std::string source_node_unique_id;
	std::vector<collab::session> session_list;
};

bool serialize_session_broadcast_structure(const session_broadcast_structure& cls,
	std::string& serialized, std::string& error);
bool deserialize_session_broadcast_structure(const std::string& serialized,
	session_broadcast_structure& cls, std::string& error);

struct message_broadcast_structure {
	std::string source_node_unique_id;
	std::vector<collab::message> message_list;
};

bool serialize_message_broadcast_structure(const message_broadcast_structure& cls,
	std::string& serialized, std::string& error);
bool deserialize_message_broadcast_structure(const std::string& serialized,
	message_broadcast_structure& cls, std::string& error);

bool serialize_user_structure(const collab::user& cls,
	std::string& serialized, std::string& error);
bool deserialize_user_structure(const std::string& serialized,
	collab::user& cls, std::string& error);

struct file_broadcast_structure {
	std::string source_node_unique_id;
	std::vector<std::string> ips;
	std::vector<collab::file> file_list;
};

bool serialize_file_broadcast_structure(const file_broadcast_structure& cls,
	std::string& serialized, std::string& error);
bool deserialize_file_broadcast_structure(const std::string& serialized,
	file_broadcast_structure& cls, std::string& error);

// same as collab::review except it doesn't contain the review text
// this is important for performance, and also to prevent the review_broadcast_structure from reaching the datagram size limit
struct review_header_structure {
	std::string unique_id;
	long long time;
	std::string session_id;
	std::string file_hash;
	std::string sender_unique_id;
};

struct review_broadcast_structure {
	std::string source_node_unique_id;
	std::vector<std::string> ips;
	std::vector<review_header_structure> review_list;
};

bool serialize_review_broadcast_structure(const review_broadcast_structure& cls,
	std::string& serialized, std::string& error);
bool deserialize_review_broadcast_structure(const std::string& serialized,
	review_broadcast_structure& cls, std::string& error);

class collab::impl {
	liblec::leccore::database::connection* _p_con;
	collab& _collab;
	std::future<void> _session_broadcast_sender;
	std::future<void> _session_broadcast_receiver;
	std::future<void> _message_broadcast_sender;
	std::future<void> _message_broadcast_receiver;
	std::future<void> _user_broadcast_sender;
	std::future<void> _user_broadcast_receiver;
	std::future<void> _file_broadcast_sender;
	std::future<void> _file_broadcast_receiver;
	std::future<void> _review_broadcast_sender;
	std::future<void> _review_broadcast_receiver;
	bool _stop_session_broadcast = false;
	std::string _cert_folder, _files_folder;
	std::function<void(const std::string& event)> _log;

public:
	std::string _unique_id;

	std::string _current_session_unique_id;

	// concurrency control related to the session broadcast thread
	liblec::mutex _session_broadcast_mutex;

	// concurrency control related to the local database
	liblec::mutex _database_mutex;

	// concurrency control related to the message broadcast thread
	liblec::mutex _message_broadcast_mutex;

	// concurrency control related to the file source
	liblec::mutex _file_source_mutex;
	bool _file_source_running = false;

	// concurrency control related to the review source
	liblec::mutex _review_source_mutex;
	bool _review_source_running = false;

	impl(collab& collab);
	~impl();

	bool initialize(const std::string& database_file, const std::string& cert_folder,
		const std::string& files_folder, std::function<void(const std::string&)> log, std::string& error);
	const std::string& cert_folder();
	const std::string& files_folder();

	std::optional<std::reference_wrapper<liblec::leccore::database::connection>> get_connection();

	static void session_broadcast_sender_func(impl* p_impl);
	static void session_broadcast_receiver_func(impl* p_impl);

	static void message_broadcast_sender_func(impl* p_impl);
	static void message_broadcast_receiver_func(impl* p_impl);

	static void user_broadcast_sender_func(impl* p_impl);
	static void user_broadcast_receiver_func(impl* p_impl);

	static void file_broadcast_sender_func(impl* p_impl);
	static void file_broadcast_receiver_func(impl* p_impl);

	static void review_broadcast_sender_func(impl* p_impl);
	static void review_broadcast_receiver_func(impl* p_impl);

	bool file_source_running();
	bool review_source_running();
};
