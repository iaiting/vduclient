#pragma once

#include "VDUConnection.h"
#include "VDUFilesystem.h"

class CVDUSession
{
protected:
	CVDUClientDlg* m_wnd; //Main window
	CString m_serverURL; //Server url
	BOOL m_loggedIn; //Is user logged in
	CString m_user; //Logged in user
	CWinThread* m_svcThread; //File system thread
	CVDUFileSystemService* m_svc; //File system pointer, running on svcThread
public:

	CVDUSession(CVDUClientDlg* mainWnd, LPCTSTR serverURL);
	~CVDUSession();

	static UINT ThreadProcFilesystemService(LPVOID service);

	static void CallbackPing(CVDUClientDlg* wnd, CHttpFile* file);
	static void CallbackLogin(CVDUClientDlg* wnd, CHttpFile* file);
	static void CallbackLoginRefresh(CVDUClientDlg* wnd, CHttpFile* file);

	//Login to server using user and ceritificate
	BOOL Login(LPCTSTR user, LPCTSTR cert);
};
