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
#include "impl.h"
#include "../helper_functions.h"

collab::impl::impl(collab& collab) :
	_p_con(nullptr),
	_collab(collab) {}

collab::impl::~impl() {
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

	if (_user_broadcast_sender.valid()) {
		// wait for the thread to exit
		while (true) {
			bool thread_running = _user_broadcast_sender.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

			if (!thread_running)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		}
	}

	if (_user_broadcast_receiver.valid()) {
		// wait for the thread to exit
		while (true) {
			bool thread_running = _user_broadcast_receiver.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

			if (!thread_running)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		}
	}

	if (_file_broadcast_sender.valid()) {
		// wait for the thread to exit
		while (true) {
			bool thread_running = _file_broadcast_sender.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

			if (!thread_running)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		}
	}

	if (_file_broadcast_receiver.valid()) {
		// wait for the thread to exit
		while (true) {
			bool thread_running = _file_broadcast_receiver.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

			if (!thread_running)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		}
	}

	if (_review_broadcast_sender.valid()) {
		// wait for the thread to exit
		while (true) {
			bool thread_running = _review_broadcast_sender.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

			if (!thread_running)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		}
	}

	if (_review_broadcast_receiver.valid()) {
		// wait for the thread to exit
		while (true) {
			bool thread_running = _review_broadcast_receiver.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;

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

bool collab::impl::initialize(const std::string& database_file, const std::string& cert_folder,
	const std::string& files_folder, std::function<void(const std::string&)> log, std::string& error) {
	if (_p_con)
		return true;

	_cert_folder = cert_folder;
	_files_folder = files_folder;
	_log = log;

	// make database connection object
	_p_con = new liblec::leccore::database::connection("sqlcipher",
		database_file, liblec::leccore::hash_string::sha256("{key#" + _unique_id + "}"));

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
		// session threads
		_session_broadcast_sender = std::async(std::launch::async, session_broadcast_sender_func, this);
		_session_broadcast_receiver = std::async(std::launch::async, session_broadcast_receiver_func, this);

		// message threads
		_message_broadcast_sender = std::async(std::launch::async, message_broadcast_sender_func, this);
		_message_broadcast_receiver = std::async(std::launch::async, message_broadcast_receiver_func, this);

		// user threads
		_user_broadcast_sender = std::async(std::launch::async, user_broadcast_sender_func, this);
		_user_broadcast_receiver = std::async(std::launch::async, user_broadcast_receiver_func, this);

		// file threads
		_file_broadcast_sender = std::async(std::launch::async, file_broadcast_sender_func, this);
		_file_broadcast_receiver = std::async(std::launch::async, file_broadcast_receiver_func, this);

		// review threads
		_review_broadcast_sender = std::async(std::launch::async, review_broadcast_sender_func, this);
		_review_broadcast_receiver = std::async(std::launch::async, review_broadcast_receiver_func, this);
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}

	return true;
}

const std::string& collab::impl::cert_folder() {
	return _cert_folder;
}

const std::string& collab::impl::files_folder() {
	return _files_folder;
}

std::optional<std::reference_wrapper<liblec::leccore::database::connection>> collab::impl::get_connection() {
	return _p_con ? std::optional<std::reference_wrapper<liblec::leccore::database::connection>>{ *_p_con } : std::nullopt;
}

collab::collab() : _d(*new impl(*this)) {
	// make unique id from computer bios serial number
	std::string error;
	liblec::leccore::pc_info _pc_info;
	liblec::leccore::pc_info::pc_details _pc_details;
	if (_pc_info.pc(_pc_details, error)) {
		if (!_pc_details.bios_serial_number.empty())
			_d._unique_id = _pc_details.bios_serial_number;
	}

	// mask with a hash
	_d._unique_id = liblec::leccore::hash_string::sha256(_d._unique_id);
}

collab::~collab() {
	delete& _d;
}

const std::string& collab::unique_id() { return _d._unique_id; }

bool collab::initialize(const std::string& database_file, const std::string& cert_folder,
	const std::string& files_folder, std::function<void(const std::string&)> log, std::string& error) {
	return _d.initialize(database_file, cert_folder, files_folder, log, error);
}

const std::string& collab::cert_folder() {
	return _d.cert_folder();
}

const std::string& collab::files_folder() {
	return _d.files_folder();
}
