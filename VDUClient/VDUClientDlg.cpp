#include "pch.h"
#include "framework.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"
#include <afxinet.h>


//Declares valid VDU api types
enum class VDUAPIType
{
	GET_PING, //Ping the server
	GET_AUTH_KEY, 
	POST_AUTH_KEY,
	DELETE_AUTH_KEY,
	GET_FILE,
	POST_FILE,
	DELETE_FILE,
};

class CVDUConnection
{
protected:
	CVDUClientDlg* m_wnd;
	WCHAR m_serverURL[INTERNET_MAX_HOST_NAME_LENGTH];
	CInternetSession* m_session;
	VDUAPIType m_type;
public:
	//Sets up the connection
	CVDUConnection(CVDUClientDlg* mainWnd, LPCTSTR serverURL, VDUAPIType type);
	~CVDUConnection();

	//Processess the connection
	void Process();
};

void CVDUConnection::Process()
{
	m_session = new CInternetSession(L"VDUClient 1.0, Windows");
	int httpVerb;
	LPCTSTR apiPath = NULL;

	m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(L"Connecting...");
	CProgressCtrl* pBar = (CProgressCtrl*)m_wnd->GetDlgItem(IDC_PROGRESSBAR);
	pBar->SetPos(0);

	switch (m_type)
	{
	case VDUAPIType::GET_PING:
		httpVerb = CHttpConnection::HTTP_VERB_GET;
		apiPath = L"/ping";
		break;
	default:
		m_wnd->MessageBox(L"Invalid VDUAPI Type", L"VDU Connection", MB_ICONASTERISK);
		return;
	}
	CHttpConnection* con = m_session->GetHttpConnection(m_serverURL, (INTERNET_PORT)4443, NULL, NULL);
	CHttpFile* pFile = con->OpenRequest(httpVerb, apiPath, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE
#ifdef _DEBUG
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
		pBar->SetPos(50);

		if (!pFile->SendRequest())
		{
			m_wnd->MessageBox(L"Failed to send request", L"VDU Connection", MB_ICONASTERISK);
			return;
		}

		DWORD statusCode;
		if (!pFile->QueryInfoStatusCode(statusCode))
		{
			m_wnd->MessageBox(L"Failed to query status code", L"VDU Connection", MB_ICONASTERISK);
			return;
		}

		m_wnd->TrayNotify(m_serverURL, L"Connected successfuly.", SIID_DRIVENET);
		m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(L"Connected");
		pBar->SetPos(100);
		pBar->SetState(PBST_NORMAL);
		m_wnd->SetConnected(TRUE);
	}
	CATCH(CInternetException, e)
	{
		m_wnd->GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(L"Not connected to server");
		m_wnd->SetConnected(FALSE);
		pBar->SetPos(100);
		pBar->SetState(PBST_ERROR);
		m_wnd->MessageBox(L"Failed to connect to server\r\nPlease check the server address", L"VDU Connection", MB_ICONASTERISK);
		//THROW(e);
	}
	END_CATCH;
	pFile->Close();
	con->Close();
	m_session->Close();
}

CVDUConnection::CVDUConnection(CVDUClientDlg* mainWnd, LPCTSTR serverURL, VDUAPIType type)
{
	m_session = NULL;
	StringCchCopy(m_serverURL, ARRAYSIZE(m_serverURL), serverURL);
	m_wnd = mainWnd;
	m_type = type;
}

CVDUConnection::~CVDUConnection()
{
	if (m_session != NULL)
		delete m_session;
}


#define ID_SYSTEMTRAY 0x1000
#define WM_TRAYICON_EVENT (WM_APP + 1)
#define WM_TRAY_EXIT (WM_APP + 2)
#define WM_TRAY_AUTORUN_TOGGLE (WM_APP + 3)
#define WM_TRAY_AUTOLOGIN_TOGGLE (WM_APP + 4)

// CVDUClientDlg dialog
CVDUClientDlg::CVDUClientDlg(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_VDUCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVDUClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVDUClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_CONTEXTMENU()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_TRAYICON_EVENT, &CVDUClientDlg::OnTrayEvent)
	ON_EN_CHANGE(IDC_SERVER_ADDRESS, &CVDUClientDlg::OnEnChangeServerAddress)
	ON_BN_CLICKED(IDC_CONNECT, &CVDUClientDlg::OnBnClickedConnect)
	ON_COMMAND(WM_TRAY_EXIT, &CVDUClientDlg::OnTrayExitCommand)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CVDUClientDlg::OnBnClickedButtonLogin)
	ON_EN_CHANGE(IDC_USERNAME, &CVDUClientDlg::OnEnChangeUsername)
	ON_BN_CLICKED(IDC_CHECK_CERTIFICATE, &CVDUClientDlg::OnBnClickedCheckCertificate)
END_MESSAGE_MAP()

// CVDUClientDlg message handlers
BOOL CVDUClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, WM_TRAY_EXIT, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	ZeroMemory(&m_trayData, sizeof(m_trayData));
	m_trayData.cbSize = sizeof(m_trayData);
	m_trayData.uID = ID_SYSTEMTRAY;
	ASSERT(IsWindow(GetSafeHwnd()));
	m_trayData.hWnd = GetSafeHwnd();
	m_trayData.uCallbackMessage = WM_TRAYICON_EVENT;
	if (StringCchCopy(m_trayData.szTip, ARRAYSIZE(m_trayData.szTip), L"VDU Client") != S_OK)
		return FALSE;
	m_trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_trayData.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_trayData.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_ADD, &m_trayData);
	//Shell_NotifyIcon(NIM_SETVERSION, &m_trayData);

	if (m_trayMenu = new CMenu())
	{
		m_trayMenu->CreatePopupMenu();
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_EXIT, L"Exit");
	}

	if (StringCchCopy(m_server, ARRAYSIZE(m_server), L"") != S_OK)
		return FALSE;

	if (StringCchCopy(m_username, ARRAYSIZE(m_username), L"") != S_OK)
		return FALSE;

	GetRegValueSz(L"LastServerAddress", m_server, m_server, ARRAYSIZE(m_server));
	GetRegValueSz(L"LastUserName", m_username, m_username, ARRAYSIZE(m_username));

	GetDlgItem(IDC_SERVER_ADDRESS)->SetWindowText(m_server);
	GetDlgItem(IDC_USERNAME)->SetWindowText(m_username);

	DWORD autoLogin = FALSE;
	GetRegValueI(L"AutoLogin", autoLogin, &autoLogin);

	DWORD useCertToLogin = FALSE;
	GetRegValueI(L"UseCertToLogin", useCertToLogin, &useCertToLogin);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CVDUClientDlg::GetRegValueSz(LPCTSTR name, LPCTSTR defaultValue, PTCHAR out_value, DWORD maxOutvalueSize)
{
	ULONG type = REG_SZ;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
		{
			MessageBox(L"Failed to read registry", L"VDU Client", MB_ICONASTERISK);
			return FALSE;
		}
	}

	LSTATUS res;
	if ((res = RegQueryValueEx(hkey, name, 0, &type, (LPBYTE)out_value, &maxOutvalueSize)) != ERROR_SUCCESS)
	{
		res = RegSetValueEx(hkey, name, 0, type, (BYTE*)defaultValue, maxOutvalueSize);
	}

	RegCloseKey(hkey);
	return res == ERROR_SUCCESS;
}

BOOL CVDUClientDlg::GetRegValueI(LPCTSTR name, DWORD defaultValue, PDWORD out_value)
{
	ULONG type = REG_DWORD;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
		{
			MessageBox(L"Failed to read registry", L"VDU Client", MB_ICONASTERISK);
			return FALSE;
		}
	}

	DWORD maxOutvalueSize = sizeof(defaultValue);
	LSTATUS res;
	if ((res = RegQueryValueEx(hkey, name, 0, &type, (LPBYTE)out_value, &maxOutvalueSize)) != ERROR_SUCCESS)
	{
		res = RegSetValueEx(hkey, name, 0, type, (BYTE*)&defaultValue, maxOutvalueSize);
	}

	RegCloseKey(hkey);
	return res == ERROR_SUCCESS;
}

BOOL CVDUClientDlg::SetRegValueI(LPCTSTR name, DWORD value)
{
	ULONG type = REG_DWORD;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
		{
			MessageBox(L"Failed to read registry", L"VDU Client", MB_ICONASTERISK);
			return FALSE;
		}
	}

	DWORD maxOutvalueSize = sizeof(value);
	LSTATUS res = RegSetValueEx(hkey, name, 0, type, (BYTE*)&value, maxOutvalueSize);

	RegCloseKey(hkey);
	return res == ERROR_SUCCESS;
}

BOOL CVDUClientDlg::SetRegValueSz(LPCTSTR name, LPCTSTR value)
{
	ULONG type = REG_SZ;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, VDU_REGPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
		{
			MessageBox(L"Failed to read registry", L"VDU Client", MB_ICONASTERISK);
			return FALSE;
		}
	}

	LSTATUS res = RegSetValueEx(hkey, name, 0, type, (BYTE*)value, DWORD(sizeof(wchar_t) * (wcslen(value) + 1)));

	RegCloseKey(hkey);
	return res == ERROR_SUCCESS;
}

void CVDUClientDlg::PostNcDestroy()
{
	Shell_NotifyIcon(NIM_DELETE, &m_trayData);
	CDialogEx::PostNcDestroy();
}

void CVDUClientDlg::SetConnected(BOOL bConnected)
{
	//if (m_connected == bConnected)
	//	return;

	m_connected = bConnected;
	
	if (m_connected)
	{
		GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONNECT)->SetWindowText(L"Disconnect");
		GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
		//GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_CERTIFICATE)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONNECT)->SetWindowText(L"Connect");
		GetDlgItem(IDC_USERNAME)->EnableWindow(FALSE);
		//GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_CERTIFICATE)->EnableWindow(FALSE);
	}
}

BOOL CVDUClientDlg::IsConnected()
{
	return m_connected;
}

BOOL CVDUClientDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CDialogEx::OnCommand(wParam, lParam);
}

void CVDUClientDlg::OnOK()
{

}

void CVDUClientDlg::OnCancel()
{

}

BOOL CVDUClientDlg::TrayTip(LPCTSTR szTip)
{
	m_trayData.uFlags |= NIF_TIP | NIF_SHOWTIP;

	if (StringCchCopy(m_trayData.szTip, ARRAYSIZE(m_trayData.szTip), szTip) != S_OK)
		return FALSE;

	return Shell_NotifyIcon(NIM_MODIFY, &m_trayData);
}

BOOL CVDUClientDlg::TrayNotify(LPCTSTR szTitle, LPCTSTR szText, SHSTOCKICONID siid)
{
	m_trayData.uFlags |= NIF_INFO | NIF_MESSAGE;// | NIF_GUID;
	m_trayData.uTimeout = 1000;
	m_trayData.dwInfoFlags |= NIIF_INFO | NIIF_USER | NIIF_LARGE_ICON;
	//HRESULT hr = CoCreateGuid(&m_trayData.guidItem);

	SHSTOCKICONINFO shii;
	ZeroMemory(&shii, sizeof(shii));
	shii.cbSize = sizeof(shii);
	if (SHGetStockIconInfo(siid, SHGSI_ICON, &shii) != S_OK)
		return FALSE;

	m_trayData.hBalloonIcon = shii.hIcon;

	if (StringCchCopy(m_trayData.szInfoTitle, ARRAYSIZE(m_trayData.szInfoTitle), szTitle) != S_OK)
		return FALSE;

	if (StringCchCopy(m_trayData.szInfo, ARRAYSIZE(m_trayData.szInfo), szText) != S_OK)
		return FALSE;

	BOOL res = Shell_NotifyIcon(NIM_MODIFY, &m_trayData);

	DestroyIcon(m_trayData.hBalloonIcon);

	return res;
}

void CVDUClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0) == SC_MINIMIZE)
	{
		AfxGetMainWnd()->ShowWindow(SW_MINIMIZE);
	}
	else if ((nID & 0xFFF0) == SC_CLOSE)
	{
		AfxGetMainWnd()->ShowWindow(SW_MINIMIZE);
		AfxGetMainWnd()->ShowWindow(SW_HIDE);
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVDUClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVDUClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CVDUClientDlg::OnTrayEvent(WPARAM wParam, LPARAM lParam)
{
	if ((UINT)wParam != ID_SYSTEMTRAY)
		return ERROR_SUCCESS;

	switch ((UINT)lParam)
	{
		case WM_MOUSEMOVE:
		{

			break;
		}
		case WM_LBUTTONUP:
		{
			if (IsIconic())
			{
				AfxGetMainWnd()->ShowWindow(SW_RESTORE);
				AfxGetMainWnd()->SetForegroundWindow();
			}
			break;
		}
		case WM_RBUTTONUP:
		{
			POINT curPoint;
			GetCursorPos(&curPoint);
			AfxGetMainWnd()->SetForegroundWindow();
			m_trayMenu->TrackPopupMenu(GetSystemMetrics(SM_MENUDROPALIGNMENT), curPoint.x, curPoint.y, this);
			AfxGetMainWnd()->PostMessage(WM_NULL, 0, 0);
			break;
		}
	}

	return ERROR_SUCCESS;
}

void CVDUClientDlg::OnTrayExitCommand()
{
	AfxGetMainWnd()->PostMessage(WM_QUIT, 0, 0);
}

void CVDUClientDlg::OnEnChangeServerAddress()
{
	CString serverAddr;
	GetDlgItem(IDC_SERVER_ADDRESS)->GetWindowText(serverAddr);
	SetRegValueSz(L"LastServerAddress", serverAddr);
}

UINT conthreadproc(LPVOID pCon)
{
	CVDUConnection* con = (CVDUConnection*)pCon;
	con->Process();
	delete con;
	return EXIT_SUCCESS;
}


void CVDUClientDlg::OnBnClickedConnect()
{
	if (IsConnected())
	{
		SetConnected(FALSE);
	}
	else
	{
		CString serverAddr;
		GetDlgItem(IDC_SERVER_ADDRESS)->GetWindowText(serverAddr);

		if (serverAddr.IsEmpty())
		{
			MessageBox(L"Invalid server address", VDU_TITLENAME, MB_ICONERROR);
			return;
		}

		AfxBeginThread(conthreadproc, LPVOID(new CVDUConnection(this, serverAddr, VDUAPIType::GET_PING)));
		GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONNECT)->EnableWindow(FALSE);
	}
}


void CVDUClientDlg::OnBnClickedButtonLogin()
{
	// TODO: Add your control notification handler code here
}


void CVDUClientDlg::OnEnChangeUsername()
{
	CString name;
	GetDlgItem(IDC_USERNAME)->GetWindowText(name);
	SetRegValueSz(L"LastUserName", name);
}


void CVDUClientDlg::OnBnClickedCheckCertificate()
{
	BOOL useCert = ((CButton*)GetDlgItem(IDC_CHECK_CERTIFICATE))->GetCheck();

	if (useCert)
	{
		GetDlgItem(IDC_BROWSE_CERT)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_BROWSE_CERT)->EnableWindow(FALSE);
	}
}
