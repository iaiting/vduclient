#include "pch.h"
#include "framework.h"
#include "VDUSession.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

CVDUSession::CVDUSession(CVDUClientDlg* mainWnd, LPCTSTR serverURL) :
	m_wnd(mainWnd), m_svcThread(nullptr), m_serverURL(serverURL), m_user(_T("")), m_loggedIn(FALSE), m_svc(nullptr)
{
}

CVDUSession::~CVDUSession()
{

}

UINT CVDUSession::ThreadProcFilesystemService(LPVOID service)
{
	CVDUFileSystemService* s = (CVDUFileSystemService*)service;
	ASSERT(s);
	ULONG Result = s->Run();
	return Result;
}

void CVDUSession::CallbackPing(CVDUClientDlg* wnd, CHttpFile* file)
{
	if (file)
	{
		DWORD statusCode = -1;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == 204)
		{
			CString date;
			file->QueryInfo(HTTP_QUERY_DATE, date);
			wnd->SetConnected(TRUE);
			wnd->TrayNotify(date, _T("Connection established."), SIID_WORLD);
			return;
		}
		else
			wnd->MessageBox(_T("Invalid Ping response"), VDU_TITLENAME, MB_ICONASTERISK);
	}

	wnd->SetConnected(FALSE);
}

void CVDUSession::CallbackLogin(CVDUClientDlg* wnd, CHttpFile* file)
{
	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		CVDUSession* session = wnd->GetSession();
		ASSERT(session != nullptr);

		return;
	}

	wnd->SetConnected(FALSE);
}

BOOL CVDUSession::Login(LPCTSTR user, LPCTSTR cert)
{
	//TODO: Login request

	//Spawn file system service on a separate thread
	CString preferredLetter = AfxGetApp()->GetProfileString(VDU_SECTION_SETTINGS, _T("PreferredDriveLetter"), _T("V:"));
	this->m_svcThread = AfxBeginThread(CVDUSession::ThreadProcFilesystemService, (LPVOID)(new CVDUFileSystemService(preferredLetter)));

	return TRUE;
}
