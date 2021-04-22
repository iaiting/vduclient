#include "pch.h"
#include "framework.h"
#include "VDUConnection.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

//initialize error buffer
TCHAR CVDUConnection::LastError[0x400] = { 0 };

INT CVDUConnection::Process()
{
	CInternetSession inetsession(_T("VDUClient 1.0, Windows"));
	int httpVerb;
	TCHAR* apiPath = NULL;

	switch (m_type)
	{
	case VDUAPIType::GET_PING:
	{
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = _T("/ping");
		break;
	}
	case VDUAPIType::POST_AUTH_KEY:
	{
		httpVerb = CHttpConnection::HTTP_VERB_POST;
		apiPath = _T("/auth/key");
		break;
	}
	case VDUAPIType::GET_AUTH_KEY:
	{
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = _T("/auth/key");
		break;
	}
	case VDUAPIType::DELETE_AUTH_KEY:
	{
		httpVerb = CHttpConnection::HTTP_VERB_DELETE;
		apiPath = _T("/auth/key");
		break;
	}
	case VDUAPIType::GET_FILE:
	{
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = _T("/file/");
		break;
	}
	case VDUAPIType::POST_FILE:
	{
		httpVerb = CHttpConnection::HTTP_VERB_POST;
		apiPath = _T("/file/");
		break;
	}
	case VDUAPIType::DELETE_FILE:
	{
		httpVerb = CHttpConnection::HTTP_VERB_DELETE;
		apiPath = _T("/file/");
		break;
	}
	default:
		WND->MessageBox(_T("Invalid VDUAPI Type"), TITLENAME, MB_ICONWARNING);
		return EXIT_FAILURE;
	}

	CString httpObjectPath;
	httpObjectPath += apiPath;
	httpObjectPath += m_parameter;

	CString serverURL, httpObj;
	DWORD service;
	INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;

	//Implicit https if only ip is input
	if (m_serverURL.Find(_T("://")) == -1)
		m_serverURL = _T("https://") + m_serverURL;

	AfxParseURL(m_serverURL, service, serverURL, httpObj, port);

	//Lock the session if this connection has a callback, it will then be responsibile for unlocking
	if (m_callback != nullptr)
	{
		VDU_SESSION_LOCK;
	}


	INT result = EXIT_SUCCESS;
	CHttpConnection* con = NULL;
	CHttpFile* pFile = NULL;
	TRY
	{
		con = inetsession.GetHttpConnection(serverURL, port, NULL, NULL);

		//Insecure mode does not validate certificates
		if (APP->IsInsecure())
		{
			pFile = con->OpenRequest(httpVerb, httpObjectPath, NULL, 1, NULL, NULL,
				INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE
				| INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
			DWORD opt;
			pFile->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
			opt |= SECURITY_SET_MASK;
			pFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
		}
		else
		{
			pFile = con->OpenRequest(httpVerb, httpObjectPath, NULL, 1, NULL, NULL,
				INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE);
		}

		//For every other API, transactions need to be done one after the other
		//And for that reason, auth token is filled in last, in order to be sure its up to date
		if (m_type != VDUAPIType::GET_PING && !APP->GetSession()->GetAuthToken().IsEmpty())
		{
			CString headers;
			headers += APIKEY_HEADER;
			headers += _T(": ");
			headers += APP->GetSession()->GetAuthToken();
			headers += _T("\r\n");
			m_requestHeaders += headers;
		}

		if (!m_requestHeaders.IsEmpty())
			pFile->AddRequestHeaders(m_requestHeaders);
	
		if (m_contentFile.IsEmpty())
		{
			pFile->SendRequest();
		}
		else
		{
			CStdioFile stdf(m_contentFile, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone);
			CFileStatus fst;
			stdf.GetStatus(fst);
			ULONGLONG writeLen = fst.m_size;

			pFile->SendRequestEx((DWORD)writeLen);

			BYTE buf[0x400];
			UINT readLen;
			while ((readLen = stdf.Read(buf, ARRAYSIZE(buf))) > 0)
			{
				pFile->Write(buf, readLen);
			}
			stdf.Close();
			pFile->EndRequest();
		}

	}
	CATCH(CException, e)
	{
		e->GetErrorMessage(LastError, ARRAYSIZE(LastError));

		if (pFile)
		{
			pFile->Close();
			delete pFile;
		}
		pFile = nullptr;

		if (con)
		{
			con->Close();
			delete con;
		}
		con = nullptr;

		//if (hFile != INVALID_HANDLE_VALUE)
		//	CloseHandle(hFile);

		inetsession.Close();
	}
	END_CATCH

	//Call our callback
	if (m_callback != nullptr)
	{
		result = m_callback(pFile);

		VDU_SESSION_UNLOCK;
	}

	inetsession.Close();

	if (pFile)
	{
		pFile->Close();
		delete pFile;
	}
	if (con)
	{
		con->Close();
		delete con;
	}

	return result;
}

CVDUConnection::CVDUConnection(CString serverURL, VDUAPIType type, VDU_CONNECTION_CALLBACK callback, CString requestHeaders, CString parameter, CString fileContentPath) :
	m_serverURL(serverURL), m_parameter(parameter), m_type(type), m_requestHeaders(requestHeaders), m_contentFile(fileContentPath), m_callback(callback)
{
}

UINT CVDUConnection::ThreadProc(LPVOID pCon)
{
	//NOTE: For destructor to be called, connection has to be properly casted
	CVDUConnection* con = (CVDUConnection*)pCon;
	if (con)
	{
		INT returnCode = con->Process();
		delete con;
		return returnCode;
	}
	else
		WND->MessageBoxNB(_T("Connection::ThreadProc: connection was null"), TITLENAME, MB_ICONERROR);
	return EXIT_SUCCESS;
}