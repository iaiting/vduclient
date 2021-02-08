#include "pch.h"
#include "framework.h"
#include "VDUSession.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

CVDUSession::CVDUSession(CString serverURL) :
	m_serverURL(serverURL), m_user(_T("")),	m_authToken(_T(""))
{
}

CVDUSession::~CVDUSession()
{

}

CString CVDUSession::GetServerURL()
{
	return m_serverURL;
}

BOOL CVDUSession::IsLoggedIn()
{
	return m_loggedIn;
}

UINT CVDUSession::ThreadProcFilesystemService(LPVOID service)
{
	CVDUFileSystemService* s = (CVDUFileSystemService*)service;
	ASSERT(s);
	return s->Run();
}

void CVDUSession::CallbackPing(CHttpFile* file)
{
	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == 204)
		{
			CString date;
			file->QueryInfo(HTTP_QUERY_DATE, date);
			WND->SetConnected(TRUE);
			//WND->TrayNotify(date, _T("Connection established."), SIID_WORLD);
			return;
		}
		else
			WND->MessageBox(_T("Invalid Ping response"), TITLENAME, MB_ICONASTERISK);
	}

	WND->SetConnected(FALSE);
}

void CVDUSession::CallbackLogin(CHttpFile* file)
{
	if (file)
	{
		CVDUSession* session = WND->GetSession();
		ASSERT(session);

		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == 201)
		{
			TCHAR apiKey[0x400] = { _T("X-Api-Key") };
			DWORD apiLeyLen = sizeof(apiKey) / sizeof(apiKey[0]);

			if (!file->QueryInfo(HTTP_QUERY_CUSTOM, (LPVOID)apiKey, &apiLeyLen))
			{
				WND->MessageBox(_T("Server did not send auth key!"), TITLENAME, MB_ICONERROR);
				return;
			}

			CString expires;
			if (!file->QueryInfo(HTTP_QUERY_EXPIRES, expires))
			{
				WND->MessageBox(_T("Server did not send Expiration Date!"), TITLENAME, MB_ICONERROR);
				return;
			}

			SYSTEMTIME expTime;
			SecureZeroMemory(&expTime, sizeof(expTime));

			if (!InternetTimeToSystemTime(expires, &expTime, 0))
			{
				WND->MessageBox(_T("Failed to convert auth token Expiration time!"), TITLENAME, MB_ICONERROR);
				return;
			}

			session->m_authToken = apiKey;
			session->m_authTokenExpires = CTime(expTime);
		}
		else
		{
			WND->MessageBox(_T("Login failed! User or certificate is invalid."), TITLENAME, MB_ICONASTERISK);
		}

		return;
	}

	WND->SetConnected(FALSE);
}

void CVDUSession::Login(CString user, CString cert)
{
	CString headers;
	headers += _T("From: ");
	headers += user;
	headers += _T("\r\n");

	m_user = user;

	AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)(
		new CVDUConnection(m_serverURL, VDUAPIType::POST_AUTH_KEY, CVDUSession::CallbackLogin, headers, _T(""), cert)));
	//Spawn file system service on a separate thread
	//CString preferredLetter = APP->GetProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), _T(""));
	//this->m_svcThread = AfxBeginThread(CVDUSession::ThreadProcFilesystemService, (LPVOID)(new CVDUFileSystemService(preferredLetter)));
}
