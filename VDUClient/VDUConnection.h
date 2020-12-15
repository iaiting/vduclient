#pragma once

#include <afxinet.h>

class CVDUClientDlg;

//Declares valid VDU api types
enum class VDUAPIType
{
	GET_PING, //Ping the server
	GET_AUTH_KEY, //Refresh auth key
	POST_AUTH_KEY, //Get auth key for the first time
	DELETE_AUTH_KEY, //Invalidate auth key
	GET_FILE, //Download file
	POST_FILE, //Upload file
	DELETE_FILE, //Invalidate file token
};

typedef void (*VDU_CONNECTION_CALLBACK)(CVDUClientDlg* wnd, CHttpFile* httpResponse);

//A single connection to a VDU server, halts thread it is executed on
class CVDUConnection
{
protected:
	CVDUClientDlg* m_wnd; //Main window
	TCHAR m_serverURL[INTERNET_MAX_HOST_NAME_LENGTH]; //Server url to connect to
	CInternetSession* m_session; //Current internet session
	VDUAPIType m_type; //Which API to call
	TCHAR m_parameter[INTERNET_MAX_HOST_NAME_LENGTH]; //Http path parameter
	VDU_CONNECTION_CALLBACK m_callback; //Function to call after http file is received
public:
	//Sets up the connection
	CVDUConnection(CVDUClientDlg* mainWnd, LPCTSTR serverURL, VDUAPIType type, LPCTSTR parameter, VDU_CONNECTION_CALLBACK callback);

	//Processes the connection and halts executing thread until done
	//Should NOT be run in main thread
	void Process();

	//Starting thread procedure for new threads, expects connection created by 'new'
	static UINT ThreadProc(LPVOID pCon);
};