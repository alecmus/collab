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

/// <summary>
/// Format data size in B, KB, MB, GB or TB.
/// </summary>
///
/// <returns>
/// Returns a formatted string in the form 5B, 45KB, 146MB, 52GB, 9TB etc.
/// </returns>
std::string format_size(unsigned long long size);
