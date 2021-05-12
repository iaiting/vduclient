/**
* 
 * @author xferan00
 * @file VDUClient.cpp
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

#include "pch.h"
#include "framework.h"
#include "VDUClient.h"
#include "VDUClientDlg.h"
#include "VDUSession.h"
#include "VDUFilesystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// VDUClient	
BEGIN_MESSAGE_MAP(VDUClient, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// VDUClient construction
VDUClient::VDUClient() : m_srefThread(nullptr), m_svc(nullptr), m_svcThread(nullptr),
m_testMode(FALSE), m_insecure(FALSE)
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
	
	// Place all significant initialization in InitInstance
	m_session = new CVDUSession(_T(""));
}

VDUClient::~VDUClient()
{
	delete m_session;
}

CVDUSession* VDUClient::GetSession()
{
	return m_session;
}

CWinThread* VDUClient::GetSessionRefreshingThread()
{
	return m_srefThread;
}

CWinThread* VDUClient::GetFileSystemServiceThread()
{
	return m_svcThread;
}

BOOL VDUClient::IsTestMode()
{
	return m_testMode;
}

BOOL VDUClient::IsInsecure()
{
	return m_insecure;
}

CVDUFileSystemService* VDUClient::GetFileSystemService()
{
	return m_svc;
}

// The one and only VDUClient object
VDUClient vduClient;

// VDUClient initialization
BOOL VDUClient::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager* pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	SetRegistryKey(PROJNAME);

	//Check if WinFSP is installed on the system
	CRegKey key;
	if (key.Open(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\WinFsp"), KEY_READ) != ERROR_SUCCESS)
	{
		MessageBox(NULL, _T("WinFsp is not intsalled on the system!\r\nPlease install it from http://www.secfs.net/winfsp/rel/"), TITLENAME, MB_ICONERROR);
		ExitProcess(EXIT_SUCCESS);
	}
	key.Close();

	//Check if an instance is already running
	//Create mailslot for messages
	HANDLE hMailslot = CreateMailslot(S_MAILSLOT, 0, MAILSLOT_WAIT_FOREVER, NULL);
	if (hMailslot == INVALID_HANDLE_VALUE)
	{
		//This is second instance of the process
		//Write the command into the mailslot, the first instance will run it
		hMailslot = CreateFile(S_MAILSLOT, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hMailslot != INVALID_HANDLE_VALUE)
		{
			DWORD writeLen;
			WriteFile(hMailslot, m_lpCmdLine, (DWORD)_tcslen(m_lpCmdLine) * sizeof(*m_lpCmdLine) + sizeof(*m_lpCmdLine), &writeLen, NULL);
			CloseHandle(hMailslot);
		}

		ExitProcess(EXIT_SUCCESS);
	}

	//Acquire the executable path
	CString moduleFilePath;
	AfxGetModuleFileName(NULL, moduleFilePath);

	//Create a new class for the VDU URL protocol
	if (key.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\") URL_PROTOCOL) == ERROR_SUCCESS)
	{
		key.SetStringValue(NULL, _T("URL: VDU Client - Access a file"));
		key.SetStringValue(_T("URL Protocol"), _T(""));	
		key.Close();
	}
	//Set the command to use the `-accessnetfile` instruction
	if (key.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\" URL_PROTOCOL "\\shell\\open\\command")) == ERROR_SUCCESS)
	{
		key.SetStringValue(NULL, _T("\"") + moduleFilePath + _T("\" -accessnetfile \"%1\""));
		key.Close();
	}
	if (key.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\") URL_PROTOCOL "\\DefaultIcon") == ERROR_SUCCESS)
	{
		key.SetStringValue(NULL, _T("\"") + moduleFilePath + _T("\",0"));
		key.Close();
	}

	//Check for startup options
	BOOL c_silent = FALSE;
	int argc;
	if (LPWSTR* argv = CommandLineToArgvW(m_lpCmdLine, &argc))
	{
		for (int i = 0; i < argc; i++)
		{
			LPWSTR arg = argv[i];

			if (!_tcscmp(arg, _T("-silent")))
				c_silent = TRUE;
			else if (!_tcscmp(arg, _T("-testmode")))
				m_testMode = TRUE;
			else if (!_tcscmp(arg, _T("-insecure")))
				m_insecure = TRUE;
		}
		LocalFree(argv);
	}

	CString preferredLetter = APP->GetProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), _T(""));
	if (preferredLetter.IsEmpty())
		APP->WriteProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), _T("V:"));
	preferredLetter = APP->GetProfileString(SECTION_SETTINGS, _T("PreferredDriveLetter"), _T(""));

	//Create worker thread
	m_srefThread = AfxBeginThread(ThreadProcLoginRefresh, (LPVOID)nullptr);

	//Dont create dialog in test mode
	if (!IsTestMode())
	{
		CVDUClientDlg* pDlg = new CVDUClientDlg();
		m_pMainWnd = pDlg;
		if (pDlg->Create(IDD_VDUCLIENT_DIALOG, AfxGetMainWnd()))
		{
			if (IsTestMode())
			{
				pDlg->ShowWindow(SW_HIDE);
			}
			else if (c_silent)
			{
				pDlg->ShowWindow(SW_MINIMIZE);
				pDlg->ShowWindow(SW_HIDE);
			}
			else
			{
				pDlg->ShowWindow(SW_SHOWNORMAL);
			}
		}
		else
		{
			//Failed to create dialog?
			return FALSE;
		}
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	//Start the file system service
	m_svcThread = AfxBeginThread(ThreadProcFilesystemService, (LPVOID)(m_svc = new CVDUFileSystemService(preferredLetter)));

	//Make sure the file system service is started
	ULONGLONG startTicks = GetTickCount64();
	while (!PathFileExists(GetFileSystemService()->GetDrivePath()))
	{
		ULONGLONG timeWaited = GetTickCount64() - startTicks;
		if (timeWaited > 10 * 1000)
			ExitProcess(-3);
	}

	//In test mode we execute input actions and quit with proper code
	if (IsTestMode())
	{
		HandleCommands(m_lpCmdLine, FALSE);
		
		//Implicit quit after all test actions have been done
		ExitProcess(EXIT_SUCCESS);
	}

	//Start reading mailslot messages
	AfxBeginThread(ThreadProcMailslot, (LPVOID)hMailslot);

	//Write current command line for execution
	hMailslot = CreateFile(S_MAILSLOT, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hMailslot != INVALID_HANDLE_VALUE)
	{
		DWORD writeLen;
		WriteFile(hMailslot, m_lpCmdLine, (DWORD)_tcslen(m_lpCmdLine) * sizeof(*m_lpCmdLine) + sizeof(*m_lpCmdLine), &writeLen, NULL);
		CloseHandle(hMailslot);
	}

	return TRUE;
}

INT VDUClient::ExitInstance()
{
	if (auto* s = GetSession())
		if (s->IsLoggedIn())
			AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)new CVDUConnection(s->GetServerURL(), VDUAPIType::DELETE_AUTH_KEY));

	GetFileSystemService()->Stop();

	if (m_pMainWnd)
		WND->DestroyWindow();
	return CWinApp::ExitInstance();
}

void VDUClient::HandleCommands(LPCWSTR cmdline, BOOL async)
{
	int argc;
	if (TCHAR** argv = CommandLineToArgvW(cmdline, &argc))
	{
		for (int i = 0; i < argc; i++)
		{
			TCHAR* arg = argv[i];
			INT result;

			if (!_tcscmp(arg, _T("-server")))
			{
				CMDLINE_ASSERT_ARGC(argc, i);
				TCHAR* server = argv[++i];

				GetSession()->Reset(server);
			}
			else if (!_tcscmp(arg, _T("-user")))
			{
				CMDLINE_ASSERT_ARGC(argc, i);
				TCHAR* user = argv[++i];

				result = GetSession()->Login(user, _T(""), async);
				if (result != EXIT_SUCCESS && APP->IsTestMode())
				{
					ExitProcess(result);
				}
			}
			else if (!_tcscmp(arg, _T("-logout")))
			{
				result = GetSession()->Logout(async);
				if (result != EXIT_SUCCESS && APP->IsTestMode())
				{
					ExitProcess(result);
				}
			}
			else if (!_tcscmp(arg, _T("-accessfile")))
			{
				CMDLINE_ASSERT_ARGC(argc, i);
				TCHAR* token = argv[++i];

				result = GetSession()->AccessFile(token, async);
				if (result != EXIT_SUCCESS && APP->IsTestMode())
				{
					ExitProcess(result);
				}
			}
			else if (!_tcscmp(arg, _T("-accessnetfile")))
			{
				CMDLINE_ASSERT_ARGC(argc, i);
				TCHAR* token = argv[++i];

				CString parsedToken = token;
				parsedToken = parsedToken.Right(parsedToken.GetLength() - 4);

				//Parse url backslashes
				while (parsedToken.GetLength() > 0 && parsedToken.GetAt(0) == '/')
					parsedToken = parsedToken.Right(parsedToken.GetLength() - 1);

				while (parsedToken.GetLength() > 0 && parsedToken.GetAt(parsedToken.GetLength() - 1) == '/')
					parsedToken = parsedToken.Left(parsedToken.GetLength() - 1);

				if (parsedToken.IsEmpty())
					continue;

				result = GetSession()->AccessFile(parsedToken, async);
				if (result != EXIT_SUCCESS && APP->IsTestMode())
				{
					ExitProcess(result);
				}
			}
			else if (!_tcscmp(arg, _T("-deletefile")))
			{
				CMDLINE_ASSERT_ARGC(argc, i);
				TCHAR* token = argv[++i];

				CVDUFile vdufile = GetFileSystemService()->GetVDUFileByToken(token);

				result = GetFileSystemService()->DeleteVDUFile(vdufile, async);
				if (result != EXIT_SUCCESS && APP->IsTestMode())
				{
					ExitProcess(result);
				}

				//Make sure file gets deleted
				GetFileSystemService()->DeleteFileInternal(token);
			}
			else if (!_tcscmp(arg, _T("-rename")))
			{
				CMDLINE_ASSERT_ARGC(argc, i + 1);
				TCHAR* token = argv[++i];
				TCHAR* name = argv[++i];

				CVDUFile vdufile = GetFileSystemService()->GetVDUFileByToken(token);

				result = GetFileSystemService()->UpdateVDUFile(vdufile, name, async);
				if (result != EXIT_SUCCESS && APP->IsTestMode())
				{
					ExitProcess(result);
				}
			}
			else if (!_tcscmp(arg, _T("-write")))
			{
				CMDLINE_ASSERT_ARGC(argc, i + 1);
				TCHAR* token = argv[++i];
				TCHAR* text = argv[++i];

				CVDUFile vdufile = GetFileSystemService()->GetVDUFileByToken(token);

				TRY
				{
					//Writing unicode text to a file
					CStdioFile stdf(GetFileSystemService()->GetDrivePath() + vdufile.m_name,
						CFile::modeWrite | CFile::typeText | CFile::shareDenyNone);

					stdf.WriteString(text);
					stdf.Flush();
					stdf.Close();

					result = GetFileSystemService()->UpdateVDUFile(vdufile, _T(""), async);
					if (result != EXIT_SUCCESS && APP->IsTestMode())
					{
						ExitProcess(result);
					}
				}
				CATCH(CException, e)
				{
					if (!async && APP->IsTestMode())
						ExitProcess(EXIT_FAILURE);
				}
				END_CATCH
			}
			else if (!_tcscmp(arg, _T("-read")))
			{
				CMDLINE_ASSERT_ARGC(argc, i + 1);
				TCHAR* token = argv[++i];
				TCHAR* text = argv[++i];

				CVDUFile vdufile = GetFileSystemService()->GetVDUFileByToken(token);

				TRY
				{
					CStdioFile stdf(GetFileSystemService()->GetDrivePath() + vdufile.m_name,
					CFile::modeRead | CFile::typeText | CFile::shareDenyNone);

					//Reading unicode text from a file and compare it to input
					CString fileContent;
					stdf.ReadString(fileContent);
					stdf.Close();

					//Compare only the exact amount of characters
					fileContent = fileContent.Left((INT)_tcslen(text));
					if (!async && text != fileContent && APP->IsTestMode())
					{
						ExitProcess(EXIT_FAILURE);
					}
				}
				CATCH(CException, e)
				{
					if (!async && APP->IsTestMode())
						ExitProcess(EXIT_FAILURE);
				}
				END_CATCH
			}
		}
		LocalFree(argv);
	}

}

UINT VDUClient::ThreadProcMailslot(LPVOID slothandle)
{
	HANDLE hSlot = (HANDLE)slothandle;
	TCHAR msgBuf[250] = { 0 };

	while (TRUE)
	{
		Sleep(500);

		DWORD nextSize;
		DWORD msgCount;
		if (!GetMailslotInfo(hSlot, 0, &nextSize, &msgCount, NULL))
			continue;

		if (nextSize == MAILSLOT_NO_MESSAGE)
			continue;

		DWORD readBytes;
		if (!ReadFile(hSlot, msgBuf, min(nextSize, ARRAYSIZE(msgBuf) - 1), &readBytes, NULL))
			continue;

		//Read all the launch options from message
		APP->HandleCommands(msgBuf, FALSE);
		SecureZeroMemory(msgBuf, sizeof(msgBuf));
	}
}

UINT VDUClient::ThreadProcFilesystemService(LPVOID service)
{
	CVDUFileSystemService* svc = (CVDUFileSystemService*) service;
	ASSERT(svc);
	//Run the service here forever
	return svc->Run();
}

UINT VDUClient::ThreadProcLoginRefresh(LPVOID)
{
	while (TRUE)
	{
		VDU_SESSION_LOCK;

		CTime expires = session->GetAuthTokenExpires();

		//Session is not set up yet, we wait
		if (expires <= 0)
		{
			VDU_SESSION_UNLOCK;
			Sleep(1000);
			continue;
		}

		//NOTE: CTime::GetCurrentTime() is offset by timezone, do not use
		SYSTEMTIME cstime;
		SecureZeroMemory(&cstime, sizeof(cstime));
		GetSystemTime(&cstime);
		CTime now(cstime);
		CTimeSpan delta = expires - now;

		//Sleep till its time to refresh or refresh immediately
		if (delta < 3)
		{
			CWinThread* refreshThread = AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)
				(new CVDUConnection(session->GetServerURL(), VDUAPIType::GET_AUTH_KEY, CVDUSession::CallbackLoginRefresh)),0,0,CREATE_SUSPENDED);

			ASSERT(refreshThread);
			refreshThread->m_bAutoDelete = FALSE;
			DWORD resumeResult = refreshThread->ResumeThread();

			VDU_SESSION_UNLOCK;

			//We wait for our refreshing thread to finish
			if (resumeResult != 0xFFFFFFFF) //Should not happen, but dont get stuck
				WaitForSingleObject(refreshThread->m_hThread, INFINITE);

			DWORD exitCode = 0;
			GetExitCodeThread(refreshThread->m_hThread, &exitCode);

			//With m_bAutoDelete we are responsibile for deleting the thread
			delete refreshThread;

			if (exitCode == 2) //Failed to refresh due to connection, we wait for a bit
			{
				Sleep(4000);
			}
		}
		else //Sleep until its time to check again
		{
			VDU_SESSION_UNLOCK;
			Sleep(1000);
			continue;
		}
	}

	return EXIT_SUCCESS;
}