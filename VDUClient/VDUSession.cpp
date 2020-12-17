#include "pch.h"
#include "framework.h"
#include "VDUSession.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

CVDUSession::CVDUSession(CVDUClientDlg* mainWnd, LPCTSTR serverURL) :
	m_wnd(mainWnd), m_fsThread(nullptr), m_serverURL(serverURL), m_user(L""), m_loggedIn(FALSE)
{
}

CVDUSession::~CVDUSession()
{

}

void CVDUSession::CallbackPing(CVDUClientDlg* wnd, CHttpFile* file)
{
	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == 204)
		{
			CString date;
			file->QueryInfo(HTTP_QUERY_DATE, date);
			wnd->SetConnected(TRUE);
			wnd->TrayNotify(date, L"Connection established.", SIID_WORLD);
			return;
		}
		else
			wnd->MessageBox(L"Invalid Ping response", VDU_TITLENAME, MB_ICONASTERISK);
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
	return TRUE;
}
