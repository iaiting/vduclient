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
			WND->UpdateStatus();
			//WND->MessageBox(_T("Ping OK"), TITLENAME, MB_OK);
		}
		else
		{
			WND->MessageBox(_T("Invalid Ping response"), TITLENAME, MB_ICONERROR);
		}
	}
	else
	{
		//TODO: Dont use MessageBox in callback - they are blocking, could lead to deadlock
		WND->MessageBox(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
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
			WND->GetDlgItem(IDC_STATIC_FILETOKEN)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_FILE_TOKEN)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_ACCESS_FILE)->EnableWindow(TRUE);
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
		WND->MessageBox(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
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
		VDU_SESSION_UNLOCK;
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
		WND->GetDlgItem(IDC_STATIC_FILETOKEN)->EnableWindow(FALSE);
		WND->GetDlgItem(IDC_FILE_TOKEN)->EnableWindow(FALSE);
		WND->GetDlgItem(IDC_ACCESS_FILE)->EnableWindow(FALSE);
	}
	else
	{
		WND->MessageBox(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
	}

	WND->GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
	WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);
}

void CVDUSession::CallbackDownloadFile(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_OK)
		{
			CString contentLength;
			if (!file->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, contentLength))
			{
				WND->MessageBox(_T("Server did not send Content-Length!"), TITLENAME, MB_ICONERROR);
				return;
			}

			int contentLen = _ttoi(contentLength);

			CString allow;
			file->QueryInfo(HTTP_QUERY_ALLOW, allow);
			allow = allow.MakeUpper();

			CString contentEncoding;
			file->QueryInfo(HTTP_QUERY_CONTENT_ENCODING, contentEncoding);

			CString contentLocation;
			file->QueryInfo(HTTP_QUERY_CONTENT_LOCATION, contentLocation);

			CString contentMD5W;
			file->QueryInfo(HTTP_QUERY_CONTENT_MD5, contentMD5W);
			CStringA contentMD5 = CStringA(contentMD5W);
			BYTE contentmd5[0x400] = {0};
			int contentmd5Len = ARRAYSIZE(contentmd5);
			Base64Decode(contentMD5, contentMD5.GetLength(), contentmd5, &contentmd5Len);

			CString contentType;
			file->QueryInfo(HTTP_QUERY_CONTENT_TYPE, contentType);

			CString lastModified;
			file->QueryInfo(HTTP_QUERY_LAST_MODIFIED, lastModified);
			SYSTEMTIME lastModifiedST;
			InternetTimeToSystemTime(lastModified, &lastModifiedST, 0);

			CString expires;
			file->QueryInfo(HTTP_QUERY_EXPIRES, expires);
			SYSTEMTIME expiresST;
			InternetTimeToSystemTime(expires, &expiresST, 0);

			CString etag;
			file->QueryInfo(HTTP_QUERY_ETAG, etag);

			BOOL canRead = allow.Find(_T("GET")) != -1;
			BOOL canWrite = allow.Find(_T("POST")) != -1;
			CString filetoken = file->GetObject();
			filetoken = filetoken.Right(filetoken.GetLength() - 6);

			CVDUFile vfile(filetoken, canRead, canWrite, contentEncoding, contentLocation, contentType, lastModifiedST, expiresST, contentmd5, etag);

			//If file created successfuly, open it and notify user
			if (APP->GetFileSystemService()->SpawnFile(vfile, file))
			{
				INT result = (INT)ShellExecute(WND->GetSafeHwnd(), _T("open"), APP->GetFileSystemService()->GetDrivePath() + vfile.m_name, NULL, NULL, SW_SHOWNORMAL);
				WND->TrayNotify(vfile.m_name, CString(_T("File successfuly accessed!")), result == SE_ERR_NOASSOC ? SIID_DOCNOASSOC : SIID_DOCASSOC);
			}
			else
			{

			}
		}
		else if (statusCode == HTTP_STATUS_NOT_FOUND)
		{
			WND->MessageBox(_T("File does not exist!"), TITLENAME, MB_ICONERROR);
		}
		else if (statusCode == HTTP_STATUS_BAD_METHOD)
		{
			WND->MessageBox(_T("You can not read this file!"), TITLENAME, MB_ICONERROR);
		}
		else if (statusCode == HTTP_STATUS_DENIED)
		{
			WND->MessageBox(_T("Invalid authorization token!\r\nPlease log in again."), TITLENAME, MB_ICONERROR);
		}
		else
		{
			WND->MessageBox(_T("Error accessing file!"), TITLENAME, MB_ICONERROR);
		}
	}
	else
	{
		WND->MessageBox(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
	}

}

void CVDUSession::CallbackUploadFile(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_OK)
		{
			//TODO: It was ok! Nice
		}
		else //TODO: Differentiate status codes
		{

		}
	}
}

void CVDUSession::CallbackInvalidateFileToken(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_OK)
		{
			//TODO: It was ok! Nice
		}
		else //TODO: Differentiate status codes
		{

		}
	}
}

void CVDUSession::Login(CString user, BYTE* certData, UINT64 certDataLen)
{
	SetUser(user);

	CString headers;
	headers += _T("From: ");
	headers += GetUser();
	headers += _T("\r\n");

	AfxBeginThread(CVDUConnection::ThreadProc, 
		(LPVOID)(new CVDUConnection(GetServerURL(), VDUAPIType::POST_AUTH_KEY, CVDUSession::CallbackLogin, headers, _T(""), certData, certDataLen)));
}

BOOL CVDUSession::IsLoggedIn()
{
	return !GetUser().IsEmpty();
}

void CVDUSession::Logout()
{
	if (!IsLoggedIn())
		return;

	AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::DELETE_AUTH_KEY, CVDUSession::CallbackLogout));
}

void CVDUSession::AccessFile(CString fileToken)
{
	if (!IsLoggedIn())
		return;

	AfxBeginThread(CVDUConnection::ThreadProc,
		(LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::GET_FILE, CVDUSession::CallbackDownloadFile,_T(""), fileToken));
}