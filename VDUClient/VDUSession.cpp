#include "pch.h"
#include "framework.h"
#include "VDUSession.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

CVDUSession::CVDUSession(CString serverURL) : m_lock(SRWLOCK_INIT)
{
	Reset(serverURL);
}

CVDUSession::~CVDUSession()
{

}

CString CVDUSession::GetServerURL()
{
	return m_serverURL;
}

CString CVDUSession::GetAuthToken()
{
	return m_authToken;
}

CTime CVDUSession::GetAuthTokenExpires()
{
	return m_authTokenExpires;
}

CString CVDUSession::GetUser()
{
	return m_user;
}

void CVDUSession::Reset(CString serverURL)
{
	m_serverURL = serverURL;
	m_user = _T("");
	m_authToken = _T("");
	m_authTokenExpires = CTime(0);
}

void CVDUSession::SetAuthData(CString authToken, CTime expires)
{
	m_authToken = authToken;
	m_authTokenExpires = expires;
}

void CVDUSession::SetUser(CString user)
{
	m_user = user;
}

void CVDUSession::CallbackPing(CHttpFile* file)
{
	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_NO_CONTENT)
		{
			CString date;
			file->QueryInfo(HTTP_QUERY_DATE, date);
			WND->TrayNotify(date, _T("Ping OK."), SIID_WORLD);
			//WND->MessageBox(_T("Ping OK"), TITLENAME, MB_OK);
		}
		else
		{
			WND->MessageBox(_T("Invalid Ping response"), TITLENAME, MB_ICONERROR);
		}
	}
	else
	{
		WND->MessageBox(_T("Could not connect to server."), TITLENAME, MB_ICONERROR);
	}

	WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);
}

void CVDUSession::CallbackLogin(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	WND->GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
	WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_CREATED)
		{
			TCHAR apiKey[0x400] = { APIKEY_HEADER };
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

			if (!InternetTimeToSystemTime(expires, &expTime, NULL))
			{
				WND->MessageBox(_T("Failed to convert auth token Expiration time!"), TITLENAME, MB_ICONERROR);
				return;
			}

			SYSTEMTIME cstime;
			SecureZeroMemory(&cstime, sizeof(cstime));
			GetSystemTime(&cstime);
			CTime now(cstime);
			CTime exp(expTime);

			//Expiration date has to be in future
			if (exp < now)
			{
				WND->MessageBox(_T("Auth expiration date in the past!"), TITLENAME, MB_ICONERROR);
				return;
			}

			//Login successful
			session->SetAuthData(apiKey, exp);
			WND->GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("Logout"));
		}
		else
		{
			session->SetUser(_T(""));
			WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
			WND->MessageBox(_T("Login failed! User or certificate is invalid."), TITLENAME, MB_ICONERROR);
		}
	}
	else
	{
		session->SetUser(_T(""));
		WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
		WND->MessageBox(_T("Could not connect to server."), TITLENAME, MB_ICONERROR);
	}
}

void CVDUSession::CallbackLoginRefresh(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_OK)
		{
			TCHAR apiKey[0x400] = { APIKEY_HEADER };
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

			if (!InternetTimeToSystemTime(expires, &expTime, NULL))
			{
				WND->MessageBox(_T("Failed to convert auth token Expiration time!"), TITLENAME, MB_ICONERROR);
				return;
			}

			SYSTEMTIME cstime;
			SecureZeroMemory(&cstime, sizeof(cstime));
			GetSystemTime(&cstime);
			CTime now(cstime);
			CTime exp(expTime);

			//Expiration date has to be in future
			if (exp < now)
			{
				WND->MessageBox(_T("Auth expiration date in the past!"), TITLENAME, MB_ICONERROR);
				return;
			}

			session->SetAuthData(apiKey, exp);
		}
		else
		{
			session->Reset(session->GetServerURL());

			WND->GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("Login"));
			WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
			WND->TrayNotify(_T("Authetification failed"), _T("Please login again to refresh your session."), SIID_SERVER);
		}
	}
	else
	{
		//Connection failed, let refreshing thread know to sleep for a bit and try again later
		AfxEndThread(2, FALSE);

		//session->Reset(session->GetServerURL());
		//WND->TrayNotify(_T("Authentification failed"), _T("Could not connect to server."), SIID_INTERNET);
	}
}

void CVDUSession::CallbackLogout(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_NO_CONTENT)
		{
			//TODO: It was ok! Nice
		}
		else
		{
			WND->TrayNotify(_T("Authetification failed"), _T("Server failed to log out. Session reset."), SIID_RENAME);
		}

		session->Reset(session->GetServerURL());

		WND->GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("Login"));
		WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
	}
	else
	{
		WND->MessageBox(_T("Connection to server failed."), TITLENAME, MB_ICONERROR);
	}

	WND->GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
	WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);
}

void CVDUSession::Login(CString user, CString cert)
{
	SetUser(user);

	CString headers;
	headers += _T("From: ");
	headers += GetUser();
	headers += _T("\r\n");

	AfxBeginThread(CVDUConnection::ThreadProc, 
		(LPVOID)(new CVDUConnection(GetServerURL(), VDUAPIType::POST_AUTH_KEY, CVDUSession::CallbackLogin, headers, _T(""), cert)));
}

void CVDUSession::Logout()
{
	if (GetUser().IsEmpty())
		return;

	AfxBeginThread(CVDUConnection::ThreadProc, 
		(LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::DELETE_AUTH_KEY, CVDUSession::CallbackLogout));
}
