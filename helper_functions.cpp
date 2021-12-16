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

namespace nmReadableSize {
	/*
	** adapted from wxWidgets whose license is as follows:
	**
	** Name:        src / common / filename.cpp
	** Purpose:     wxFileName - encapsulates a file path
	** Author:      Robert Roebling, Vadim Zeitlin
	** Modified by:
	** Created:     28.12.2000
	** RCS-ID:      $Id$
	** Copyright:   (c) 2000 Robert Roebling
	** Licence:     wxWindows licence
	*/

	// size conventions
	enum SizeConvention {
		SIZE_CONV_TRADITIONAL,  // 1024 bytes = 1 KB
		SIZE_CONV_SI            // 1000 bytes = 1 KB
	};

	std::string GetHumanReadableSize(const long double& dSize,
		const std::string& nullsize,
		int precision,
		SizeConvention conv) {
		// deal with trivial case first
		if (dSize == 0)
			return nullsize;

		// depending on the convention used the multiplier may be either 1000 or
		// 1024 and the binary infix may be empty (for "KB") or "i" (for "KiB")
		long double multiplier = 1024.;

		switch (conv) {
		case SIZE_CONV_TRADITIONAL:
			// nothing to do, this corresponds to the default values of both
			// the multiplier and infix string
			break;

		case SIZE_CONV_SI:
			multiplier = 1000;
			break;
		}

		const long double kiloByteSize = multiplier;
		const long double megaByteSize = multiplier * kiloByteSize;
		const long double gigaByteSize = multiplier * megaByteSize;
		const long double teraByteSize = multiplier * gigaByteSize;

		const long double bytesize = dSize;

		size_t const cchDest = 256;
		char pszDest[cchDest];

		if (bytesize < kiloByteSize)
			StringCchPrintfA(pszDest, cchDest, "%.*f B", 0, dSize);
		else if (bytesize < megaByteSize)
			StringCchPrintfA(pszDest, cchDest, "%.*f KB", precision, bytesize / kiloByteSize);
		else if (bytesize < gigaByteSize)
			StringCchPrintfA(pszDest, cchDest, "%.*f MB", precision, bytesize / megaByteSize);
		else if (bytesize < teraByteSize)
			StringCchPrintfA(pszDest, cchDest, "%.*f GB", precision, bytesize / gigaByteSize);
		else
			StringCchPrintfA(pszDest, cchDest, "%.*f TB", precision, bytesize / teraByteSize);

		return pszDest;
	}
}

std::string format_size(unsigned long long size) {
	return nmReadableSize::GetHumanReadableSize(static_cast<long double>(size), "0 B", 0,
		nmReadableSize::SIZE_CONV_TRADITIONAL);
}
