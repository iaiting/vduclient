
// VDUClientDlg.h : header file
//

#pragma once


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
	HICON m_hIcon;
	CMenu* m_trayMenu;
	NOTIFYICONDATA m_trayData;

	//Creates a notification bubble
	BOOL TrayNotify(LPCTSTR szTitle, LPCTSTR szText, SHSTOCKICONID siid = SIID_DRIVENET);
	BOOL TrayTip(LPCTSTR szTip);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void PostNcDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTrayEvent(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrayExitCommand();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeServerAddress();
	afx_msg void OnBnClickedConnect();
};
