/*
 * @copyright 2015-2022 Bill Zissimopoulos
 *
 * @file VDUSession.cpp
 * This file is licensed under the GPLv3 licence.
 *
 * You can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 3 as published by the Free Software
 * Foundation.
 */

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

INT CVDUSession::CallbackPing(CHttpFile* file)
{
	if (!APP->IsTestMode())
		WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_NO_CONTENT)
		{
			if (!APP->IsTestMode())
			{
				CString date;
				file->QueryInfo(HTTP_QUERY_DATE, date);
				WND->TrayNotify(date, _T("Ping OK."), SIID_WORLD);
				WND->UpdateStatus();
			}

			return EXIT_SUCCESS;
		}
		else
		{
			WND->MessageBoxNB(_T("Invalid Ping response"), TITLENAME, MB_ICONERROR);
		}
	}
	else
	{
		//TODO: Dont use MessageBox in callback - they are blocking, could lead to deadlock
		WND->MessageBoxNB(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
	}
	return EXIT_FAILURE;
}

INT CVDUSession::CallbackLogin(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (!APP->IsTestMode())
	{
		WND->GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);
	}

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
				WND->MessageBoxNB(_T("Server did not send auth key!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			CString expires;
			if (!file->QueryInfo(HTTP_QUERY_EXPIRES, expires))
			{
				WND->MessageBoxNB(_T("Server did not send Expiration Date!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			SYSTEMTIME expTime;
			SecureZeroMemory(&expTime, sizeof(expTime));

			if (!InternetTimeToSystemTime(expires, &expTime, NULL))
			{
				WND->MessageBoxNB(_T("Failed to convert auth token Expiration time!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			SYSTEMTIME cstime;
			SecureZeroMemory(&cstime, sizeof(cstime));
			GetSystemTime(&cstime);
			CTime now(cstime);
			CTime exp(expTime);

			//Expiration date has to be in future
			if (exp < now)
			{
				WND->MessageBoxNB(_T("Auth expiration date in the past!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			//Login successful
			session->SetAuthData(apiKey, exp);

			if (!APP->IsTestMode())
			{
				WND->GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("Logout"));
				WND->GetDlgItem(IDC_STATIC_FILETOKEN)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_FILE_TOKEN)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_ACCESS_FILE)->EnableWindow(TRUE);
				WND->UpdateStatus();
			}

			return EXIT_SUCCESS;
		}
		else
		{
			session->SetUser(_T(""));

			if (!APP->IsTestMode())
			{
				WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
				WND->MessageBoxNB(_T("Login failed! User or certificate is invalid."), TITLENAME, MB_ICONERROR);
			}
		}
	}
	else
	{
		session->SetUser(_T(""));

		if (!APP->IsTestMode())
		{
			WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
			WND->MessageBoxNB(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
		}
	}

	return EXIT_FAILURE;
}

INT CVDUSession::CallbackLoginRefresh(CHttpFile* file)
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
				WND->MessageBoxNB(_T("Server did not send auth key!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			CString expires;
			if (!file->QueryInfo(HTTP_QUERY_EXPIRES, expires))
			{
				WND->MessageBoxNB(_T("Server did not send Expiration Date!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			SYSTEMTIME expTime;
			SecureZeroMemory(&expTime, sizeof(expTime));

			if (!InternetTimeToSystemTime(expires, &expTime, NULL))
			{
				WND->MessageBoxNB(_T("Failed to convert auth token Expiration time!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			SYSTEMTIME cstime;
			SecureZeroMemory(&cstime, sizeof(cstime));
			GetSystemTime(&cstime);
			CTime now(cstime);
			CTime exp(expTime);

			//Expiration date has to be in future
			if (exp < now)
			{
				WND->MessageBoxNB(_T("Auth expiration date in the past!"), TITLENAME, MB_ICONERROR);
				return EXIT_FAILURE;
			}

			session->SetAuthData(apiKey, exp);

			WND->UpdateStatus();

			return EXIT_SUCCESS;
		}
		else
		{
			session->Reset(session->GetServerURL());

			if (!APP->IsTestMode())
			{
				WND->GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("Login"));
				WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
				WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
				WND->TrayNotify(_T("Authetification failed"), _T("Please login again to refresh your session."), SIID_SERVER);
			}
		}
	}
	else
	{
		//Connection failed, let refreshing thread know to sleep for a bit and try again later
		return 2;
	}

	return EXIT_FAILURE;
}

INT CVDUSession::CallbackLogout(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (!APP->IsTestMode())
	{
		WND->GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);
	}

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		session->Reset(session->GetServerURL());

		if (!APP->IsTestMode())
		{
			WND->GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("Login"));
			WND->GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
			WND->GetDlgItem(IDC_STATIC_FILETOKEN)->EnableWindow(FALSE);
			WND->GetDlgItem(IDC_FILE_TOKEN)->EnableWindow(FALSE);
			WND->GetDlgItem(IDC_ACCESS_FILE)->EnableWindow(FALSE);
			WND->UpdateStatus();
		}

		if (statusCode == HTTP_STATUS_NO_CONTENT)
		{
			//TODO: It was ok! Nice 
			return EXIT_SUCCESS;
		}
		else
		{
			if (!APP->IsTestMode())
				WND->TrayNotify(_T("Authetification failed"), _T("Server failed to log out. Session reset."), SIID_RENAME);
		}
	}
	else
	{
		WND->MessageBoxNB(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
	}

	return EXIT_FAILURE;
}

INT CVDUSession::CallbackDownloadFile(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (!APP->IsTestMode())
	{
		WND->GetDlgItem(IDC_FILE_TOKEN)->EnableWindow(TRUE);
		WND->GetDlgItem(IDC_ACCESS_FILE)->EnableWindow(TRUE);
	}

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_OK)
		{
			CString contentLength;
			if (!file->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, contentLength))
			{
				WND->MessageBoxNB(_T("Server did not send Content-Length!"), TITLENAME, MB_ICONERROR);
				//return;
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
			/*CStringA contentMD5 = CStringA(contentMD5W);
			BYTE contentmd5[0x400] = {0};
			int contentmd5Len = ARRAYSIZE(contentmd5);
			Base64Decode(contentMD5, contentMD5.GetLength(), contentmd5, &contentmd5Len);*/

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

			CVDUFile vfile(filetoken, canRead, canWrite, contentLen, contentEncoding, contentLocation, contentType, lastModifiedST, expiresST, contentMD5W, etag);

			//If file created successfuly, open it and notify user
			if (APP->GetFileSystemService()->CreateVDUFile(vfile, file))
			{
				//Dont open anything in test mode
				if (!APP->IsTestMode())
				{
					ShellExecute(WND->GetSafeHwnd(), _T("open"), APP->GetFileSystemService()->GetDrivePath() + vfile.m_name, NULL, NULL, SW_SHOWNORMAL);
					WND->TrayNotify(vfile.m_name, CString(_T("File successfuly accessed!")), SIID_DOCASSOC);
					WND->UpdateStatus();
				}
				return EXIT_SUCCESS;
			}
			else
			{
				WND->MessageBoxNB(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
			}
		}
		else if (statusCode == HTTP_STATUS_NOT_FOUND)
		{
			WND->MessageBoxNB(_T("File does not exist!"), TITLENAME, MB_ICONERROR);
		}
		else if (statusCode == HTTP_STATUS_BAD_METHOD)
		{
			WND->MessageBoxNB(_T("You can not read this file!"), TITLENAME, MB_ICONERROR);
		}
		else if (statusCode == HTTP_STATUS_DENIED)
		{
			WND->MessageBoxNB(_T("Invalid authorization token!\r\nPlease log in again."), TITLENAME, MB_ICONERROR);
		}
		else
		{
			WND->MessageBoxNB(_T("Error accessing file!"), TITLENAME, MB_ICONERROR);
		}
	}
	else
	{
		WND->MessageBoxNB(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
	}

	return EXIT_FAILURE;
}

INT CVDUSession::CallbackUploadFile(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		CString filetoken = file->GetObject();
		filetoken = filetoken.Right(filetoken.GetLength() - 6);
		CVDUFile vdufile = APP->GetFileSystemService()->GetVDUFileByToken(filetoken);

		if (statusCode == HTTP_STATUS_CREATED)
		{
			if (vdufile != CVDUFile::InvalidFile)
			{
				CString expires;
				file->QueryInfo(HTTP_QUERY_EXPIRES, expires);
				SYSTEMTIME expiresST;
				InternetTimeToSystemTime(expires, &expiresST, 0);

				CString etag;
				file->QueryInfo(HTTP_QUERY_ETAG, etag);

				CString allow;
				file->QueryInfo(HTTP_QUERY_ALLOW, allow);
				allow = allow.MakeUpper();

				vdufile.m_etag = etag;
				vdufile.m_expires = expiresST;
				vdufile.m_canRead = allow.Find(_T("GET")) != -1;
				vdufile.m_canWrite = allow.Find(_T("POST")) != -1;
				vdufile.m_md5base64 = APP->GetFileSystemService()->CalcFileMD5Base64(vdufile);

				APP->GetFileSystemService()->UpdateFileInternal(vdufile);

				WND->UpdateStatus();
			}
			else
			{
				WND->MessageBoxNB(_T("Local file does not exist!\r\nPlease re-access the file."), TITLENAME, MB_ICONERROR);
			}
			return EXIT_SUCCESS;
		}
		else if (statusCode == HTTP_STATUS_RESET_CONTENT)
		{
			APP->GetFileSystemService()->DeleteFileInternal(filetoken);

			WND->UpdateStatus();

			WND->MessageBoxNB(_T("Token for currently edited file has expired after last successful upload.\r\nIn order to continue work on the file, access via new token."),
				TITLENAME, MB_ICONINFORMATION);

			return EXIT_SUCCESS;
		}
		else
		{
			if (statusCode == HTTP_STATUS_CONFLICT)
			{
				WND->MessageBoxNB(_T("The changes you made to the file are in conflict!\r\nPlease resolve these changes or delete your file."), TITLENAME, MB_ICONERROR);
			}
			else if (statusCode == HTTP_STATUS_NOT_FOUND)
			{
				WND->MessageBoxNB(_T("File does not exist!"), TITLENAME, MB_ICONERROR);
			}
			else if (statusCode == HTTP_STATUS_BAD_METHOD)
			{
				WND->MessageBoxNB(_T("Method not allowed!\r\nYoure trying to write to a read-only resource?"), TITLENAME, MB_ICONERROR);
			}
			else if (statusCode == HTTP_STATUS_REQUEST_TIMEOUT)
			{
				WND->MessageBoxNB(_T("Server timed out!"), TITLENAME, MB_ICONERROR);
			}
			else if (statusCode == HTTP_STATUS_DENIED)
			{
				WND->MessageBoxNB(_T("Invalid authorization token!\r\nPlease log in again."), TITLENAME, MB_ICONERROR);
			}
			else
			{
				WND->MessageBoxNB(_T("Error accessing file!"), TITLENAME, MB_ICONERROR);
			}
		}
	}
	else
	{
		WND->MessageBoxNB(CVDUConnection::LastError, TITLENAME, MB_ICONERROR);
	}

	return EXIT_FAILURE;
}

INT CVDUSession::CallbackInvalidateFileToken(CHttpFile* file)
{
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (file)
	{
		DWORD statusCode;
		file->QueryInfoStatusCode(statusCode);

		if (statusCode == HTTP_STATUS_NO_CONTENT)
		{
			CString filetoken = file->GetObject();
			filetoken = filetoken.Right(filetoken.GetLength() - 6);

			APP->GetFileSystemService()->DeleteFileInternal(filetoken);

			WND->UpdateStatus();

			return EXIT_SUCCESS;
		}
		else if (statusCode == HTTP_STATUS_NOT_FOUND)
		{
			WND->MessageBoxNB(_T("File does not exist!"), TITLENAME, MB_ICONERROR);
		}
		else if (statusCode == HTTP_STATUS_REQUEST_TIMEOUT)
		{
			WND->MessageBoxNB(_T("Server timed out!"), TITLENAME, MB_ICONERROR);
		}
		else if (statusCode == HTTP_STATUS_DENIED)
		{
			//Dont use this message, deletion has custom message
			//WND->MessageBoxNB(_T("Invalid authorization token!\r\nPlease log in again."), TITLENAME, MB_ICONERROR);
		}
		else
		{
			WND->MessageBoxNB(_T("Error accessing file!"), TITLENAME, MB_ICONERROR);
		}
	}

	return EXIT_FAILURE;
}

INT CVDUSession::Login(CString user, CString certPath, BOOL async)
{
	SetUser(user);

	CString headers;
	headers += _T("From: ");
	headers += GetUser();
	headers += _T("\r\n");

	if (async)
	{
		AfxBeginThread(CVDUConnection::ThreadProc,
			(LPVOID)(new CVDUConnection(GetServerURL(), VDUAPIType::POST_AUTH_KEY, CVDUSession::CallbackLogin, headers, _T(""), certPath)));
	}
	else
	{
		CWinThread* t = AfxBeginThread(CVDUConnection::ThreadProc,
			(LPVOID)(new CVDUConnection(GetServerURL(), VDUAPIType::POST_AUTH_KEY, CVDUSession::CallbackLogin, headers, _T(""), certPath)), 0, CREATE_SUSPENDED);

		DWORD exitCode;
		WAIT_THREAD_EXITCODE(t, exitCode);

		return exitCode;
	}

	return EXIT_SUCCESS;
}

BOOL CVDUSession::IsLoggedIn()
{
	return !GetUser().IsEmpty() && !GetAuthToken().IsEmpty();
}

INT CVDUSession::Logout(BOOL async)
{
	if (async)
	{
		if (!IsLoggedIn())
			return EXIT_SUCCESS;

		AfxBeginThread(CVDUConnection::ThreadProc,
			(LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::DELETE_AUTH_KEY, CVDUSession::CallbackLogout));
	}
	else
	{
		CWinThread* t = AfxBeginThread(CVDUConnection::ThreadProc,
			(LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::DELETE_AUTH_KEY, CVDUSession::CallbackLogout), 0, CREATE_SUSPENDED);

		DWORD exitCode;
		WAIT_THREAD_EXITCODE(t, exitCode);

		return exitCode;
	}

	return EXIT_SUCCESS;
}

INT CVDUSession::AccessFile(CString fileToken, BOOL async)
{
	if (!IsLoggedIn() || APP->GetFileSystemService()->GetVDUFileByToken(fileToken).IsValid())
		return EXIT_FAILURE;

	if (async)
	{
		AfxBeginThread(CVDUConnection::ThreadProc,
			(LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::GET_FILE, CVDUSession::CallbackDownloadFile, _T(""), fileToken));
	}
	else
	{
		CWinThread* t = AfxBeginThread(CVDUConnection::ThreadProc,
			(LPVOID)new CVDUConnection(GetServerURL(), VDUAPIType::GET_FILE, CVDUSession::CallbackDownloadFile, _T(""), fileToken), 0, CREATE_SUSPENDED);

		DWORD exitCode;
		WAIT_THREAD_EXITCODE(t, exitCode);

		return exitCode;
	}

	return EXIT_SUCCESS;
}