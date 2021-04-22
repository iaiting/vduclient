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
#define WM_TRAY_OPEN_DRIVE (WM_APP + 5)

// CVDUClientDlg dialog
CVDUClientDlg::CVDUClientDlg(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_VDUCLIENT_DIALOG, pParent),
m_trayData{0}, m_trayMenu(NULL)
{
	m_hIcon = APP->LoadIcon(IDR_MAINFRAME);
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
	ON_COMMAND(WM_TRAY_EXIT, &CVDUClientDlg::OnTrayExitCommand)
	ON_COMMAND(WM_TRAY_AUTOLOGIN_TOGGLE, &CVDUClientDlg::OnAutologinToggleCommand)
	ON_COMMAND(WM_TRAY_AUTORUN_TOGGLE, &CVDUClientDlg::OnAutorunToggleCommand)
	ON_COMMAND(WM_TRAY_OPEN_DRIVE, &CVDUClientDlg::OnOpenDriveCommand)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CVDUClientDlg::OnBnClickedButtonLogin)
	ON_EN_CHANGE(IDC_USERNAME, &CVDUClientDlg::OnEnChangeUsername)
	ON_CBN_SELCHANGE(IDC_COMBO_DRIVELETTER, &CVDUClientDlg::OnCbnSelchangeComboDriveletter)
	ON_BN_CLICKED(IDC_CHECK_CERTIFICATE, &CVDUClientDlg::OnBnClickedCheckCertificate)
	ON_BN_CLICKED(IDC_BUTTON_PING, &CVDUClientDlg::OnBnClickedPing)
	ON_BN_CLICKED(IDC_BUTTON_CERTSELECT, &CVDUClientDlg::OnBnClickedButtonCertselect)
	ON_BN_CLICKED(IDC_ACCESS_FILE, &CVDUClientDlg::OnBnClickedAccessFile)
	ON_EN_SETFOCUS(IDC_FILE_TOKEN, &CVDUClientDlg::OnEnSetfocusFileToken)
END_MESSAGE_MAP()

// CVDUClientDlg message handlers
BOOL CVDUClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	GetProgressBar()->SetRange(0, 100);
	GetProgressBar()->SetState(PBST_PAUSED);

	//Set up tray icon
	SecureZeroMemory(&m_trayData, sizeof(m_trayData));
	m_trayData.cbSize = sizeof(m_trayData);
	m_trayData.uID = ID_SYSTEMTRAY;
	ASSERT(IsWindow(GetSafeHwnd()));
	m_trayData.hWnd = GetSafeHwnd();
	m_trayData.uCallbackMessage = WM_TRAYICON_EVENT;
	StringCchCopy(m_trayData.szTip, ARRAYSIZE(m_trayData.szTip), TITLENAME);
	m_trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_trayData.hIcon = APP->LoadIcon(IDR_MAINFRAME);
	m_trayData.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_ADD, &m_trayData);

	CString moduleFilePath;
	AfxGetModuleFileName(NULL, moduleFilePath);
	CString moduleFileName = PathFindFileName(moduleFilePath);
	CString moduleFolderPath = moduleFilePath.Left(moduleFilePath.GetLength() - moduleFileName.GetLength() - 1);

	//Create an entry on windows startup
	CRegKey key;
	if (key.Open(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")) == ERROR_SUCCESS)
	{
		key.SetStringValue(TITLENAME, moduleFilePath + _T(" -silent"));
		key.Close();
	}

	//Allows application to be searched for and opened using windows shell/search
	if (key.Open(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\") + moduleFileName) == ERROR_SUCCESS)
	{
		key.SetStringValue(NULL, moduleFilePath);
		key.SetStringValue(_T("Path"), moduleFolderPath);
		key.Close();
	}

	//Create tray popup menu
	if (m_trayMenu = new CMenu())
	{
		m_trayMenu->CreatePopupMenu();
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_AUTORUN_TOGGLE, _T("Run on startup")); //0
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_AUTOLOGIN_TOGGLE, _T("Auto Login")); //1
		m_trayMenu->AppendMenu(MF_SEPARATOR); //2
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_OPEN_DRIVE, _T("Browse files")); //3
		m_trayMenu->AppendMenu(MF_STRING, WM_TRAY_EXIT, _T("Exit")); //4

		if (APP->GetProfileInt(SECTION_SETTINGS, _T("AutoLogin"), FALSE))
			m_trayMenu->CheckMenuItem(1, MF_BYPOSITION | MF_CHECKED);

		if (key.Open(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run")) == ERROR_SUCCESS)
		{
			BYTE startupBuf[0x100] = { 0 };
			ULONG startupBufLen = ARRAYSIZE(startupBuf);
			if (key.QueryBinaryValue(TITLENAME, startupBuf, &startupBufLen) == ERROR_SUCCESS)
			{
				StartupApprovedEntry* entry = reinterpret_cast<StartupApprovedEntry*>(startupBuf);
				if (entry->state == StartupApprovedState::ENABLED)
				{
					m_trayMenu->CheckMenuItem(0, MF_BYPOSITION | MF_CHECKED);
				}
			}
			key.Close();
		}
	}

	m_server = APP->GetProfileString(SECTION_SETTINGS, _T("LastServerAddress"), _T(""));
	m_username = APP->GetProfileString(SECTION_SETTINGS, _T("LastUserName"), _T(""));

	GetDlgItem(IDC_SERVER_ADDRESS)->SetWindowText(m_server);
	GetDlgItem(IDC_USERNAME)->SetWindowText(m_username);

	int useCertToLogin = APP->GetProfileInt(SECTION_SETTINGS, _T("UseCertToLogin"), FALSE);
	((CButton*)GetDlgItem(IDC_CHECK_CERTIFICATE))->SetCheck(useCertToLogin);

	if (useCertToLogin)
		GetDlgItem(IDC_BUTTON_CERTSELECT)->EnableWindow(TRUE);

	m_certPath = APP->GetProfileString(SECTION_SETTINGS, _T("ClientCertPath"), _T(""));
	if (!m_certPath.IsEmpty())
	{
		CString certFName = PathFindFileName(m_certPath);
		GetDlgItem(IDC_BUTTON_CERTSELECT)->SetWindowText(certFName);
	}

	GetDlgItem(IDC_BUTTON_PING)->SetFocus();

	//Iterate through all logical drives, display unused ones as options
	//Note: This is only executed on program startup, does not update dynamically
	DWORD drives = GetLogicalDrives();
	if (drives == NULL)
		MessageBox(_T("Failed to read logical drives"), TITLENAME, MB_ICONWARNING);
	else
	{
		const TCHAR* letters[] = { _T("A:"), _T("B:"), _T("C:"), _T("D:"), _T("E:"), _T("F:"), _T("G:"), _T("H:"), _T("I:"), _T("J:"), _T("K:"), _T("L:"),
			_T("M:"), _T("N:"), _T("O:"), _T("P:"), _T("Q:"), _T("R:"), _T("S:"), _T("T:"), _T("U:"), _T("V:"), _T("W:"), _T("X:"), _T("Y:"), _T("Z:") }; //All available drive letters

		CComboBox* comboDriveLetter = (CComboBox*)GetDlgItem(IDC_COMBO_DRIVELETTER);
		comboDriveLetter->ModifyStyle(0, CBS_DROPDOWNLIST);

		CString preferredLetter = APP->GetProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), _T("V:"));

		for (int i = 0; i < ARRAYSIZE(letters); i++)
		{
			const TCHAR* letter = letters[i];
			BOOL isTaken = (drives & (1 << i)) != 0;

			if (!isTaken)
			{
				comboDriveLetter->AddString(letter);
				if (!_tcscmp(letter, preferredLetter))
				{
					comboDriveLetter->SetCurSel(comboDriveLetter->FindString(-1, letter));
				}
			}
		}

		comboDriveLetter->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_DRIVELETTER)->EnableWindow(TRUE);
	}

	VDU_SESSION_LOCK;
	session->Reset(m_server);
	VDU_SESSION_UNLOCK;

	//Handle autologin
	if (APP->GetProfileInt(SECTION_SETTINGS, _T("AutoLogin"), FALSE))
	{
		OnBnClickedButtonLogin();
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVDUClientDlg::UpdateStatus()
{
	if (APP->IsTestMode())
		return;

	CString trayStatus = TITLENAME;
	CString windowStatus;

	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	if (session->IsLoggedIn())
	{
		trayStatus += _T("\r\nLogged in as " + session->GetUser());
		windowStatus += _T("User: ") + session->GetUser() + _T(" ");

		CString fileCntStr;
		fileCntStr.Format(_T("%llu"), APP->GetFileSystemService()->GetVDUFileCount());

		trayStatus += _T("\r\n" + fileCntStr + _T(" files"));
		windowStatus += _T("| Files: ") + fileCntStr; 
	}

	if (GetProgressBar()->GetState() != PBST_PAUSED)
	{
		BOOL failed = GetProgressBar()->GetState() == PBST_ERROR;
		CString percentage;
		percentage.Format(_T("%d%%"), GetProgressBar()->GetPos());
		trayStatus += failed ? _T("\r\nDownload failed ") : _T("\r\nDownloading ") + percentage;
		windowStatus += failed ? _T(" | Download failed ") : _T(" | Downloading.. ") + percentage;
	}

	if (trayStatus != m_trayData.szTip)
		SetTrayTip(trayStatus);

	CString oldWndText;
	GetDlgItem(IDC_STATIC_STATUS)->GetWindowText(oldWndText);

	if (oldWndText != windowStatus)
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(windowStatus);
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
	return (CProgressCtrl*)GetDlgItem(IDC_PROGRESSBAR);
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

BOOL CVDUClientDlg::SetTrayTip(CString szTip)
{
	m_trayData.uFlags = NIF_TIP;

	if (StringCchCopy(m_trayData.szTip, ARRAYSIZE(m_trayData.szTip), szTip) != S_OK)
		return FALSE;

	return Shell_NotifyIcon(NIM_MODIFY, &m_trayData);
}

BOOL CVDUClientDlg::TrayNotify(CString szTitle, CString szText, SHSTOCKICONID siid)
{
	m_trayData.uFlags = NIF_INFO | NIF_MESSAGE;
	m_trayData.uTimeout = 1000;
	m_trayData.dwInfoFlags |= NIIF_INFO | NIIF_USER | NIIF_LARGE_ICON;

	SHSTOCKICONINFO shii;
	SecureZeroMemory(&shii, sizeof(shii));
	shii.cbSize = sizeof(shii);
	if (SHGetStockIconInfo(siid, SHGSI_ICON, &shii) != S_OK)
		return FALSE;

	//You are responsibile for freeing the icon
	m_trayData.hBalloonIcon = shii.hIcon;

	if (StringCchCopy(m_trayData.szInfoTitle, ARRAYSIZE(m_trayData.szInfoTitle), szTitle) != S_OK)
		return FALSE;

	if (StringCchCopy(m_trayData.szInfo, ARRAYSIZE(m_trayData.szInfo), szText) != S_OK)
		return FALSE;

	BOOL res = Shell_NotifyIcon(NIM_MODIFY, &m_trayData);

	DestroyIcon(shii.hIcon);

	return res;
}

void CVDUClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == SC_CLOSE)
	{
		ShowWindow(SW_MINIMIZE);
		ShowWindow(SW_HIDE);
		return;
	}

	CDialogEx::OnSysCommand(nID, lParam);
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

BOOL CVDUClientDlg::OnQueryEndSession()
{
	TCHAR reason[0x200];
	DWORD reasonLen = ARRAYSIZE(reason);
	if (ShutdownBlockReasonQuery(WND->GetSafeHwnd(), reason, &reasonLen))
	{
		return FALSE; //TODO: DO something with reason?
	}
	return CDialogEx::OnQueryEndSession();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVDUClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CVDUClientDlg::OnClose()
{
	
	CDialogEx::OnClose();
}

LRESULT CVDUClientDlg::OnTrayEvent(WPARAM wParam, LPARAM lParam)
{
	if ((UINT)wParam != ID_SYSTEMTRAY)
		return ERROR_SUCCESS;

	switch ((UINT)lParam)
	{
		case WM_MOUSEMOVE:
		{
			UpdateStatus();
			break;
		}
		case WM_LBUTTONUP:
		{
			if (IsIconic())
			{
				ShowWindow(SW_RESTORE);
			}
			SetForegroundWindow();
			break;
		}
		case WM_RBUTTONUP:
		{
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow();
			m_trayMenu->TrackPopupMenu(GetSystemMetrics(SM_MENUDROPALIGNMENT), curPoint.x, curPoint.y, this);
			PostMessage(WM_NULL, 0, 0);
			break;
		}
	}

	return ERROR_SUCCESS;
}

void CVDUClientDlg::OnTrayExitCommand()
{
	if (APP->GetFileSystemService()->GetVDUFileCount() > 0)
	{
		INT answer = MessageBox(_T("There are still some accesibile files!\r\n\r\nYES - Exit without invalidating file tokens\r\nNO - Be taken to file directory (delete by yourself)\r\nCANCEL - Do nothing"),
			TITLENAME, MB_YESNOCANCEL | MB_ICONINFORMATION);
		if (answer == IDCANCEL)
			return;
		else if (answer == IDNO)
		{
			ShellExecute(WND->GetSafeHwnd(), _T("explore"), APP->GetFileSystemService()->GetDrivePath(), NULL, NULL, SW_SHOWNORMAL);
			return;
		}
	}

	AfxPostQuitMessage(EXIT_SUCCESS);
}

void CVDUClientDlg::OnAutorunToggleCommand()
{
	CRegKey key;
	if (key.Open(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run")) == ERROR_SUCCESS)
	{
		BYTE startupBuf[0x100] = { 0 };
		ULONG startupBufLen = ARRAYSIZE(startupBuf);
		if (key.QueryBinaryValue(TITLENAME, startupBuf, &startupBufLen) == ERROR_SUCCESS)
		{
			StartupApprovedEntry* entry = reinterpret_cast<StartupApprovedEntry*>(startupBuf);
			if (entry->state == StartupApprovedState::ENABLED) //Is enabled
			{
				entry->state = StartupApprovedState::DISABLED;
				SYSTEMTIME st;
				GetSystemTime(&st);
				SystemTimeToFileTime(&st, &entry->disabledTime); //Save the disabled time 

				key.SetBinaryValue(TITLENAME, entry, startupBufLen);
				m_trayMenu->CheckMenuItem(0, MF_BYPOSITION | MF_UNCHECKED);
			}
			else
			{
				entry->state = StartupApprovedState::ENABLED;
				SecureZeroMemory(&entry->disabledTime, sizeof(entry->disabledTime));

				key.SetBinaryValue(TITLENAME, entry, startupBufLen);
				m_trayMenu->CheckMenuItem(0, MF_BYPOSITION | MF_CHECKED);
			}
		}
		key.Close();
	}
}

void CVDUClientDlg::OnOpenDriveCommand()
{
	ShellExecute(WND->GetSafeHwnd(), _T("explore"), APP->GetFileSystemService()->GetDrivePath(), NULL, NULL, SW_SHOWNORMAL);
}

void CVDUClientDlg::OnAutologinToggleCommand()
{
	INT autoLogin = APP->GetProfileInt(SECTION_SETTINGS, _T("AutoLogin"), FALSE);
	autoLogin = !autoLogin;
	APP->WriteProfileInt(SECTION_SETTINGS, _T("AutoLogin"), autoLogin);
	if (autoLogin)
		m_trayMenu->CheckMenuItem(1, MF_BYPOSITION | MF_CHECKED);
	else
		m_trayMenu->CheckMenuItem(1, MF_BYPOSITION | MF_UNCHECKED);
}

void CVDUClientDlg::OnEnChangeServerAddress()
{
	CString serverAddr;
	GetDlgItem(IDC_SERVER_ADDRESS)->GetWindowText(serverAddr);

	m_server = serverAddr;

	APP->WriteProfileString(SECTION_SETTINGS, _T("LastServerAddress"), m_server);

	if (!m_server.IsEmpty())
	{
		GetDlgItem(IDC_BUTTON_PING)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_BUTTON_PING)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);
	}

	VDU_SESSION_LOCK;
	session->Reset(m_server);
	VDU_SESSION_UNLOCK;
}

void CVDUClientDlg::TryPing()
{
	AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)new CVDUConnection(m_server, VDUAPIType::GET_PING, CVDUSession::CallbackPing));
}

void CVDUClientDlg::OnBnClickedButtonLogin()
{
	//VDU_SESSION_LOCK;
	CVDUSession* session = APP->GetSession();

	if (!session->IsLoggedIn()) //Loggin in 
	{
		CString user;
		GetDlgItem(IDC_USERNAME)->GetWindowText(user);

		session->Login(user, IsLoginUsingCertificate() ? m_certPath : _T(""));
	}
	else //Log out
	{
		session->Logout();
	}

	//VDU_SESSION_UNLOCK;


	((CEdit*)GetDlgItem(IDC_SERVER_ADDRESS))->SetSel(-1, FALSE);
	((CEdit*)GetDlgItem(IDC_USERNAME))->SetSel(-1, FALSE);
	GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_PING)->EnableWindow(FALSE);
	GetDlgItem(IDC_SERVER_ADDRESS)->EnableWindow(FALSE);
	GetDlgItem(IDC_USERNAME)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC_SERVERADDRESS)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(FALSE);
}

void CVDUClientDlg::OnEnChangeUsername()
{
	CString name;
	GetDlgItem(IDC_USERNAME)->GetWindowText(name);
	m_username = name;
	APP->WriteProfileString(SECTION_SETTINGS, _T("LastUserName"), name);
}

void CVDUClientDlg::OnBnClickedCheckCertificate()
{
	if (IsLoginUsingCertificate())
	{
		GetDlgItem(IDC_BUTTON_CERTSELECT)->EnableWindow(TRUE);
		APP->WriteProfileInt(SECTION_SETTINGS, _T("UseCertToLogin"), TRUE);
	}
	else
	{
		GetDlgItem(IDC_BUTTON_CERTSELECT)->EnableWindow(FALSE);
		APP->WriteProfileInt(SECTION_SETTINGS, _T("UseCertToLogin"), FALSE);
	}
}


void CVDUClientDlg::OnCbnSelchangeComboDriveletter()
{
	CComboBox* comboDriveLetter = (CComboBox*)GetDlgItem(IDC_COMBO_DRIVELETTER);
	CString letter;
	comboDriveLetter->GetLBText(comboDriveLetter->GetCurSel(), letter);
	CString oldLetter = APP->GetProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), letter);
	APP->WriteProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), letter);
	ASSERT(APP->GetFileSystemService());

	//On change we remount
	if (oldLetter != letter)
		APP->GetFileSystemService()->Remount(letter);
}

void CVDUClientDlg::OnBnClickedPing()
{
	TryPing();
	GetDlgItem(IDC_BUTTON_PING)->EnableWindow(FALSE);
}

void CVDUClientDlg::OnBnClickedButtonCertselect()
{
	ShowWindow(SW_MINIMIZE);

	CString filter = _T("CRT Files (*.crt)|*.crt|All Files (*.*)|*.*||");
	CFileDialog fd(TRUE, _T("crt"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter, NULL);
	fd.GetOFN().lpstrTitle = _T("Select client secret file");
	if (fd.DoModal() == IDOK)
	{
		m_certPath = fd.GetPathName();
		CString certFName = PathFindFileName(m_certPath);
		if (certFName.IsEmpty() || GetFileAttributes(m_certPath) == 0xFFFFFFFF)
		{
			MessageBox(_T("Invalid cert file!"), TITLENAME, MB_ICONERROR);
			m_certPath = _T("");
			GetDlgItem(IDC_BUTTON_CERTSELECT)->SetWindowText(_T("Select cert.."));
			return;
		}

		APP->WriteProfileString(SECTION_SETTINGS, _T("ClientCertPath"), m_certPath);
		GetDlgItem(IDC_BUTTON_CERTSELECT)->SetWindowText(certFName);
	}
	else
	{
		//Failed to spawn file dialog?
	}

	//Modal dialogs for modeless dialogs result in a loss of focus
	//Need to manually restore focus to the main dialog window
	ShowWindow(SW_HIDE);
	ShowWindow(SW_RESTORE);
	ShowWindow(SW_SHOWNORMAL);
	SetFocus();
	SetForegroundWindow();
}

void CVDUClientDlg::OnBnClickedAccessFile()
{
	CString fileToken;
	GetDlgItem(IDC_FILE_TOKEN)->GetWindowText(fileToken);

	if (fileToken.IsEmpty())
	{
		AfxMessageBox(_T("Please input the file token."), MB_ICONWARNING);
		return;
	}
	else if (APP->GetFileSystemService()->GetVDUFileByToken(fileToken) != CVDUFile::InvalidFile)
	{
		MessageBox(_T("File already accessed"), TITLENAME, MB_ICONINFORMATION);
		return;
	}

	//VDU_SESSION_LOCK not needed? Access token is filled in last anyway..
	CVDUSession* session = APP->GetSession();
	ASSERT(session);

	session->AccessFile(fileToken);

	GetDlgItem(IDC_FILE_TOKEN)->EnableWindow(FALSE);
	GetDlgItem(IDC_ACCESS_FILE)->EnableWindow(FALSE);
}


void CVDUClientDlg::OnEnSetfocusFileToken()
{
}

//HACK
//I really wanna use messageboxes from worker threads so here we are..
typedef struct MsgBoxParams_t
{
public:
	CString text;
	CString title;
	UINT flags;
} MsgBoxParams_s;

//Doesnt block the main thread! WOW!
UINT ThreadProcMsgBoxNB(LPVOID mbp0)
{
	MsgBoxParams_s* mbp = (MsgBoxParams_s*)mbp0;
	WND->ShowWindow(SW_RESTORE);
	WND->SetForegroundWindow();
	WND->MessageBoxW(mbp->text, mbp->title, mbp->flags);
	delete mbp;
	return EXIT_SUCCESS;
}

void CVDUClientDlg::MessageBoxNB(CString text, CString title, UINT flags)
{
	if (APP->IsTestMode())
		return;

	MsgBoxParams_s* mbp = new MsgBoxParams_s;
	mbp->text = CString(text);
	mbp->title = CString(title);
	mbp->flags = flags;

	AfxBeginThread(ThreadProcMsgBoxNB, (LPVOID)mbp);
}