#pragma once

#include <afxinet.h>

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

//Connection callback with HTTP response as a parameter
//Has guaranteed exclusive access to VDU session
//Is executed on calling thread
typedef void (*VDU_CONNECTION_CALLBACK)(CHttpFile* httpResponse);

//A single connection to a VDU server, halts thread it is executed on
class CVDUConnection
{
protected:
	VDUAPIType m_type; //Which API to call
	CString m_serverURL; //Server URL to send request to
	CString m_parameter; //Http path parameter
	CString m_requestHeaders; //HTTP Request headers
	CString m_content; //HTTP content
	VDU_CONNECTION_CALLBACK m_callback; //Function to call after http file is received
public:
	//Sets up the connection
	CVDUConnection(CString serverURL, VDUAPIType type, VDU_CONNECTION_CALLBACK callback = nullptr,
		CString requestHeaders = _T(""), CString parameter = _T(""), CString content = _T(""));

	//Processes the connection and halts executing thread until done
	//Should NOT be run in main thread
	void Process();

	//Starting thread procedure for new threads, expects connection created by 'new'
	static UINT ThreadProc(LPVOID pCon);
};