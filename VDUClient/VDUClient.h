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

//VDUClient CWinApp 
#define APP ((VDUClient*)AfxGetApp())
//Main window of program
#define WND ((CVDUClientDlg*)APP->GetMainWnd())

//Locks the session to be used by current thread, use UNLOCK when done
#define VDU_SESSION_LOCK CVDUSession* session = APP->GetSession();AcquireSRWLockExclusive(&session->m_lock)
//Unlocks session for other threads to use, do not forget this
#define VDU_SESSION_UNLOCK ReleaseSRWLockExclusive(&APP->GetSession()->m_lock);

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

	//Only use when you have exclusive access to the session by locking
	//Otherwise you risk out-of-date information
	CVDUSession* GetSession();

	//Refreshes session auth token when its about to expire
	CWinThread* GetSessionRefreshingThread();

	//Handles the filesystem service
	CWinThread* GetFileSystemServiceThread();

	//Allows operations with the filesystem service
	CVDUFileSystemService* GetFileSystemService();

//Overrides

	//For initializing core of the program
	virtual BOOL InitInstance();

	//Making sure window exits properly
	virtual INT ExitInstance();

//Thread Procedures
	static UINT ThreadProcFilesystemService(LPVOID service);
	static UINT ThreadProcLoginRefresh(LPVOID);

//Implementation
	DECLARE_MESSAGE_MAP()
};

extern VDUClient vduClient;
