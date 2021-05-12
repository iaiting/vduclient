/**
*
 * @author Adam Feranec
 * @file VDUConnection.h
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
//Return value will be thread exit code
typedef INT (*VDU_CONNECTION_CALLBACK)(CHttpFile* httpResponse);

//A single connection to a VDU server, halts thread it is executed on
class CVDUConnection
{
protected:
	VDUAPIType m_type; //Which API to call
	CString m_serverURL; //Server URL to send request to
	CString m_parameter; //Http path parameter
	CString m_requestHeaders; //HTTP Request headers
	CString m_contentFile; //File path of HTTP content
	VDU_CONNECTION_CALLBACK m_callback; //Function to call after http file is received
public:
	//Sets up the connection - construction does NOT initiate the connection, call Process()
	//content is copied if set
	CVDUConnection(CString serverURL, VDUAPIType type, VDU_CONNECTION_CALLBACK callback = nullptr,
		CString requestHeaders = _T(""), CString parameter = _T(""), CString fileContentPath = _T(""));

	//Processes the connection and halts executing thread until done
	//Should NOT be run in main thread
	//Returns callback result or SUCCESS
	INT Process();

	//Starting thread procedure for new threads, expects connection created by 'new'
	static UINT ThreadProc(LPVOID pCon);

	//Signifies last translated connection error to inform user
	static TCHAR LastError[0x400];
};