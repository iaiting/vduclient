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

	BOOL IsLoggedIn(); //Checks if an user is logged in

	//Login to server using user and ceritificate
	void Login(CString user, CString certPath);
	//Log out current user
	void Logout();
	//
	void AccessFile(CString fileToken);

	//Callbacks are guaranteed to have exclusive access to session
	static INT CallbackPing(CHttpFile* file);
	static INT CallbackLogin(CHttpFile* file);
	static INT CallbackLoginRefresh(CHttpFile* file);
	static INT CallbackLogout(CHttpFile* file);
	static INT CallbackDownloadFile(CHttpFile* file);
	static INT CallbackUploadFile(CHttpFile* file);
	static INT CallbackInvalidateFileToken(CHttpFile* file);
};
