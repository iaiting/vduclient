/**
*
 * @author Adam Feranec
 * @file VDUSession.h
 * 
 * This project is licensed under GPLv3, as it includes a modification
 * of work of WinFsp - Windows File System Proxy , Copyright (C) Bill Zissimopoulos.
 * GitHub page at https://github.com/billziss-gh/winfsp, Website at https://www.secfs.net/winfsp/.
 * The original file https://github.com/billziss-gh/winfsp/blob/master/tst/passthrough-cpp/passthrough-cpp.cpp
 *  was modified into two:
 * https://github.com/coolguy124/vduclient/blob/master/VDUClient/VDUFilesystem.cpp
 * https://github.com/coolguy124/vduclient/blob/master/VDUClient/VDUFilesystem.h
 * by Adam Feranec, dates and details noted in said files.
 * @copyright 2015-2020 Bill Zissimopoulos
 *
 */

#pragma once

#include "VDUConnection.h"
#include "VDUFilesystem.h"
#include <time.h>

#define APIKEY_HEADER _T("X-Api-Key")

class CVDUSession
{
public:
	SRWLOCK m_lock; //Used to handle writing to session data
private:
	CString m_serverURL; //Server url
	CString m_user; //Logged in user
	CString m_authToken; //Current autorization token
	CTime m_authTokenExpires; //When auth token expires
public:
	CVDUSession(CString serverURL);
	~CVDUSession();

	//Functions are only to be used while having exclusive access to session

	void Reset(CString serverURL); //Resets session state for new server
	CString GetServerURL(); //Returns the current server URL
	CString GetUser(); //Returns the current user
	CString GetAuthToken(); //Returns current auth token
	CTime GetAuthTokenExpires(); //Returns time when auth token expires
	void SetUser(CString user); //Sets current user name
	void SetAuthData(CString authToken, CTime expires); //Sets authorization data

	BOOL IsLoggedIn(); //Checks if an user is logged in

	//Login to server using user and ceritificate
	//This function is BLOCKING if async is FALSE
	//Returns success or exit code if not async
	INT Login(CString user, CString certPath, BOOL async = TRUE);

	//Log out current user
	//This function is BLOCKING if async is FALSE
	//Returns success or exit code if not async
	INT Logout(BOOL async = TRUE);

	//Attempts to download VDU file of fileToken and add it to the filesystem
	//This function is BLOCKING if async is FALSE
	//Returns success or exit code if not async
	INT AccessFile(CString fileToken, BOOL async = TRUE);

	//Callbacks are guaranteed to have exclusive access to session
	static INT CallbackPing(CHttpFile* file);
	static INT CallbackLogin(CHttpFile* file);
	static INT CallbackLoginRefresh(CHttpFile* file);
	static INT CallbackLogout(CHttpFile* file);
	static INT CallbackDownloadFile(CHttpFile* file);
	static INT CallbackUploadFile(CHttpFile* file);
	static INT CallbackInvalidateFileToken(CHttpFile* file);
};
