#pragma once

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "VDUSession.h"

#define VDU_REGPATH _T("SOFTWARE\\VDU Client")

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
	CString m_server;
	CString m_username;
	CString m_statusText;
	BOOL m_connected;
	HICON m_hIcon;
	CMenu* m_trayMenu;
	NOTIFYICONDATA m_trayData;
	CProgressCtrl* m_progressBar;
	CVDUSession* m_session;

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
	afx_msg void OnAutorunToggleCommand();
	afx_msg void OnAutologinToggleCommand();
	DECLARE_MESSAGE_MAP()
public:
	/*Registry handling functions*/
	BOOL GetRegValueI(LPCTSTR name, DWORD defaultValue, PDWORD out_value, LPCTSTR path = VDU_REGPATH);
	BOOL SetRegValueI(LPCTSTR name, DWORD value, LPCTSTR path = VDU_REGPATH);
	BOOL GetRegValueSz(LPCTSTR name, LPCTSTR defaultValue, PTCHAR out_value, DWORD maxOutvalueSize, LPCTSTR path = VDU_REGPATH, ULONG type = REG_SZ);
	BOOL SetRegValueSz(LPCTSTR name, LPCTSTR value, LPCTSTR path = VDU_REGPATH, ULONG type = REG_SZ);

	//Creates a notification bubble
	BOOL TrayNotify(LPCTSTR szTitle, LPCTSTR szText, SHSTOCKICONID siid = SIID_DRIVENET);
	//Sets the tray hover tip
	BOOL TrayTip(LPCTSTR szTip);

	//Bottom progress bar
	CProgressCtrl* GetProgressBar();
	//Return VDU session
	CVDUSession* GetSession();

	void TryConnectSession();
	//Is connected to the server
	void SetConnected(BOOL bConnected);
	BOOL IsConnected();

	//Uses certificate to log in
	BOOL IsLoginUsingCertificate();

	//Set the client status text, in multiple places
	BOOL SetStatus(LPCTSTR szText);

public:
	afx_msg void OnEnChangeServerAddress();
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnEnChangeUsername();
	afx_msg void OnBnClickedCheckCertificate();
	afx_msg void OnCbnSelchangeComboDriveletter();
};
