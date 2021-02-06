#include "pch.h"
#include "framework.h"
#include "VDUConnection.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

void CVDUConnection::Process()
{
	m_session = new CInternetSession(_T("VDUClient 1.0, Windows"));
	int httpVerb;
	LPCTSTR apiPath = NULL;

	//m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(_T("Connecting...");
	//m_wnd->GetProgressBar()->SetPos(0);

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
		m_wnd->MessageBox(_T("Invalid VDUAPI Type"), VDU_TITLENAME, MB_ICONWARNING);
		delete m_session;
		return;
	}

	CString httpObjectPath;
	httpObjectPath += apiPath;
	httpObjectPath += m_parameter;

	CHttpConnection* con = m_session->GetHttpConnection(m_serverURL, (INTERNET_PORT)4443, NULL, NULL);
	CHttpFile* pFile = con->OpenRequest(httpVerb, httpObjectPath, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE
#ifdef _DEBUG //Ignores certificates in debug mode
		| INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	DWORD opt;
	pFile->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
	opt |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
	pFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
#else 
		);
#endif

	if (m_requestHeaders && !m_requestHeaders.IsEmpty())
		pFile->AddRequestHeaders(m_requestHeaders);

	TRY
	{
		pFile->SendRequest();
	}
	CATCH(CInternetException, e)
	{
		TCHAR errmsg[0x400];
		e->GetErrorMessage(errmsg, ARRAYSIZE(errmsg));
		m_wnd->MessageBox(errmsg, VDU_TITLENAME, MB_ICONWARNING);

		//THROW(e);
		if (pFile)
			pFile->Close();
		pFile = nullptr;
		e->Delete();
	}
	END_CATCH;

	//Call our callback
	if (m_callback != nullptr)
		m_callback(m_wnd, pFile);

	if (pFile)
		pFile->Close();
	con->Close();
	m_session->Close();
	delete m_session;
}

//Constructing connection does not initiate it
CVDUConnection::CVDUConnection(CVDUClientDlg* mainWnd, LPCTSTR serverURL, VDUAPIType type, LPCTSTR parameter, CString requestHeaders, VDU_CONNECTION_CALLBACK callback) :
	m_session(nullptr), m_wnd(mainWnd), m_type(type), m_requestHeaders(requestHeaders), m_callback(callback)
{
	StringCchCopy(m_serverURL, ARRAYSIZE(m_serverURL), serverURL);
	StringCchCopy(m_parameter, ARRAYSIZE(m_parameter), parameter);
}

UINT CVDUConnection::ThreadProc(LPVOID pCon)
{
	if (pCon)
	{
		((CVDUConnection*)pCon)->Process();
		delete pCon;
	}
	else
		MessageBox(NULL, _T("Connection::ThreadProc: pCon was null"), VDU_TITLENAME, MB_ICONERROR);
	return EXIT_SUCCESS;
}