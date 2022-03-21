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

#pragma once

#include <string>
#include <vector>

static inline std::string shorten_unique_id(const std::string& unique_id) {
	std::string short_id;

	for (size_t i = 0; i < unique_id.length() && i < 8; i++)
		short_id += unique_id[i];
	
	return short_id;
}

template <typename T>
static inline T smallest(T a, T b) {
	return (((a) < (b)) ? (a) : (b));
}

template <typename T>
static inline T largest(T a, T b) {
	return (((a) > (b)) ? (a) : (b));
}

/// <summary>
/// Extract file name from full path.
/// </summary>
/// 
/// <param name="full_path">
/// The full path, e.g. C:\\MyFolder\\myFile.txt
/// </param>
/// 
/// <param name="file_name">
/// The directory in which file specified in sFullPath is.
/// </param>
/// 
/// <returns>
/// Returns true if successful, else false.
/// </returns>
bool get_filename_from_full_path(
	const std::string& full_path,
	std::string& file_name);

/// <summary>
/// Extract directory from full path.
/// </summary>
/// 
/// <param name="full_path">
/// The full path, e.g. C:\\MyFolder\\myFile.txt
/// </param>
/// 
/// <param name="directory">
/// The directory in which file specified in full_path is.
/// </param>
/// 
/// <returns>
/// Returns true if successful, else false.
/// </returns>
bool get_directory_from_full_path(
	const std::string& full_path,
	std::string& directory);

/// <summary>
/// Get the folder from which this module is running.
/// </summary>
/// 
/// <returns>
/// The full path to the current folder.
/// </returns>
std::string get_current_folder();

namespace liblec {
	void log(const std::string& string);

	class auto_mutex;

	/// <summary>
	/// A mutex object with no publicly accessible methods.
	/// Can only be used by the <see cref="auto_mutex"/> class.
	/// </summary>
	class mutex {
	public:
		mutex();
		~mutex();

	private:
		friend auto_mutex;
		class mutex_impl;
		mutex_impl* _d;
	};

	/// <summary>
	/// A mutex class that automatically unlocks the mutex when it's out of scope.
	/// </summary>
	/// 
	/// <remarks>
	/// Usage example to prevent multiple threads from accessing a function at the same moment:
	/// 
	/// liblec::mutex print_mutex;
	/// 
	/// void print() {
	///		liblec::auto_mutex lock(print_mutex);
	/// 
	///		// do printing
	/// 
	///		return;
	/// }
	/// 
	/// </remarks>
	class auto_mutex {
	public:
		auto_mutex(mutex& mtx);
		~auto_mutex();

	private:
		class auto_mutex_impl;
		auto_mutex_impl* _d;
	};
}

std::string select_ip(std::vector<std::string> server_ips, std::vector<std::string> client_ips);

bool file_available(const std::string& full_path);
