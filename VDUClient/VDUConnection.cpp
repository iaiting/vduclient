#include "pch.h"
#include "framework.h"
#include "VDUConnection.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

void CVDUConnection::Process()
{
	CInternetSession inetsession(_T("VDUClient 1.0, Windows"));
	int httpVerb;
	LPCTSTR apiPath = NULL;

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
		return;
	}

	CString httpObjectPath;
	httpObjectPath += apiPath;
	httpObjectPath += m_parameter;

	INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
#ifdef _DEBUG //If debug server
	if (m_serverURL == _T("127.0.0.1"))
		port = 4443;
#endif

	//Empty URL causes exception
	if (m_serverURL.IsEmpty())
		return;

	//Lock the session if this connection has a callback, it will then be responsibile for unlocking
	if (m_callback != nullptr)
	{
		VDU_SESSION_LOCK;
	}

	CHttpConnection* con = inetsession.GetHttpConnection(m_serverURL, port, NULL, NULL);
	CHttpFile* pFile = con->OpenRequest(httpVerb, httpObjectPath, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_TRANSFER_BINARY
#ifdef _DEBUG //Ignores certificates in debug mode
		| INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	DWORD opt;
	pFile->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
	opt |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
	pFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
#else 
		);
#endif

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

	TRY
	{
		//Content -> http content after headrs
		pFile->SendRequest(NULL, NULL, m_content.GetBuffer(), m_content.GetLength());
	}
	CATCH(CInternetException, e)
	{
		TCHAR errmsg[0x400];
		e->GetErrorMessage(errmsg, ARRAYSIZE(errmsg));
		//TODO: How to include this reasonably? Rn its handled elsewhere
		//WND->MessageBox(errmsg, TITLENAME, MB_ICONERROR);

		//THROW(e);
		if (pFile)
			pFile->Close();
		pFile = nullptr;

		if (con)
			con->Close();
		con = nullptr;

		e->Delete();
	}
	END_CATCH;

	//Call our callback
	if (m_callback != nullptr)
	{
		m_callback(pFile);
		VDU_SESSION_UNLOCK;
	}

	

	if (pFile)
		pFile->Close();
	if (con)
		con->Close();
}

//Constructing connection does not initiate it
CVDUConnection::CVDUConnection(CString serverURL, VDUAPIType type, VDU_CONNECTION_CALLBACK callback, CString requestHeaders, CString parameter, CString content) :
	m_serverURL(serverURL), m_parameter(parameter), m_type(type), m_requestHeaders(requestHeaders), m_callback(callback), m_content(content)
{

}

UINT CVDUConnection::ThreadProc(LPVOID pCon)
{
	if (pCon)
	{
		((CVDUConnection*)pCon)->Process();
		delete pCon;
	}
	else
		MessageBox(NULL, _T("Connection::ThreadProc: pCon was null"), TITLENAME, MB_ICONERROR);
	return EXIT_SUCCESS;
}