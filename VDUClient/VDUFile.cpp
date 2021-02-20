#include "pch.h"
#include "VDUFile.h"

CVDUFile::CVDUFile(CString token, BOOL canRead, BOOL canWrite, CString encoding, CString name, CString type, SYSTEMTIME& lastModified, SYSTEMTIME& expires, BYTE* md5, CString etag) :
m_token(token), m_canRead(canRead), m_canWrite(canWrite), m_encoding(encoding), m_name(name), m_type(type), m_lastModified(lastModified),m_expires(expires), m_etag(etag), m_md5{0}
{
	CopyMemory(const_cast<BYTE*>(m_md5), md5, ARRAYSIZE(m_md5));
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
