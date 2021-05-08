/**
*
 * @author xferan00
 * @file VDUFile.cpp
 *
 */

#include "pch.h"
#include "VDUFile.h"

CVDUFile CVDUFile::InvalidFile = CVDUFile();

CVDUFile::CVDUFile(CString token, BOOL canRead, BOOL canWrite, UINT32 length, CString encoding, CString name, CString type,
	SYSTEMTIME& lastModified, SYSTEMTIME& expires, CString md5b64, CString etag) :
m_token(token), m_canRead(canRead), m_canWrite(canWrite), m_length(length), m_encoding(encoding), m_name(name),
m_type(type), m_lastModified(lastModified),m_expires(expires), m_etag(etag), m_md5base64(md5b64)
{

}

CVDUFile::CVDUFile() : m_canRead(FALSE), m_canWrite(FALSE), m_length(0), m_lastModified(SYSTEMTIME()), m_expires(SYSTEMTIME())
{
}

CVDUFile::~CVDUFile()
{

}

void CVDUFile::operator=(const CVDUFile& f)
{
	this->m_token = CString(f.m_token);
	this->m_canRead = f.m_canRead;
	this->m_canWrite = f.m_canWrite;
	this->m_length = f.m_length;
	this->m_encoding = CString(f.m_encoding);
	this->m_name = CString(f.m_name);
	this->m_type = CString(f.m_type);
	this->m_lastModified = f.m_lastModified;
	this->m_expires = f.m_expires;
	this->m_md5base64 = CString(f.m_md5base64);
	this->m_etag = CString(f.m_etag);
}

bool CVDUFile::operator==(const CVDUFile& f)
{
	return this->m_token == f.m_token;
}

bool CVDUFile::operator!=(const CVDUFile& f)
{
	return this->m_token != f.m_token;
}

bool CVDUFile::IsValid()
{
	return !this->m_token.IsEmpty() && *this != CVDUFile::InvalidFile;
}
