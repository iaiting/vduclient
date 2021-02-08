// VDUClient.h : main header file for the application
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "VDUFilesystem.h"


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
	CWinThread* m_refreshThread; //Session refreshing thread
	CWinThread* m_svcThread; //File system thread
	CVDUFileSystemService* m_svc; //File system pointer, running on m_svcThread
public:
	VDUClient();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern VDUClient vduClient;
