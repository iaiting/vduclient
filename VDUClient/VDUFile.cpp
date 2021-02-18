#include "pch.h"
#include "VDUFile.h"

CVDUFile::CVDUFile(CString token, BOOL canRead, BOOL canWrite, CString encoding, CString name, CString type, SYSTEMTIME& lastModified, SYSTEMTIME& expires, CString etag) :
m_token(token), m_canRead(canRead), m_canWrite(canWrite), m_encoding(encoding), m_name(name), m_type(type), m_lastModified(lastModified),m_expires(expires), m_etag(etag)
{

}

CVDUFile::~CVDUFile()
{

}

DWORD CVDUFile::AccessFlags()
{
	DWORD flags = GENERIC_EXECUTE;
	if (m_canRead)
		flags |= GENERIC_READ;
	if (m_canWrite)
		flags |= GENERIC_WRITE;
	return flags;
}
