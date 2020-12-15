#include "pch.h"
#include "framework.h"
#include "VDUConnection.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

void CVDUConnection::Process()
{
	m_session = new CInternetSession(L"VDUClient 1.0, Windows");
	int httpVerb;
	LPCTSTR apiPath = NULL;

	//m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(L"Connecting...");
	//m_wnd->GetProgressBar()->SetPos(0);

	switch (m_type)
	{
	case VDUAPIType::GET_PING:
	{
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = L"/ping";
		break;
	}
	case VDUAPIType::POST_AUTH_KEY:
	{
		httpVerb = CHttpConnection::HTTP_VERB_POST;
		apiPath = L"/auth/key";
		break;
	}
	case VDUAPIType::GET_AUTH_KEY:
	{
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = L"/auth/key";
		break;
	}
	case VDUAPIType::DELETE_AUTH_KEY:
	{
		httpVerb = CHttpConnection::HTTP_VERB_DELETE;
		apiPath = L"/auth/key";
		break;
	}
	case VDUAPIType::GET_FILE:
	{
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = L"/file/";
		break;
	}
	default:
		m_wnd->MessageBox(L"Invalid VDUAPI Type", L"VDU Connection", MB_ICONWARNING);
		return;
	}


	TCHAR httpObjectPath[INTERNET_MAX_HOST_NAME_LENGTH];
	wsprintfW(httpObjectPath, L"%s%s", apiPath, m_parameter);
	CHttpConnection* con = m_session->GetHttpConnection(m_serverURL, (INTERNET_PORT)4443, NULL, NULL);
	CHttpFile* pFile = con->OpenRequest(httpVerb, apiPath, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE
#ifdef _DEBUG //Ignores certificates in debug mode
		| INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	DWORD opt;
	pFile->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
	opt |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
	pFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS, opt);
#else 
		);
#endif

	TRY
	{
		//m_wnd->GetProgressBar()->SetPos(50);

		if (!pFile->SendRequest())
		{
			m_wnd->MessageBox(L"Failed to send request", L"VDU Connection", MB_ICONWARNING);
			return;
		}

		/*DWORD statusCode;
		if (!pFile->QueryInfoStatusCode(statusCode))
		{
			m_wnd->MessageBox(L"Failed to query status code", L"VDU Connection", MB_ICONWARNING);
			return;
		}*/

		/*m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(L"Connected");
		m_wnd->GetProgressBar()->SetPos(100);
		m_wnd->GetProgressBar()->SetState(PBST_NORMAL);
		m_wnd->SetConnected(TRUE);
		m_wnd->TrayNotify(m_serverURL, L"Connected successfuly.", SIID_DRIVENET);*/
	}
		CATCH(CInternetException, e)
	{
		//pFile = nullptr;


		/*m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(L"Not connected to server");
		m_wnd->SetConnected(FALSE);
		m_wnd->GetProgressBar()->SetPos(100);
		m_wnd->GetProgressBar()->SetState(PBST_ERROR);
		m_wnd->MessageBox(L"Failed to connect to server\r\nPlease check the server address", L"VDU Connection", MB_ICONWARNING);*/
		//THROW(e);
	}
	END_CATCH;

	//Call our callback
	if (m_callback != nullptr)
		m_callback(m_wnd, pFile);

	pFile->Close();
	con->Close();
	m_session->Close();
	delete m_session;
}

//Constructing connection does not initiate it
CVDUConnection::CVDUConnection(CVDUClientDlg* mainWnd, LPCTSTR serverURL, VDUAPIType type, LPCTSTR parameter, VDU_CONNECTION_CALLBACK callback) :
	m_session(nullptr), m_wnd(mainWnd), m_type(type), m_callback(callback)
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
		MessageBox(NULL, L"Connection::ThreadProc: pCon was null", VDU_TITLENAME, MB_ICONERROR);
	return EXIT_SUCCESS;
}