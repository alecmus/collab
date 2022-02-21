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
