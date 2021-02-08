#pragma once

#include "VDUConnection.h"
#include "VDUFilesystem.h"
#include <time.h>

class CVDUSession
{
protected:
	CString m_serverURL; //Server url
	CString m_user; //Logged in user
	CString m_authToken; //Current autorization token
	CTime m_authTokenExpires; //When auth token expires
public:

	CVDUSession(CString serverURL);
	~CVDUSession();

	CString GetServerURL();
	BOOL IsLoggedIn();

	static UINT ThreadProcFilesystemService(LPVOID service);
	static UINT ThreadProcLoginRefresh(LPVOID);

	static void CallbackPing(CHttpFile* file);
	static void CallbackLogin(CHttpFile* file);
	static void CallbackLoginRefresh(CHttpFile* file);

	//Login to server using user and ceritificate
	void Login(CString user, CString cert);
};
