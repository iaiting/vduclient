// VDUClient.h : main header file for the application
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "VDUFilesystem.h"
#include "VDUSession.h"


#define VFSNAME _T("VDU")
#define SECTION_SETTINGS _T("Settings")
#define TITLENAME _T("VDU Client")
#define APP ((VDUClient*)AfxGetApp())
#define WND ((CVDUClientDlg*)APP->GetMainWnd())

// VDUClient:
// See VDUClient.cpp for the implementation of this class
//
class VDUClient : public CWinApp
{
private:
	CVDUSession* m_session; //Client session
	CWinThread* m_srefThread; //Session refresh thread
	CWinThread* m_svcThread; //File system thread
	CVDUFileSystemService* m_svc; //File system pointer, running on m_svcThread
public:
	VDUClient();
	~VDUClient();

	CVDUSession* GetSession();
	CWinThread* GetSessionRefreshingThread();
	CWinThread* GetFileSystemServiceThread();

// Overrides
	virtual BOOL InitInstance();
	virtual INT ExitInstance();

// Thread Procedures
	static UINT ThreadProcFilesystemService(LPVOID service);
	static UINT ThreadProcLoginRefresh(LPVOID);

// Implementation
	DECLARE_MESSAGE_MAP()
};

extern VDUClient vduClient;
