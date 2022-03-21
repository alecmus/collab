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
#include <functional>

/// <summary>Collaboration class.</summary>
class collab {
public:
	/// <summary>Session structure.</summary>
	struct session {
		/// <summary>The session's unique ID, preferrably a uuid.</summary>
		std::string unique_id;

		/// <summary>The session's name.</summary>
		std::string name;

		/// <summary>The session's description.</summary>
		std::string description;

		/// <summary>The hash of the session's passphrase.</summary>
		std::string passphrase_hash;

		bool operator==(const session& param) const {
			return
				unique_id == param.unique_id &&
				name == param.name &&
				description == param.description;
		}

		bool operator!=(const session& param) const {
			return !operator==(param);
		}
	};

	/// <summary>Message structure.</summary>
	struct message {
		/// <summary>The message's unique ID, preferrably a uuid.</summary>
		std::string unique_id;

		/// <summary>The time the message was posted (time_t value).</summary>
		long long time;

		/// <summary>The unique ID of the session in which the message was posted.</summary>
		std::string session_id;

		/// <summary>The unique ID of the user that posted the message.</summary>
		std::string sender_unique_id;

		/// <summary>The message text (basic HTML supported).</summary>
		std::string text;

		bool operator==(const message& param) const {
			return
				unique_id == param.unique_id &&
				time == param.time &&
				session_id == param.session_id &&
				sender_unique_id == param.sender_unique_id &&
				text == param.text;
		}

		bool operator!=(const message& param) const {
			return !operator==(param);
		}
	};

	/// <summary>User structure.</summary>
	struct user {
		/// <summary>The user's unique ID, preferrably a uuid or sha256 of something that uniquely identifies the user's computer.</summary>
		std::string unique_id;

		/// <summary>The user's username, preferrably unique but doesn't have to be.</summary>
		std::string username;

		/// <summary>The user's display name, preferrably the forename and surname seperated by a space.</summary>
		std::string display_name;

		/// <summary>A blob of the user's image, preferrably a compressed format such as .jpg, with limited dimensions.</summary>
		std::string user_image;
	};

	/// <summary>File structure.</summary>
	struct file {
		/// <summary>The file's hash, preferrably a secure modern hash such as sha256.</summary>
		std::string hash;

		/// <summary>The time the file was posted (time_t value).</summary>
		long long time;

		/// <summary>The unique ID of the session to which the file was posted.</summary>
		std::string session_id;

		/// <summary>The unique ID of the user that posted the file.</summary>
		std::string sender_unique_id;

		/// <summary>The name of the file, excluding the file extension, e.g. 'SRS Document'</summary>
		std::string name;

		/// <summary>The extension of the file, including the dot, e.g. '.pdf'.</summary>
		std::string extension;

		/// <summary>A brief description of the file.</summary>
		std::string description;

		/// <summary>The size of the file, in bytes.</summary>
		long long size;

		bool operator==(const file& param) const {
			return
				hash == param.hash &&
				time == param.time &&
				session_id == param.session_id &&
				sender_unique_id == param.sender_unique_id &&
				name == param.name &&
				extension == param.extension &&
				description == param.description &&
				size == param.size;
		}

		bool operator!=(const file& param) const {
			return !operator==(param);
		}
	};

	/// <summary>Review structure.</summary>
	struct review {
		/// <summary>The review's unique ID, preferrably a uuid.</summary>
		std::string unique_id;

		/// <summary>The time the review was posted (time_t value).</summary>
		long long time;

		/// <summary>The unique ID of the session in which the review was posted.</summary>
		std::string session_id;

		/// <summary>The hash of the file to which the review belongs.</summary>
		std::string file_hash;

		/// <summary>The unique ID of the user that posted the review.</summary>
		std::string sender_unique_id;

		/// <summary>The review text (basic HTML supported).</summary>
		std::string text;

		bool operator==(const review& param) const {
			return
				unique_id == param.unique_id &&
				time == param.time &&
				session_id == param.session_id &&
				file_hash == param.file_hash &&
				sender_unique_id == param.sender_unique_id &&
				text == param.text;
		}

		bool operator!=(const review& param) const {
			return !operator==(param);
		}
	};

	collab ();
	~collab ();

	/// <summary>Get the unique id associated with this PC.</summary>
	/// <returns>The unique id string (a unique sha256 hash).</returns>
	const std::string& unique_id();

	/// <summary>Initialize the collaboration class.</summary>
	/// <param name="database_file">The full path to the database file.</param>
	/// <param name="cert_folder">The full path to the certificate folder.</param>
	/// <param name="files_folder">The full path to the files folder.</param>
	/// <param name="log">The function to call for logging.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>This method creates the database connection. Calling any methods that need a
	/// database connection before calling it will only result in those methods returning
	/// a 'No database connection' error.</remarks>
	bool initialize(const std::string& database_file, const std::string& cert_folder,
		const std::string& files_folder, std::function<void(const std::string&)> log, std::string& error);

	/// <summary>Get the full path to the app folder.</summary>
	/// <returns>Returns the full path to the app folder.</returns>
	const std::string& cert_folder();

	/// <summary>Get the full path to the files folder.</summary>
	/// <returns>Returns the full path to the files folder.</returns>
	const std::string& files_folder();
	
	//------------------------------------------------------------------------------------------------
	// users

	/// <summary>Save a new user.</summary>
	/// <param name="user">The user's information, as defined in <see cref="collab::user"></see>.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Saves to the local database.</remarks>
	bool save_user(const user& user,
		std::string& error);

	/// <summary>Check if a user exists.</summary>
	/// <param name="unique_id">The user's unique id.</param>
	/// <returns>Returns true if the user exists, else false.</returns>
	/// <remarks>Checks the local database.</remarks>
	bool user_exists(const std::string& unique_id);

	/// <summary>Gets user info from the local database.</summary>
	/// <param name="unique_id">The unique id of the user.</param>
	/// <param name="user">The user's information, as defined in <see cref="collab::user"></see>.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reads from the local database.</remarks>
	bool get_user(const std::string& unique_id,
		user& user, std::string& error);

	/// <summary>Get a user's display name.</summary>
	/// <param name="unique_id">The user's unique id.</param>
	/// <param name="display_name">The user's display name.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reads from the local database.</remarks>
	bool get_user_display_name(const std::string& unique_id,
		std::string& display_name,
		std::string& error);

	/// <summary>Edit an existing user.</summary>
	/// <param name="unique_id">The user's unique id.</param>
	/// <param name="user">The new user details.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Saves changes to the local database.</remarks>
	bool edit_user(const std::string& unique_id,
		const user& user,
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

	/// <summary>Get a session.</summary>
	/// <param name="unique_id">The session's unique id.</param>
	/// <param name="session_info">The session.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool get_session(const std::string& unique_id,
		session& session_info, std::string& error);

	/// <summary>Get all available sessions.</summary>
	/// <param name="sessions">The list of all available sessions.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reads from the local database.</remarks>
	bool get_sessions(std::vector<session>& sessions,
		std::string& error);

	/// <summary>Get all available local sessions.</summary>
	/// <param name="sessions">The list of available local sessions.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>This is effectively all available sessions less the temporary
	/// sessions.</remarks>
	bool get_local_sessions(std::vector<session>& sessions,
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

	/// <summary>Set the current session's unique id.</summary>
	/// <param name="session_unique_id">The session's unique id.</param>
	void set_current_session_unique_id(const std::string& session_unique_id);

	//------------------------------------------------------------------------------------------------
	// messages

	/// <summary>Create a session message.</summary>
	/// <param name="message">The session message as defined in <see cref="collab::message"></see>.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool create_message(const message& message,
		std::string& error);

	/// <summary>Get session messages.</summary>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <param name="messages">The list of messages.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Messages are ordered chronologically, starting with the oldest.</remarks>
	bool get_messages(const std::string& session_unique_id,
		std::vector<message>& messages,
		std::string& error);

	/// <summary>Get latest session messages.</summary>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <param name="messages">The list of messages.</param>
	/// <param name="number">The number of latest messages (0 means unlimited).</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Messages are ordered chronologically, starting with the latest.</remarks>
	bool get_latest_messages(const std::string& session_unique_id,
		std::vector<message>& messages,
		int number, std::string& error);

	/// <summary>Check if a user has any messages in a given session.</summary>
	/// <param name="user_unique_id">The user's unique id.</param>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <returns>Returns true if the user has at least one message in the session, else false.</returns>
	bool user_has_messages_in_session(const std::string& user_unique_id,
		const std::string& session_unique_id);

	//------------------------------------------------------------------------------------------------
	// files

	/// <summary>Create a file.</summary>
	/// <param name="file">The file, as defined in <see cref='collab::file'></see>.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool create_file(const file& file, std::string& error);

	/// <summary>Get session files.</summary>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <param name="files">The list of files.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Files are ordered chronologically, starting with the latest.</remarks>
	bool get_files(const std::string& session_unique_id,
		std::vector<file>& files,
		std::string& error);

	/// <summary>Get session file.</summary>
	/// <param name="hash">The file's hash.</param>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <param name="file">The file as defined in <see cref="collab::file"></see>.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Files are ordered chronologically, starting with the latest.</remarks>
	bool get_file(const std::string& hash,
		const std::string& session_unique_id, file& file,
		std::string& error);

	/// <summary>Check if a file exists in a given session.</summary>
	/// <param name="hash">The hash of the file.</param>
	/// <param name="session_unique_id">The session's unique ID.</param>
	/// <returns>Returns true if the file exists, else false.</returns>
	bool file_exists(const std::string& hash,
		const std::string& session_unique_id);

	/// <summary>Check if a file exists.</summary>
	/// <param name="hash">The hash of the file.</param>
	/// <returns>Returns true if the file exists, else false.</returns>
	bool file_exists(const std::string& hash);

	/// <summary>Check if a user has any files in a given session.</summary>
	/// <param name="user_unique_id">The user's unique id.</param>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <returns>Returns true if the user has at least one file in the session, else false.</returns>
	bool user_has_files_in_session(const std::string& user_unique_id,
		const std::string& session_unique_id);

	//------------------------------------------------------------------------------------------------
	// reviews

	/// <summary>Create a review.</summary>
	/// <param name="review">The review as defined in <see cref="collab::review"></see>.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool create_review(const review& review,
		std::string& error);

	/// <summary>Get session file reviews.</summary>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <param name="reviews">The list of reviews.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reviews are ordered chronologically, starting with the latest.</remarks>
	bool get_reviews(const std::string& session_unique_id,
		std::vector<review>& reviews,
		std::string& error);

	/// <summary>Get session file reviews for a specific file.</summary>
	/// <param name="session_unique_id">The session's unique id.</param>
	/// <param name="file_hash">The hash of the file.</param>
	/// <param name="reviews">The list of reviews.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	/// <remarks>Reviews are ordered chronologically, starting with the latest.</remarks>
	bool get_reviews(const std::string& session_unique_id,
		const std::string& file_hash,
		std::vector<review>& reviews,
		std::string& error);

	/// <summary>Get file review.</summary>
	/// <param name="unique_id">The review's unique id.</param>
	/// <param name="review">The review.</param>
	/// <param name="error">Error information.</param>
	/// <returns>Returns true if successful, else false.</returns>
	bool get_review(const std::string& unique_id,
		review& review,
		std::string& error);

	/// <summary>Check if a file review exists.</summary>
	/// <param name="unique_id">The review's unique id.</param>
	/// <returns>Returns true if the review exists, else false.</returns>
	/// <remarks>Checks the local database.</remarks>
	bool review_exists(const std::string& unique_id);

private:
	class impl;
	impl& _d;

	// Copying an object of this class is not allowed
	collab(const collab&) = delete;
	collab& operator=(const collab&) = delete;
};
