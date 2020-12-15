#pragma once

#include "VDUConnection.h"

class CVDUSession
{
protected:
	CVDUClientDlg* m_wnd; //Main window
	TCHAR m_serverURL[INTERNET_MAX_HOST_NAME_LENGTH]; //Server url
	CWinThread* m_fsThread; //File system thread
public:

	CVDUSession(CVDUClientDlg* mainWnd, LPCTSTR serverURL);
	~CVDUSession();

	static void CallbackPing(CVDUClientDlg* wnd, CHttpFile* file);

	//Login to server using user and ceritificate
	BOOL Login(LPCTSTR user, LPCTSTR cert);
};
