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
	void SetUser(CString user);
	void SetAuthData(CString authToken, CTime expires);

	//Login to server using user and ceritificate
	void Login(CString user, CString cert);
	//Log out current user
	void Logout();

	//Callbacks are guaranteed to have exclusive access to session
	static void CallbackPing(CHttpFile* file);
	static void CallbackLogin(CHttpFile* file);
	static void CallbackLoginRefresh(CHttpFile* file);
	static void CallbackLogout(CHttpFile* file);
	static void CallbackDownloadFile(CHttpFile* file);
	static void CallbackUploadFile(CHttpFile* file);
	static void CallbackInvalidateFileToken(CHttpFile* file);
};
