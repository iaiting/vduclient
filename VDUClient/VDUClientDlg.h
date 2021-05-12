/**
*
 * @author Adam Feranec
 * @file VDUClientDlg.h
 * 
 * This project is licensed under GPLv3, as it includes a modification
 * of work of WinFsp - Windows File System Proxy , Copyright (C) Bill Zissimopoulos.
 * GitHub page at https://github.com/billziss-gh/winfsp, Website at https://www.secfs.net/winfsp/.
 * The original file https://github.com/billziss-gh/winfsp/blob/master/tst/passthrough-cpp/passthrough-cpp.cpp
 *  was modified into two:
 * https://github.com/coolguy124/vduclient/blob/master/VDUClient/VDUFilesystem.cpp
 * https://github.com/coolguy124/vduclient/blob/master/VDUClient/VDUFilesystem.h
 * by Adam Feranec, dates and details noted in said files.
 * @copyright 2015-2020 Bill Zissimopoulos
 *
 */

#pragma once

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "VDUSession.h"

#define VDU_REGPATH _T("SOFTWARE\\VDU Client")

typedef struct StartupApprovedEntry_t
{
	enum FlagBits
	{
		DISABLED = (1 << 0), //The application will not start
		TASKMGR_VIEWED = (1 << 1), //The entry was viewed in the Task Manager
	};

	DWORD flags; //Contains flag bits
	FILETIME disabledTime; //Time when the entry was disabled
} StartupApprovedEntry;

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
	CString m_certPath;
	HICON m_hIcon;
	CMenu* m_trayMenu;
	NOTIFYICONDATA m_trayData;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCancel();
	afx_msg void OnOK();
	afx_msg void PostNcDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg BOOL OnQueryEndSession();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg LRESULT OnTrayEvent(WPARAM wParam, LPARAM lParam);
	afx_msg void OnAutorunToggleCommand();
	afx_msg void OnOpenDriveCommand();
	afx_msg void OnAutologinToggleCommand();
	DECLARE_MESSAGE_MAP()
public:
	//Creates a notification bubble
	BOOL TrayNotify(CString szTitle, CString szText, SHSTOCKICONID siid = SIID_DRIVENET);
	//Sets the tray hover tip
	BOOL SetTrayTip(CString szTip);

	//Bottom progress bar
	CProgressCtrl* GetProgressBar();

	//Attempts to ping the server
	void TryPing();

	//Uses certificate to log in
	BOOL IsLoginUsingCertificate();

	//Updates the client status, seen in tray icon and in the window
	void UpdateStatus();

	//Non-blocking MessageBox 
	void MessageBoxNB(CString text, CString title, UINT flags);
public:
	afx_msg void OnTrayExitCommand();
	afx_msg void OnEnChangeServerAddress();
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnEnChangeUsername();
	afx_msg void OnBnClickedCheckCertificate();
	afx_msg void OnCbnSelchangeComboDriveletter();
	afx_msg void OnBnClickedPing();
	afx_msg void OnBnClickedButtonCertselect();
	afx_msg void OnBnClickedAccessFile();
	afx_msg void OnEnSetfocusFileToken();
};
