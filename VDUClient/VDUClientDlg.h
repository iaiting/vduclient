#pragma once

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define VDU_TITLENAME L"VDU Client"
#define VDU_REGPATH L"SOFTWARE\\VDU Client"

// CVDUClientDlg dialog
class CVDUClientDlg : public CDialogEx
{
// Construction
public:
	CVDUClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VDUCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	TCHAR m_server[256];
	TCHAR m_username[256];
	TCHAR m_statusText[256];
	BOOL m_connected;
	HICON m_hIcon;
	CMenu* m_trayMenu;
	NOTIFYICONDATA m_trayData;

	//UINT conthreadproc(LPVOID);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCancel();
	afx_msg void OnOK();
	afx_msg void PostNcDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTrayEvent(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrayExitCommand();
	DECLARE_MESSAGE_MAP()
public:
	BOOL GetRegValueI(LPCTSTR name, DWORD defaultValue, PDWORD out_value);
	BOOL SetRegValueI(LPCTSTR name, DWORD value);
	BOOL GetRegValueSz(LPCTSTR name, LPCTSTR defaultValue, PTCHAR out_value, DWORD maxOutvalueSize = 0x400);
	BOOL SetRegValueSz(LPCTSTR name, LPCTSTR value);
	//Creates a notification bubble
	BOOL TrayNotify(LPCTSTR szTitle, LPCTSTR szText, SHSTOCKICONID siid = SIID_DRIVENET);
	//Sets the tray hover tip
	BOOL TrayTip(LPCTSTR szTip);

	void SetConnected(BOOL bConnected);
	BOOL IsConnected();

	//Set the client status text, in multiple places
	BOOL SetStatusText(LPCTSTR szText);

public:
	afx_msg void OnEnChangeServerAddress();
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnEnChangeUsername();
	afx_msg void OnBnClickedCheckCertificate();
};
