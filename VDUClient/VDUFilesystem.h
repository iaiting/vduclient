/*
 * @copyright 2015-2022 Bill Zissimopoulos
 *
 * @file VDUFilesystem.h
 * This file is a derivative work licensed under the GPLv3 licence and was originally a part of WinFsp.
 *
 * You can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 3 as published by the Free Software
 * Foundation.
 *
 * Licensees holding a valid commercial license may use this software
 * in accordance with the commercial license agreement provided in
 * conjunction with the software.  The terms and conditions of any such
 * commercial license agreement shall govern, supersede, and render
 * ineffective any application of the GPLv3 license to this software,
 * notwithstanding of any reference thereto in the software or
 * associated repository.
 *
 * WinFsp - Windows File System Proxy , Copyright (C) Bill Zissimopoulos
 * GitHub page at https://github.com/billziss-gh/winfsp, Website at https://www.secfs.net/winfsp/.
 * The original file https://github.com/winfsp/winfsp/blob/master/tst/passthrough-cpp/passthrough-cpp.cpp
 *
 */

#pragma once

#include <winsvc.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <strsafe.h>
#include <winnt.h>
#include <bcrypt.h>
#include <winfsp/winfsp.hpp>
#include <vector>
#include <Wincrypt.h>
#include "VDUClientDlg.h"
#include "VDUFile.h"
#include "VDUClient.h"
#include <VersionHelpers.h>

//Disable the use of Windows internals
//#define NO_WINTERNL

#ifdef _DEBUG
#include <ctime>
//Print fn calls of filesystem in a console window
#define DEBUG_PRINT_FILESYSTEM_CALLS //Print fn calls of filesystem in a console window
#endif

#define PROGNAME                        "vdufs"
#define ALLOCATION_UNIT                 4096
#define FULLPATH_SIZE                   (MAX_PATH + FSP_FSCTL_TRANSACT_PATH_SIZEMAX / sizeof(WCHAR))
#define ConcatPath(FN, FP)              (0 == StringCbPrintf(FP, sizeof FP, _T("%s%s"), _Path, FN))
#define HandleFromFileDesc(FD)          ((VdufsFileDesc *)(FD))->Handle

class CVDUFileSystem : public Fsp::FileSystemBase
{
public:
    CVDUFileSystem();
    ~CVDUFileSystem();
    NTSTATUS SetPath(PWSTR Path);

protected:
    static NTSTATUS GetFileInfoInternal(HANDLE Handle, FileInfo* FileInfo);
    NTSTATUS Init(PVOID Host);
    NTSTATUS GetVolumeInfo(
        VolumeInfo* VolumeInfo);
    NTSTATUS GetSecurityByName(
        PWSTR FileName,
        PUINT32 PFileAttributes/* or ReparsePointIndex */,
        PSECURITY_DESCRIPTOR SecurityDescriptor,
        SIZE_T* PSecurityDescriptorSize);
    NTSTATUS Create(
        PWSTR FileName,
        UINT32 CreateOptions,
        UINT32 GrantedAccess,
        UINT32 FileAttributes,
        PSECURITY_DESCRIPTOR SecurityDescriptor,
        UINT64 AllocationSize,
        PVOID* PFileNode,
        PVOID* PFileDesc,
        OpenFileInfo* OpenFileInfo);
    NTSTATUS Open(
        PWSTR FileName,
        UINT32 CreateOptions,
        UINT32 GrantedAccess,
        PVOID* PFileNode,
        PVOID* PFileDesc,
        OpenFileInfo* OpenFileInfo);
    NTSTATUS Overwrite(
        PVOID FileNode,
        PVOID FileDesc,
        UINT32 FileAttributes,
        BOOLEAN ReplaceFileAttributes,
        UINT64 AllocationSize,
        FileInfo* FileInfo);
    VOID Cleanup(
        PVOID FileNode,
        PVOID FileDesc,
        PWSTR FileName,
        ULONG Flags);
    VOID Close(
        PVOID FileNode,
        PVOID FileDesc);
    NTSTATUS Read(
        PVOID FileNode,
        PVOID FileDesc,
        PVOID Buffer,
        UINT64 Offset,
        ULONG Length,
        PULONG PBytesTransferred);
    NTSTATUS Write(
        PVOID FileNode,
        PVOID FileDesc,
        PVOID Buffer,
        UINT64 Offset,
        ULONG Length,
        BOOLEAN WriteToEndOfFile,
        BOOLEAN ConstrainedIo,
        PULONG PBytesTransferred,
        FileInfo* FileInfo);
    NTSTATUS Flush(
        PVOID FileNode,
        PVOID FileDesc,
        FileInfo* FileInfo);
    NTSTATUS GetFileInfo(
        PVOID FileNode,
        PVOID FileDesc,
        FileInfo* FileInfo);
    NTSTATUS SetBasicInfo(
        PVOID FileNode,
        PVOID FileDesc,
        UINT32 FileAttributes,
        UINT64 CreationTime,
        UINT64 LastAccessTime,
        UINT64 LastWriteTime,
        UINT64 ChangeTime,
        FileInfo* FileInfo);
    NTSTATUS SetFileSize(
        PVOID FileNode,
        PVOID FileDesc,
        UINT64 NewSize,
        BOOLEAN SetAllocationSize,
        FileInfo* FileInfo);
    NTSTATUS CanDelete(
        PVOID FileNode,
        PVOID FileDesc,
        PWSTR FileName);
    NTSTATUS Rename(
        PVOID FileNode,
        PVOID FileDesc,
        PWSTR FileName,
        PWSTR NewFileName,
        BOOLEAN ReplaceIfExists);
    NTSTATUS GetSecurity(
        PVOID FileNode,
        PVOID FileDesc,
        PSECURITY_DESCRIPTOR SecurityDescriptor,
        SIZE_T* PSecurityDescriptorSize);
    NTSTATUS SetSecurity(
        PVOID FileNode,
        PVOID FileDesc,
        SECURITY_INFORMATION SecurityInformation,
        PSECURITY_DESCRIPTOR ModificationDescriptor);
    NTSTATUS ReadDirectory(
        PVOID FileNode,
        PVOID FileDesc,
        PWSTR Pattern,
        PWSTR Marker,
        PVOID Buffer,
        ULONG Length,
        PULONG PBytesTransferred);
    NTSTATUS ReadDirectoryEntry(
        PVOID FileNode,
        PVOID FileDesc,
        PWSTR Pattern,
        PWSTR Marker,
        PVOID* PContext,
        DirInfo* DirInfo);

private:

    PWSTR _Path;
};

struct VdufsFileDesc
{
    VdufsFileDesc() : Handle(INVALID_HANDLE_VALUE), DirBuffer()
    {
    }
    ~VdufsFileDesc()
    {
        CloseHandle(Handle);
        CVDUFileSystem::DeleteDirectoryBuffer(&DirBuffer);
    }
    HANDLE Handle;
    PVOID DirBuffer;
};


//File system service that handles the filesystem
class CVDUFileSystemService : public Fsp::Service
{
private:
    CVDUFileSystem m_fs; //File system definition
    Fsp::FileSystemHost m_host; //File system host
    TCHAR m_driveLetter[128]; //Drive letter buffer
    CString m_workDirPath; //Path to work directory
    SRWLOCK m_filesLock; //Lock for accssing files vector
    std::vector<CVDUFile> m_files; //Vector of accessable files
protected:
    NTSTATUS OnStart(ULONG Argc, PWSTR* Argv);
    NTSTATUS OnStop();
public:
    CVDUFileSystemService(CString DriveLetter);

    //Returns the filesystem host for mounting
    Fsp::FileSystemHost& GetHost();
    //Returns the active virtual drive path
    CString GetDrivePath();
    //Returns the work directory path
    CString GetWorkDirPath();
    //Returns accessible VDU file by name
    CVDUFile GetVDUFileByName(CString name);
    //Returns accessible VDU file by access token
    CVDUFile GetVDUFileByToken(CString token);
    //Ammount of accesisibile files
    ULONGLONG GetVDUFileCount();
    //Deletes a VDU file internally, from disk, from memory
    void DeleteFileInternal(CString token);
    //Updates a VDU file internally
    void UpdateFileInternal(CVDUFile newfile);

    //Calculated MD5 of contents in file
    //https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content
    //Returns base64 of md5 bytes or empty string on failure
    CString CalcFileMD5Base64(CVDUFile file);

    //Remount filesystem to different drive letter
    NTSTATUS Remount(CString DriveLetter);

    //Create a new VDU file in filesystem from httpFile
    BOOL CreateVDUFile(CVDUFile vdufile, CHttpFile* httpfile);

    //Sends update of VDU file data to the server
    //This function is BLOCKING if async is FALSE
    //Returns success or exit code if not async
    INT UpdateVDUFile(CVDUFile vdufile, CString newName = _T(""), BOOL async = TRUE);

    //Request token invalidation for VDU file
    //This function is BLOCKING if async is FALSE
    //Returns success or exit code if not async
    INT DeleteVDUFile(CVDUFile vdufile, BOOL async = TRUE);
};