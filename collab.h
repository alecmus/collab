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

#include <string>
#include <vector>

/// <summary>Collaboration class.</summary>
class collab {
	std::string _unique_id;

	enum class node_status {
		unknown,
		available,
		unavailable,
		busy,
	};

	struct node {
		std::string id;
		std::string name;
		std::string display_name;
		std::vector<std::string> ip_address_list;
		std::string signed_in_session_id;
		node_status status;
	};
	
	struct token {

	};

public:
	struct session {
		std::string unique_id;
		std::string name;
		std::string description;
		std::string passphrase_hash;
	};

	collab ();
	~collab ();

	/// <summary>Get the unique id associated with this PC.</summary>
	/// <returns>The unique id string (a unique sha256 hash).</returns>
	const std::string& unique_id();

	/// <summary>Initialize the collaboration class.</summary>
	/// <param name="database_file">The full path to the database file.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>This method creates the database connection. Calling any methods that need a
	/// database connection before calling it will only result in those methods returning
	/// a 'No database connection' error.</remarks>
	bool initialize(const std::string& database_file,
		std::string& error);
	
	//------------------------------------------------------------------------------------------------
	// users

	/// <summary>Save a new user.</summary>
	/// <param name="unique_id">The user's unique id.</param>
	/// <param name="username">The username, e.g. alecmus.</param>
	/// <param name="display_name">The user's display name, e.g. Alec Musasa.</param>
	/// <param name="user_image">The user's image (blob data).</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Saves to the local database.</remarks>
	bool save_user(const std::string& unique_id,
		const std::string& username,
		const std::string& display_name,
		const std::string& user_image,
		std::string& error);

	/// <summary>Check if a user exists.</summary>
	/// <param name="unique_id">The user's unique id.</param>
	/// <returns>Returns true if the user exists, else false.</returns>
	/// <remarks>Checks the local database.</remarks>
	bool user_exists(const std::string& unique_id);

	/// <summary>Gets user info from the local database.</summary>
	/// <param name="unique_id">The unique id of the user.</param>
	/// <param name="username">The username, e.g. alecmus.</param>
	/// <param name="display_name">The user's display name,e.g. Alec Musasa.</param>
	/// <param name="user_image">The user's image (blob data).</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reads from the local database.</remarks>
	bool get_user(const std::string& unique_id,
		std::string& username,
		std::string& display_name,
		std::string& user_image,
		std::string& error);

	/// <summary>Edit an existing user.</summary>
	/// <param name="unique_id">The user's unique id.</param>
	/// <param name="username">The new username.</param>
	/// <param name="display_name">The new display name.</param>
	/// <param name="user_image">The new image (blob data).</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Saves changes to the local database.</remarks>
	bool edit_user(const std::string& unique_id,
		const std::string& username,
		const std::string& display_name,
		const std::string& user_image,
		std::string& error);

	//------------------------------------------------------------------------------------------------
	// sessions

	/// <summary>Create a new session.</summary>
	/// <param name="session">The session information as defined in the session structure.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Adds a new entry to the local database.</remarks>
	bool create_session(const session& session,
		std::string& error);

	/// <summary>Check if a session exists.</summary>
	/// <param name="unique_id">The session's unique id.</param>
	/// <returns>Returns true if the session exists, else false.</returns>
	/// <remarks>Checks the local database.</remarks>
	bool session_exists(const std::string& unique_id);

	/// <summary>Remove a session from the local database.</summary>
	/// <param name="unique_id">The session's unique id.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool remove_session(const std::string& unique_id,
		std::string& error);

	/// <summary>Get all available sessions.</summary>
	/// <param name="sessions">The list of all available sessions.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reads from the local database.</remarks>
	bool get_sessions(std::vector<session>& sessions,
		std::string& error);

	/// <summary>Create temporary session entry.</summary>
	/// <param name="unique_id">The session's unique id.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>A session remains on a temporary session list if it is imported and
	/// has not yet been joined. This way if the app is closed and re-opened when the
	/// source of the imported session is no longer available it will not continue to
	/// be listed. This is an essential auto-cleanup mechanism.</remarks>
	bool create_temporary_session_entry(const std::string& unique_id,
		std::string& error);

	/// <summary>Check if a unique id belongs to a session in the temporary session list.</summary>
	/// <param name="unique_id">The session's unique id.</param>
	/// <returns>Returns true if the session is in the temporary session list, else false.</returns>
	bool is_temporary_session_entry(const std::string& unique_id);

	/// <summary>Remove a session from the temporary session list.</summary>
	/// <param name="unique_id">The session's unique id.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool remove_temporary_session_entry(const std::string& unique_id,
		std::string& error);

private:
	class impl;
	impl& _d;
};
