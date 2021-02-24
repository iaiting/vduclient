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
	CString m_token; //Access token
	BOOL m_canRead; //Is file readable?
	BOOL m_canWrite; //Is file writable?
	UINT32 m_length; //content length
	CString m_encoding; //Content MIME encoding
	CString m_name; //File name
	CString m_type; //Content MIME type
	SYSTEMTIME m_lastModified; //Last modified ST
	SYSTEMTIME m_expires; //Expires ST
	BYTE m_md5[MD5_LEN]; //
	CString m_etag; //File version
public:
	CVDUFile(CString token, BOOL canRead, BOOL canWrite, UINT32 length, CString enconding, CString name, CString type, SYSTEMTIME& lastModified, SYSTEMTIME& expires, BYTE* md5, CString etag);
	CVDUFile();
	~CVDUFile();

	void operator=(const CVDUFile& f);
	bool operator==(const CVDUFile& f);
	bool operator!=(const CVDUFile& f);

	//Is the file a valid file?
	bool IsValid();

	//Returns readability and writability as access flags
	DWORD AccessFlags();
	
	static CVDUFile InvalidFile;
};