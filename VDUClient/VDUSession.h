#pragma once

#include "VDUConnection.h"
#include "VDUFilesystem.h"
#include <time.h>

#define APIKEY_HEADER _T("X-Api-Key")

class CVDUSession
{
private:
	SRWLOCK m_lock; //Used to handle writing to session data

	CString m_serverURL; //Server url
	CString m_user; //Logged in user
	CString m_authToken; //Current autorization token
	CTime m_authTokenExpires; //When auth token expires
public:

	CVDUSession(CString serverURL);
	~CVDUSession();

	void Reset(CString serverURL); //Resets session state for new server

	//Read functions are thread safe
	//Write functions are protected by srw locks

	CString GetServerURL(); //Returns the current server URL
	CString GetUser(); //Returns the current user
	CString GetAuthToken(); //Returns current auth token
	CTime GetAuthTokenExpires(); //Returns time when auth token expires

	void SetUser(CString user);
	void SetAuthData(CString authToken, CTime expires);

	static void CallbackPing(CHttpFile* file);
	static void CallbackLogin(CHttpFile* file);
	static void CallbackLoginRefresh(CHttpFile* file);
	static void CallbackLogout(CHttpFile* file);

	//Login to server using user and ceritificate
	void Login(CString user, CString cert);
	void Logout();
};
