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

#include "helper_functions.h"
#include <Windows.h>
#include <strsafe.h>	// for StringCchPrintfA

#include <mutex>
#include <sstream>
#include <algorithm>
#include <stdio.h>

/// <summary>
/// Get current module's full path, whether it's a .exe or a .dll.
/// </summary>
/// 
/// <param name="full_path">
/// Full path to the module.
/// </param>
/// 
/// <returns>
/// Returns true if successful, else false;
/// </returns>
bool get_module_full_path(std::string& full_path) {
	char buffer[MAX_PATH];
	HMODULE h_module = NULL;

	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		// use the address of the current function to detect the module handle of the current application (critical for DLLs)
		(LPCSTR)&get_module_full_path,
		&h_module))
		return false;

	// using the module handle enables detection of filename even of a DLL
	GetModuleFileNameA(h_module, buffer, MAX_PATH);
	full_path = buffer;
	return true;
}

bool get_directory_from_full_path(
	const std::string& full_path,
	std::string& directory) {
	directory.clear();

	const size_t last_slash_index = full_path.rfind('\\');

	if (std::string::npos != last_slash_index)
		directory = full_path.substr(0, last_slash_index);

	return true;
}

bool get_filename_from_full_path(
	const std::string& full_path,
	std::string& file_name) {
	file_name.clear();

	const size_t last_slash_idx = full_path.rfind('\\');

	if (std::string::npos != last_slash_idx) {
		file_name = full_path;
		file_name.erase(0, last_slash_idx + 1);
	}

	return true;
}

std::string get_current_folder() {
	std::string full_path;
	if (get_module_full_path(full_path)) {
		std::string current_folder;
		return get_directory_from_full_path(full_path, current_folder) ?
			current_folder : std::string();
	}
	else
		return std::string();
}

class mutex;

/// <summary>
/// Wrapper for the std::mutex object.
/// </summary>
class liblec::mutex::mutex_impl {
public:
	mutex_impl() {}
	~mutex_impl() {}

	void lock() {
		_mtx.lock();
	}

	void unlock() {
		_mtx.unlock();
	}

	std::mutex _mtx;
};

liblec::mutex::mutex() {
	_d = new mutex_impl;
}

liblec::mutex::~mutex() {
	if (_d) {
		delete _d;
		_d = nullptr;
	}
}

class liblec::auto_mutex::auto_mutex_impl {
public:
	auto_mutex_impl(mutex& mtx) :
		_p_mtx(&mtx) {
		_p_mtx->_d->lock();
	}

	~auto_mutex_impl() {
		_p_mtx->_d->unlock();
	}

private:
	mutex* _p_mtx;
};

liblec::auto_mutex::auto_mutex(mutex& mtx) {
	_d = new auto_mutex_impl(mtx);
}

liblec::auto_mutex::~auto_mutex() {
	if (_d) {
		delete _d;
		_d = nullptr;
	}
}

void liblec::log(const std::string& string) {
#if defined(_DEBUG)
	std::string _string = "-->" + string + "\n";
	printf(_string.c_str());
	OutputDebugStringA(_string.c_str());
#endif
}

template<typename T>
bool sort_and_compare(std::vector<T>& v1, std::vector<T>& v2) {
	std::sort(v1.begin(), v1.end());
	std::sort(v2.begin(), v2.end());
	return v1 == v2;
}

struct ip {
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
};

ip convert_ip(std::string s_ip) {
	ip m_ip;
	sscanf_s(s_ip.c_str(), "%d.%d.%d.%d", &m_ip.i, &m_ip.j, &m_ip.k, &m_ip.l);
	return m_ip;
}

std::string convert_ip(ip m_ip) {
	std::stringstream s;
	s << m_ip.i << "." << m_ip.j << "." << m_ip.k << "." << m_ip.l;
	return s.str();
}

std::string select_ip(std::vector<std::string> server_ips, std::vector<std::string> client_ips) {
	std::string s_ip;

	try {
		// erase all occurences of "127.0.0.1" from the IP lists
		server_ips.erase(std::remove(server_ips.begin(), server_ips.end(), "127.0.0.1"), server_ips.end());
		client_ips.erase(std::remove(client_ips.begin(), client_ips.end(), "127.0.0.1"), client_ips.end());

		do {
			if (sort_and_compare(server_ips, client_ips)) {
				s_ip = "127.0.0.1";
				break;
			}

			std::vector<ip> v_server, v_client;

			// convert server IPs
			for (const auto& it : server_ips)
				v_server.push_back(convert_ip(it));

			// convert client IPs
			for (const auto& it : client_ips)
				v_client.push_back(convert_ip(it));

			struct similarity_count {
				ip server_ip;
				int count;
			};

			std::vector<similarity_count> v_structs;

			// loop through the client IPs and find which one is most similar to the server IPs
			for (const auto& it : v_client) {
				for (const auto& m_it : v_server) {
					int count = 0;

					if (it.i == m_it.i) {
						count++;

						if (it.j == m_it.j) {
							count++;

							if (it.k == m_it.k) {
								count++;

								if (it.l == m_it.l)
									count++;
							}
						}
					}

					similarity_count m;
					m.count = count;
					m.server_ip = m_it;

					v_structs.push_back(m);
				}
			}

			// loop through and select one with greatest count
			if (v_structs.empty()) {
				// this shouldn't happen
				s_ip.clear();
				break;
			}

			s_ip = convert_ip(v_structs[0].server_ip);
			int highest_count = v_structs[0].count;

			for (const auto& it : v_structs) {
				if (it.count > highest_count) {
					s_ip = convert_ip(it.server_ip);
					highest_count = it.count;
				}
			}
		} while (false);
	}
	catch (const std::exception&) {
		// this shouldn't happen
	}

	return s_ip;
}
