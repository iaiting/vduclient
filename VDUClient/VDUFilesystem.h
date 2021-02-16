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
#include "VDUFile.h"

#ifdef _DEBUG
#include <ctime>
#define DEBUG_PRINT_FILESYSTEM_CALLS //Should print fn calls of filesystem
#endif

#define PROGNAME                        "vdufs"
#define ALLOCATION_UNIT                 4096
#define FULLPATH_SIZE                   (MAX_PATH + FSP_FSCTL_TRANSACT_PATH_SIZEMAX / sizeof(WCHAR))
#define info(format, ...)               Service::Log(EVENTLOG_INFORMATION_TYPE, (PWSTR)format, __VA_ARGS__)
#define warn(format, ...)               Service::Log(EVENTLOG_WARNING_TYPE, (PWSTR)format, __VA_ARGS__)
#define fail(format, ...)               Service::Log(EVENTLOG_ERROR_TYPE,(PWSTR)format, __VA_ARGS__)
#define ConcatPath(FN, FP)              (0 == StringCbPrintf(FP, sizeof FP, _T("%s%s"), _Path, FN))
#define HandleFromFileDesc(FD)          ((PtfsFileDesc *)(FD))->Handle

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
    UINT64 _CreationTime;
};

struct PtfsFileDesc
{
    PtfsFileDesc() : Handle(INVALID_HANDLE_VALUE), DirBuffer()
    {
    }
    ~PtfsFileDesc()
    {
        CloseHandle(Handle);
        CVDUFileSystem::DeleteDirectoryBuffer(&DirBuffer);
    }
    HANDLE Handle;
    PVOID DirBuffer;
};

class CVDUFileSystemService : public Fsp::Service
{
public:
    CVDUFileSystemService(CString DriveLetter);

    //Returns the filesystem host for mounting
    Fsp::FileSystemHost& GetHost();

    //Remount filesystem to different drive letter
    NTSTATUS Remount(CString DriveLetter);
protected:
    NTSTATUS OnStart(ULONG Argc, PWSTR* Argv);
    NTSTATUS OnStop();

private:
    CVDUFileSystem m_fs;
    Fsp::FileSystemHost m_host;
    TCHAR m_driveLetter[128];
    HANDLE m_hWorkDir;
};