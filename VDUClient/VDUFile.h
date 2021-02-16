#pragma once

#include <afxinet.h>
#include <atlenc.h>
#include <atlstr.h>

//Encapsulates a VDU File with all important data
class CVDUFile
{
private:
	BOOL m_canRead;
	BOOL m_canWrite;
	CString m_encoding;
	CString m_name;
	CString m_type;
	SYSTEMTIME m_lastModified;
	SYSTEMTIME m_expires;
	CString m_etag;

public:
	CVDUFile(BOOL canRead, BOOL canWrite, CString enconding, CString name, CString type, SYSTEMTIME& lastModified, SYSTEMTIME& expires, CString etag);
	~CVDUFile();
};