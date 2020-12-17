#include "pch.h"
#include "framework.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "afxdialogex.h"

#define ID_SYSTEMTRAY 0x1000
#define WM_TRAYICON_EVENT (WM_APP + 1)
#define WM_TRAY_EXIT (WM_APP + 2)
#define WM_TRAY_AUTORUN_TOGGLE (WM_APP + 3)
#define WM_TRAY_AUTOLOGIN_TOGGLE (WM_APP + 4)

// CVDUClientDlg dialog
CVDUClientDlg::CVDUClientDlg(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_VDUCLIENT_DIALOG, pParent), m_progressBar(nullptr),
m_connected(FALSE), m_server{0}, m_statusText{0}  
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
	ON_COMMAND(WM_TRAY_AUTOLOGIN_TOGGLE, &CVDUClientDlg::OnAutologinToggleCommand)
	ON_COMMAND(WM_TRAY_AUTORUN_TOGGLE, &CVDUClientDlg::OnAutorunToggleCommand)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CVDUClientDlg::OnBnClickedButtonLogin)
	ON_EN_CHANGE(IDC_USERNAME, &CVDUClientDlg::OnEnChangeUsername)
	ON_CBN_SELCHANGE(IDC_COMBO_DRIVELETTER, &CVDUClientDlg::OnCbnSelchangeComboDriveletter)
	ON_BN_CLICKED(IDC_CHECK_CERTIFICATE, &CVDUClientDlg::OnBnClickedCheckCertificate)
END_MESSAGE_MAP()

// CVDUClientDlg message handlers
BOOL CVDUClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_progressBar = (CProgressCtrl*)GetDlgItem(IDC_PROGRESSBAR);
	m_progressBar->SetRange(0, 100);

	BOOL silent = FALSE;

	//Check for startup options
	int argc;
	LPWSTR* argv = CommandLineToArgvW(AfxGetApp()->m_lpCmdLine, &argc);
	if (argv == nullptr)
		MessageBox(L"Failed to read command line", VDU_TITLENAME, MB_ICONASTERISK);
	else
	{
		for (int i = 0; i < argc; i++)
		{
			LPWSTR arg = argv[i];

			if (!wcscmp(arg, L"-silent"))
				silent = TRUE;
		}
	}
	LocalFree(argv);

	//No session by default
	m_session = nullptr;

	//Set up tray icon
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
	//Shell_NotifyIcon(NIM_SETVERSION, &m_trayData); //- Not needed in latest version

	//Create tray popup menu
	if (m_trayMenu = new CMenu())
	{
		m_trayMenu->CreatePopupMenu();
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_AUTORUN_TOGGLE, L"Auto-run");
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_AUTOLOGIN_TOGGLE, L"Auto-login");
		m_trayMenu->AppendMenu(MF_SEPARATOR);
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_EXIT, L"Exit");


		DWORD autoLogin = FALSE;
		GetRegValueI(L"AutoLogin", autoLogin, &autoLogin);
		if (autoLogin)
		{
			m_trayMenu->CheckMenuItem(1, MF_BYPOSITION | MF_CHECKED);
		}

		DWORD autoRun = FALSE;
		//GetRegValueI(L"VDU Client", autoRun, &autoRun, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run");

		if (autoRun == 2)
		{
			m_trayMenu->CheckMenuItem(2, MF_BYPOSITION | MF_CHECKED);
		}
	}

	if (StringCchCopy(m_server, ARRAYSIZE(m_server), L"") != S_OK)
		return FALSE;

	if (StringCchCopy(m_username, ARRAYSIZE(m_username), L"") != S_OK)
		return FALSE;

	CString moduleFileName;
	AfxGetModuleFileName(NULL, moduleFileName);

	//Create an entry on windows startup
	SetRegValueSz(L"VDU Client", moduleFileName, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");

	//Pre type last values
	GetRegValueSz(L"LastServerAddress", m_server, m_server, ARRAYSIZE(m_server));
	GetRegValueSz(L"LastUserName", m_username, m_username, ARRAYSIZE(m_username));

	GetDlgItem(IDC_SERVER_ADDRESS)->SetWindowText(m_server);
	GetDlgItem(IDC_USERNAME)->SetWindowText(m_username);

	DWORD useCertToLogin = FALSE;
	GetRegValueI(L"UseCertToLogin", useCertToLogin, &useCertToLogin);
	((CButton*)GetDlgItem(IDC_CHECK_CERTIFICATE))->SetCheck(useCertToLogin);

	GetDlgItem(IDC_CONNECT)->SetFocus();

	if (silent)
	{
		ShowWindow(SW_HIDE);
	}

	DWORD drives = GetLogicalDrives();
	if (drives == NULL)
		MessageBox(L"Failed to read logical drives", VDU_TITLENAME, MB_ICONWARNING);
	else
	{
		const TCHAR* letters[] = { L"A:", L"B:", L"C:", L"D:", L"E:", L"F:", L"G:", L"H:", L"I:", L"J:", L"K:", L"L:",
			L"M:", L"N:", L"O:", L"P:", L"Q:", L"R:", L"S:", L"T:", L"U:", L"V:", L"W:", L"X:", L"Y:", L"Z:" }; //All available drive letters

		CComboBox* comboDriveLetter = (CComboBox*)GetDlgItem(IDC_COMBO_DRIVELETTER);
		comboDriveLetter->ModifyStyle(0, CBS_DROPDOWNLIST);
		TCHAR preferredLetter[128] = {'\0'};
		GetRegValueSz(L"PreferredDriveLetter", preferredLetter, preferredLetter, ARRAYSIZE(preferredLetter));

		for (int i = 0; i < ARRAYSIZE(letters); i++)
		{
			const TCHAR* letter = letters[i];
			BOOL isTaken = (drives & (1 << i)) != 0;

			if (!isTaken)
			{
				comboDriveLetter->AddString(letter);
				if (!wcscmp(letter, preferredLetter))
				{
					comboDriveLetter->SetCurSel(comboDriveLetter->FindString(-1, letter));
				}
			}
		}

		comboDriveLetter->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_DRIVELETTER)->EnableWindow(TRUE);
	}

	SetWindowTheme(GetSafeHwnd(), L"Explorer", NULL);
	SendMessage(WM_THEMECHANGED, 0, 0);

	return !silent;  // return TRUE  unless you set the focus to a control
}

BOOL CVDUClientDlg::GetRegValueSz(LPCTSTR name, LPCTSTR defaultValue, PTCHAR out_value, DWORD maxOutvalueSize, LPCTSTR path, ULONG type)
{
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
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

BOOL CVDUClientDlg::GetRegValueI(LPCTSTR name, DWORD defaultValue, PDWORD out_value, LPCTSTR path)
{
	ULONG type = REG_DWORD;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
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

BOOL CVDUClientDlg::SetRegValueI(LPCTSTR name, DWORD value, LPCTSTR path)
{
	ULONG type = REG_DWORD;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
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

BOOL CVDUClientDlg::SetRegValueSz(LPCTSTR name, LPCTSTR value, LPCTSTR path, ULONG type)
{
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_WRITE | KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, NULL) != ERROR_SUCCESS)
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

BOOL CVDUClientDlg::IsLoginUsingCertificate()
{
	return ((CButton*)GetDlgItem(IDC_CHECK_CERTIFICATE))->GetCheck();
}

CProgressCtrl* CVDUClientDlg::GetProgressBar()
{
	return m_progressBar;
}

void CVDUClientDlg::SetConnected(BOOL bConnected)
{
	//if (bConnected == m_connected)
	//	return;

	m_connected = bConnected;

	if (m_connected)
	{
		GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONNECT)->SetWindowText(L"Disconnect");
		GetDlgItem(IDC_USERNAME)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_CERTIFICATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(FALSE);

		if (m_session)
			delete m_session;

		m_session = new CVDUSession(this, m_server);
	}
	else
	{
		GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONNECT)->SetWindowText(L"Connect");
		GetDlgItem(IDC_USERNAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_CERTIFICATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(TRUE);

		if (m_session)
			delete m_session;

		m_session = nullptr;
	}
}

BOOL CVDUClientDlg::IsConnected()
{
	return m_connected;
}

CVDUSession* CVDUClientDlg::GetSession()
{
	return m_session;
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
	/*if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else*/ 
	if ((nID & 0xFFF0) == SC_MINIMIZE)
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

void CVDUClientDlg::OnAutorunToggleCommand()
{
	//TCHAR autoRun[256];

}

void CVDUClientDlg::OnAutologinToggleCommand()
{
	DWORD autoLogin = FALSE;
	GetRegValueI(L"AutoLogin", autoLogin, &autoLogin);
	autoLogin = !autoLogin;
	SetRegValueI(L"AutoLogin", autoLogin);
	if (autoLogin)
		m_trayMenu->CheckMenuItem(1, MF_BYPOSITION | MF_CHECKED);
	else
		m_trayMenu->CheckMenuItem(1, MF_BYPOSITION | MF_UNCHECKED);
}

void CVDUClientDlg::OnEnChangeServerAddress()
{
	CString serverAddr;
	GetDlgItem(IDC_SERVER_ADDRESS)->GetWindowText(serverAddr);
	SetRegValueSz(L"LastServerAddress", serverAddr);
}

void CVDUClientDlg::TryConnectSession()
{
	/*CWinThread* t = */AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)new CVDUConnection(this, m_server, VDUAPIType::GET_PING, L"", &CVDUSession::CallbackPing));
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

		TryConnectSession();
		GetDlgItem(IDC_CONNECT)->GetFocus();
		GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(FALSE);
	}
}


void CVDUClientDlg::OnBnClickedButtonLogin()
{
	//TODO: REMOVE MOVE
	//AfxBeginThread(vdufs_main, 0);
}

void CVDUClientDlg::OnEnChangeUsername()
{
	CString name;
	GetDlgItem(IDC_USERNAME)->GetWindowText(name);
	SetRegValueSz(L"LastUserName", name);
}

void CVDUClientDlg::OnBnClickedCheckCertificate()
{
	if (IsLoginUsingCertificate())
	{
		GetDlgItem(IDC_BROWSE_CERT)->EnableWindow(TRUE);
		SetRegValueI(L"UseCertToLogin", TRUE);
	}
	else
	{
		GetDlgItem(IDC_BROWSE_CERT)->EnableWindow(FALSE);
		SetRegValueI(L"UseCertToLogin", FALSE);
	}
}


void CVDUClientDlg::OnCbnSelchangeComboDriveletter()
{
	CComboBox* comboDriveLetter = (CComboBox*)GetDlgItem(IDC_COMBO_DRIVELETTER);
	CString letter;
	comboDriveLetter->GetLBText(comboDriveLetter->GetCurSel(), letter);
	SetRegValueSz(L"PreferredDriveLetter", letter);
}