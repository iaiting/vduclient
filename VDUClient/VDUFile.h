#pragma once

#include <afxinet.h>
#include <atlenc.h>
#include <atlstr.h>

#define MD5_LEN 16

//Encapsulates a VDU File with all important data
class CVDUFile
{
//Member are const, can be public
public:
	const CString m_token; //Access token
	const BOOL m_canRead; //Is file readable?
	const BOOL m_canWrite; //Is file writable?
	const CString m_encoding; //Content MIME encoding
	const CString m_name; //File name
	const CString m_type; //Content MIME type
	const SYSTEMTIME m_lastModified; //Last modified ST
	const SYSTEMTIME m_expires; //Expires ST
	const BYTE m_md5[MD5_LEN]; //
	const CString m_etag; //File version
public:
	CVDUFile(CString token, BOOL canRead, BOOL canWrite, CString enconding, CString name, CString type, SYSTEMTIME& lastModified, SYSTEMTIME& expires, BYTE* md5, CString etag);
	~CVDUFile();

	//Returns readability and writability as access flags
	DWORD AccessFlags();
	//
};