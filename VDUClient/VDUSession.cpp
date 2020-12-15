#include "pch.h"
#include "framework.h"
#include "VDUSession.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

CVDUSession::CVDUSession(CVDUClientDlg* mainWnd, LPCTSTR serverURL) : m_wnd(mainWnd), m_fsThread(nullptr)
{
	StringCchCopy(m_serverURL, ARRAYSIZE(m_serverURL), serverURL);
}

CVDUSession::~CVDUSession()
{

}

void CVDUSession::CallbackPing(CVDUClientDlg* wnd, CHttpFile* file)
{
	//TODO: read headers, add notif and stuff back

	if (file)
		wnd->SetConnected(TRUE);
	else
		wnd->SetConnected(FALSE);
}

BOOL CVDUSession::Login(LPCTSTR user, LPCTSTR cert)
{
	return TRUE;
}
