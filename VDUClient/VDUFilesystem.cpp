#include "pch.h"
#include "VDUFilesystem.h"

Vdufs::Vdufs() : FileSystemBase(), _Path()
{
}

Vdufs::~Vdufs()
{
    delete[] _Path;
}

NTSTATUS Vdufs::SetPath(PWSTR Path)
{
    WCHAR FullPath[MAX_PATH];
    ULONG Length;
    HANDLE Handle;
    FILETIME CreationTime;
    DWORD LastError;

    Handle = CreateFileW(
        Path, FILE_READ_ATTRIBUTES, 0, 0,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (INVALID_HANDLE_VALUE == Handle)
        return NtStatusFromWin32(GetLastError());

    Length = GetFinalPathNameByHandleW(Handle, FullPath, FULLPATH_SIZE - 1, 0);
    if (0 == Length)
    {
        LastError = GetLastError();
        CloseHandle(Handle);
        return NtStatusFromWin32(LastError);
    }
    if (L'\\' == FullPath[Length - 1])
        FullPath[--Length] = L'\0';

    if (!GetFileTime(Handle, &CreationTime, 0, 0))
    {
        LastError = GetLastError();
        CloseHandle(Handle);
        return NtStatusFromWin32(LastError);
    }

    CloseHandle(Handle);

    Length++;
    _Path = new WCHAR[Length];
    memcpy(_Path, FullPath, Length * sizeof(WCHAR));

    _CreationTime = ((PLARGE_INTEGER)&CreationTime)->QuadPart;

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::GetFileInfoInternal(HANDLE Handle, FileInfo* FileInfo)
{
    BY_HANDLE_FILE_INFORMATION ByHandleFileInfo;

    if (!GetFileInformationByHandle(Handle, &ByHandleFileInfo))
        return NtStatusFromWin32(GetLastError());

    FileInfo->FileAttributes = ByHandleFileInfo.dwFileAttributes;
    FileInfo->ReparseTag = 0;
    FileInfo->FileSize =
        ((UINT64)ByHandleFileInfo.nFileSizeHigh << 32) | (UINT64)ByHandleFileInfo.nFileSizeLow;
    FileInfo->AllocationSize = (FileInfo->FileSize + ALLOCATION_UNIT - 1)
        / ALLOCATION_UNIT * ALLOCATION_UNIT;
    FileInfo->CreationTime = ((PLARGE_INTEGER)&ByHandleFileInfo.ftCreationTime)->QuadPart;
    FileInfo->LastAccessTime = ((PLARGE_INTEGER)&ByHandleFileInfo.ftLastAccessTime)->QuadPart;
    FileInfo->LastWriteTime = ((PLARGE_INTEGER)&ByHandleFileInfo.ftLastWriteTime)->QuadPart;
    FileInfo->ChangeTime = FileInfo->LastWriteTime;
    FileInfo->IndexNumber = 0;
    FileInfo->HardLinks = 0;

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::Init(PVOID Host0)
{
    Fsp::FileSystemHost* Host = (Fsp::FileSystemHost*)Host0;
    Host->SetSectorSize(ALLOCATION_UNIT);
    Host->SetSectorsPerAllocationUnit(1);
    Host->SetFileInfoTimeout(1000);
    Host->SetCaseSensitiveSearch(FALSE);
    Host->SetCasePreservedNames(TRUE);
    Host->SetUnicodeOnDisk(TRUE);
    Host->SetPersistentAcls(TRUE);
    Host->SetPostCleanupWhenModifiedOnly(TRUE);
    Host->SetPassQueryDirectoryPattern(TRUE);
    Host->SetVolumeCreationTime(_CreationTime);
    Host->SetVolumeSerialNumber(0x42069);
    Host->SetFlushAndPurgeOnCleanup(TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::GetVolumeInfo(
    VolumeInfo* VolumeInfo)
{
    ULARGE_INTEGER TotalSize, FreeSize;

    //if (!GetVolumePathName(_Path, Root, MAX_PATH))
     //    return NtStatusFromWin32(GetLastError());

    if (!GetDiskFreeSpaceEx(_Path, 0, &TotalSize, &FreeSize))
        return NtStatusFromWin32(GetLastError());

    //GetFileSizeEx()

    /*VolumeInfo->TotalSize = TotalSize.QuadPart;
    VolumeInfo->FreeSize = FreeSize.QuadPart;
    swprintf_s(VolumeInfo->VolumeLabel, L"VDU");
    VolumeInfo->VolumeLabelLength = wcslen(VolumeInfo->VolumeLabel);*/

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::GetSecurityByName(
    PWSTR FileName,
    PUINT32 PFileAttributes/* or ReparsePointIndex */,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    SIZE_T* PSecurityDescriptorSize)
{
    WCHAR FullPath[FULLPATH_SIZE];
    HANDLE Handle;
    FILE_ATTRIBUTE_TAG_INFO AttributeTagInfo;
    DWORD SecurityDescriptorSizeNeeded;
    NTSTATUS Result;

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    Handle = CreateFileW(FullPath,
        FILE_READ_ATTRIBUTES | READ_CONTROL, 0, 0,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (INVALID_HANDLE_VALUE == Handle)
    {
        Result = NtStatusFromWin32(GetLastError());
        goto exit;
    }

    if (0 != PFileAttributes)
    {
        if (!GetFileInformationByHandleEx(Handle,
            FileAttributeTagInfo, &AttributeTagInfo, sizeof AttributeTagInfo))
        {
            Result = NtStatusFromWin32(GetLastError());
            goto exit;
        }

        *PFileAttributes = AttributeTagInfo.FileAttributes;
    }

    if (0 != PSecurityDescriptorSize)
    {
        if (!GetKernelObjectSecurity(Handle,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            SecurityDescriptor, (DWORD)*PSecurityDescriptorSize, &SecurityDescriptorSizeNeeded))
        {
            *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;
            Result = NtStatusFromWin32(GetLastError());
            goto exit;
        }

        *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;
    }

    Result = STATUS_SUCCESS;

exit:
    if (INVALID_HANDLE_VALUE != Handle)
        CloseHandle(Handle);

    return Result;
}

NTSTATUS Vdufs::Create(
    PWSTR FileName,
    UINT32 CreateOptions,
    UINT32 GrantedAccess,
    UINT32 FileAttributes,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    UINT64 AllocationSize,
    PVOID* PFileNode,
    PVOID* PFileDesc,
    OpenFileInfo* OpenFileInfo)
{
    WCHAR FullPath[FULLPATH_SIZE];
    SECURITY_ATTRIBUTES SecurityAttributes;
    ULONG CreateFlags;
    PtfsFileDesc* FileDesc;

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    FileDesc = new PtfsFileDesc;

    SecurityAttributes.nLength = sizeof SecurityAttributes;
    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    CreateFlags = FILE_FLAG_BACKUP_SEMANTICS;
    if (CreateOptions & FILE_DELETE_ON_CLOSE)
        CreateFlags |= FILE_FLAG_DELETE_ON_CLOSE;

    if (CreateOptions & FILE_DIRECTORY_FILE)
    {
        /*
         * It is not widely known but CreateFileW can be used to create directories!
         * It requires the specification of both FILE_FLAG_BACKUP_SEMANTICS and
         * FILE_FLAG_POSIX_SEMANTICS. It also requires that FileAttributes has
         * FILE_ATTRIBUTE_DIRECTORY set.
         */
        CreateFlags |= FILE_FLAG_POSIX_SEMANTICS;
        FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else
        FileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

    if (0 == FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    FileDesc->Handle = CreateFileW(FullPath,
        GrantedAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &SecurityAttributes,
        CREATE_NEW, CreateFlags | FileAttributes, 0);
    if (INVALID_HANDLE_VALUE == FileDesc->Handle)
    {
        delete FileDesc;
        return NtStatusFromWin32(GetLastError());
    }

    *PFileDesc = FileDesc;

    return GetFileInfoInternal(FileDesc->Handle, &OpenFileInfo->FileInfo);
}

NTSTATUS Vdufs::Open(
    PWSTR FileName,
    UINT32 CreateOptions,
    UINT32 GrantedAccess,
    PVOID* PFileNode,
    PVOID* PFileDesc,
    OpenFileInfo* OpenFileInfo)
{
    WCHAR FullPath[FULLPATH_SIZE];
    ULONG CreateFlags;
    PtfsFileDesc* FileDesc;

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    FileDesc = new PtfsFileDesc;

    CreateFlags = FILE_FLAG_BACKUP_SEMANTICS;
    if (CreateOptions & FILE_DELETE_ON_CLOSE)
        CreateFlags |= FILE_FLAG_DELETE_ON_CLOSE;

    FileDesc->Handle = CreateFileW(FullPath,
        GrantedAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
        OPEN_EXISTING, CreateFlags, 0);
    if (INVALID_HANDLE_VALUE == FileDesc->Handle)
    {
        delete FileDesc;
        return NtStatusFromWin32(GetLastError());
    }

    *PFileDesc = FileDesc;

    return GetFileInfoInternal(FileDesc->Handle, &OpenFileInfo->FileInfo);
}

NTSTATUS Vdufs::Overwrite(
    PVOID FileNode,
    PVOID FileDesc,
    UINT32 FileAttributes,
    BOOLEAN ReplaceFileAttributes,
    UINT64 AllocationSize,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);
    FILE_BASIC_INFO BasicInfo = { 0 };
    FILE_ALLOCATION_INFO AllocationInfo = { 0 };
    FILE_ATTRIBUTE_TAG_INFO AttributeTagInfo;

    if (ReplaceFileAttributes)
    {
        if (0 == FileAttributes)
            FileAttributes = FILE_ATTRIBUTE_NORMAL;

        BasicInfo.FileAttributes = FileAttributes;
        if (!SetFileInformationByHandle(Handle,
            FileBasicInfo, &BasicInfo, sizeof BasicInfo))
            return NtStatusFromWin32(GetLastError());
    }
    else if (0 != FileAttributes)
    {
        if (!GetFileInformationByHandleEx(Handle,
            FileAttributeTagInfo, &AttributeTagInfo, sizeof AttributeTagInfo))
            return NtStatusFromWin32(GetLastError());

        BasicInfo.FileAttributes = FileAttributes | AttributeTagInfo.FileAttributes;
        if (BasicInfo.FileAttributes ^ FileAttributes)
        {
            if (!SetFileInformationByHandle(Handle,
                FileBasicInfo, &BasicInfo, sizeof BasicInfo))
                return NtStatusFromWin32(GetLastError());
        }
    }

    if (!SetFileInformationByHandle(Handle,
        FileAllocationInfo, &AllocationInfo, sizeof AllocationInfo))
        return NtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

VOID Vdufs::Cleanup(
    PVOID FileNode,
    PVOID FileDesc,
    PWSTR FileName,
    ULONG Flags)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);

    if (Flags & CleanupDelete)
    {
        CloseHandle(Handle);

        /* this will make all future uses of Handle to fail with STATUS_INVALID_HANDLE */
        HandleFromFileDesc(FileDesc) = INVALID_HANDLE_VALUE;
    }
}

VOID Vdufs::Close(
    PVOID FileNode,
    PVOID FileDesc0)
{
    PtfsFileDesc* FileDesc = (PtfsFileDesc*)FileDesc0;

    delete FileDesc;
}

NTSTATUS Vdufs::Read(
    PVOID FileNode,
    PVOID FileDesc,
    PVOID Buffer,
    UINT64 Offset,
    ULONG Length,
    PULONG PBytesTransferred)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);
    OVERLAPPED Overlapped = { 0 };

    Overlapped.Offset = (DWORD)Offset;
    Overlapped.OffsetHigh = (DWORD)(Offset >> 32);

    if (!ReadFile(Handle, Buffer, Length, PBytesTransferred, &Overlapped))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::Write(
    PVOID FileNode,
    PVOID FileDesc,
    PVOID Buffer,
    UINT64 Offset,
    ULONG Length,
    BOOLEAN WriteToEndOfFile,
    BOOLEAN ConstrainedIo,
    PULONG PBytesTransferred,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);
    LARGE_INTEGER FileSize;
    OVERLAPPED Overlapped = { 0 };

    if (ConstrainedIo)
    {
        if (!GetFileSizeEx(Handle, &FileSize))
            return NtStatusFromWin32(GetLastError());

        if (Offset >= (UINT64)FileSize.QuadPart)
            return STATUS_SUCCESS;
        if (Offset + Length > (UINT64)FileSize.QuadPart)
            Length = (ULONG)((UINT64)FileSize.QuadPart - Offset);
    }

    Overlapped.Offset = (DWORD)Offset;
    Overlapped.OffsetHigh = (DWORD)(Offset >> 32);

    if (!WriteFile(Handle, Buffer, Length, PBytesTransferred, &Overlapped))
        return NtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS Vdufs::Flush(
    PVOID FileNode,
    PVOID FileDesc,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);

    /* we do not flush the whole volume, so just return SUCCESS */
    if (0 == Handle)
        return STATUS_SUCCESS;

    if (!FlushFileBuffers(Handle))
        return NtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS Vdufs::GetFileInfo(
    PVOID FileNode,
    PVOID FileDesc,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS Vdufs::SetBasicInfo(
    PVOID FileNode,
    PVOID FileDesc,
    UINT32 FileAttributes,
    UINT64 CreationTime,
    UINT64 LastAccessTime,
    UINT64 LastWriteTime,
    UINT64 ChangeTime,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);
    FILE_BASIC_INFO BasicInfo = { 0 };

    if (INVALID_FILE_ATTRIBUTES == FileAttributes)
        FileAttributes = 0;
    else if (0 == FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    BasicInfo.FileAttributes = FileAttributes;
    BasicInfo.CreationTime.QuadPart = CreationTime;
    BasicInfo.LastAccessTime.QuadPart = LastAccessTime;
    BasicInfo.LastWriteTime.QuadPart = LastWriteTime;
    //BasicInfo.ChangeTime = ChangeTime;

    if (!SetFileInformationByHandle(Handle,
        FileBasicInfo, &BasicInfo, sizeof BasicInfo))
        return NtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS Vdufs::SetFileSize(
    PVOID FileNode,
    PVOID FileDesc,
    UINT64 NewSize,
    BOOLEAN SetAllocationSize,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);
    FILE_ALLOCATION_INFO AllocationInfo;
    FILE_END_OF_FILE_INFO EndOfFileInfo;

    if (SetAllocationSize)
    {
        /*
         * This file system does not maintain AllocationSize, although NTFS clearly can.
         * However it must always be FileSize <= AllocationSize and NTFS will make sure
         * to truncate the FileSize if it sees an AllocationSize < FileSize.
         *
         * If OTOH a very large AllocationSize is passed, the call below will increase
         * the AllocationSize of the underlying file, although our file system does not
         * expose this fact. This AllocationSize is only temporary as NTFS will reset
         * the AllocationSize of the underlying file when it is closed.
         */

        AllocationInfo.AllocationSize.QuadPart = NewSize;

        if (!SetFileInformationByHandle(Handle,
            FileAllocationInfo, &AllocationInfo, sizeof AllocationInfo))
            return NtStatusFromWin32(GetLastError());
    }
    else
    {
        EndOfFileInfo.EndOfFile.QuadPart = NewSize;

        if (!SetFileInformationByHandle(Handle,
            FileEndOfFileInfo, &EndOfFileInfo, sizeof EndOfFileInfo))
            return NtStatusFromWin32(GetLastError());
    }

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS Vdufs::CanDelete(
    PVOID FileNode,
    PVOID FileDesc,
    PWSTR FileName)
{
    return STATUS_UNSUCCESSFUL;

    HANDLE Handle = HandleFromFileDesc(FileDesc);
    FILE_DISPOSITION_INFO DispositionInfo;

    DispositionInfo.DeleteFile = TRUE;

    if (!SetFileInformationByHandle(Handle,
        FileDispositionInfo, &DispositionInfo, sizeof DispositionInfo))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::Rename(
    PVOID FileNode,
    PVOID FileDesc,
    PWSTR FileName,
    PWSTR NewFileName,
    BOOLEAN ReplaceIfExists)
{
    WCHAR FullPath[FULLPATH_SIZE], NewFullPath[FULLPATH_SIZE];

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    if (!ConcatPath(NewFileName, NewFullPath))
        return STATUS_OBJECT_NAME_INVALID;

    if (!MoveFileExW(FullPath, NewFullPath, ReplaceIfExists ? MOVEFILE_REPLACE_EXISTING : 0))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::GetSecurity(
    PVOID FileNode,
    PVOID FileDesc,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    SIZE_T* PSecurityDescriptorSize)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);
    DWORD SecurityDescriptorSizeNeeded;

    if (!GetKernelObjectSecurity(Handle,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        SecurityDescriptor, (DWORD)*PSecurityDescriptorSize, &SecurityDescriptorSizeNeeded))
    {
        *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;
        return NtStatusFromWin32(GetLastError());
    }

    *PSecurityDescriptorSize = SecurityDescriptorSizeNeeded;

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::SetSecurity(
    PVOID FileNode,
    PVOID FileDesc,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ModificationDescriptor)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);

    if (!SetKernelObjectSecurity(Handle, SecurityInformation, ModificationDescriptor))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS Vdufs::ReadDirectory(
    PVOID FileNode,
    PVOID FileDesc0,
    PWSTR Pattern,
    PWSTR Marker,
    PVOID Buffer,
    ULONG Length,
    PULONG PBytesTransferred)
{
    PtfsFileDesc* FileDesc = (PtfsFileDesc*)FileDesc0;
    return BufferedReadDirectory(&FileDesc->DirBuffer,
        FileNode, FileDesc, Pattern, Marker, Buffer, Length, PBytesTransferred);
}

NTSTATUS Vdufs::ReadDirectoryEntry(
    PVOID FileNode,
    PVOID FileDesc0,
    PWSTR Pattern,
    PWSTR Marker,
    PVOID* PContext,
    DirInfo* DirInfo)
{
    PtfsFileDesc* FileDesc = (PtfsFileDesc*)FileDesc0;
    HANDLE Handle = FileDesc->Handle;
    WCHAR FullPath[FULLPATH_SIZE];
    ULONG Length, PatternLength;
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;

    if (0 == *PContext)
    {
        if (0 == Pattern)
            Pattern = (PWSTR)L"*";
        PatternLength = (ULONG)wcslen(Pattern);

        Length = GetFinalPathNameByHandleW(Handle, FullPath, FULLPATH_SIZE - 1, 0);
        if (0 == Length)
            return NtStatusFromWin32(GetLastError());
        if (Length + 1 + PatternLength >= FULLPATH_SIZE)
            return STATUS_OBJECT_NAME_INVALID;

        if (L'\\' != FullPath[Length - 1])
            FullPath[Length++] = L'\\';
        memcpy(FullPath + Length, Pattern, PatternLength * sizeof(WCHAR));
        FullPath[Length + PatternLength] = L'\0';

        FindHandle = FindFirstFileW(FullPath, &FindData);
        if (INVALID_HANDLE_VALUE == FindHandle)
            return STATUS_NO_MORE_FILES;

        *PContext = FindHandle;
    }
    else
    {
        FindHandle = *PContext;
        if (!FindNextFileW(FindHandle, &FindData))
        {
            FindClose(FindHandle);
            return STATUS_NO_MORE_FILES;
        }
    }

    memset(DirInfo, 0, sizeof * DirInfo);
    Length = (ULONG)wcslen(FindData.cFileName);
    DirInfo->Size = (UINT16)(FIELD_OFFSET(Vdufs::DirInfo, FileNameBuf) + Length * sizeof(WCHAR));
    DirInfo->FileInfo.FileAttributes = FindData.dwFileAttributes;
    DirInfo->FileInfo.ReparseTag = 0;
    DirInfo->FileInfo.FileSize =
        ((UINT64)FindData.nFileSizeHigh << 32) | (UINT64)FindData.nFileSizeLow;
    DirInfo->FileInfo.AllocationSize = (DirInfo->FileInfo.FileSize + ALLOCATION_UNIT - 1)
        / ALLOCATION_UNIT * ALLOCATION_UNIT;
    DirInfo->FileInfo.CreationTime = ((PLARGE_INTEGER)&FindData.ftCreationTime)->QuadPart;
    DirInfo->FileInfo.LastAccessTime = ((PLARGE_INTEGER)&FindData.ftLastAccessTime)->QuadPart;
    DirInfo->FileInfo.LastWriteTime = ((PLARGE_INTEGER)&FindData.ftLastWriteTime)->QuadPart;
    DirInfo->FileInfo.ChangeTime = DirInfo->FileInfo.LastWriteTime;
    DirInfo->FileInfo.IndexNumber = 0;
    DirInfo->FileInfo.HardLinks = 0;
    memcpy(DirInfo->FileNameBuf, FindData.cFileName, Length * sizeof(WCHAR));

    return STATUS_SUCCESS;
}

class VdufsService : public Fsp::Service
{
public:
    VdufsService();

protected:
    NTSTATUS OnStart(ULONG Argc, PWSTR* Argv);
    NTSTATUS OnStop();

private:
    Vdufs fs;
    Fsp::FileSystemHost _Host;
};

static NTSTATUS EnableBackupRestorePrivileges(VOID)
{
    union
    {
        TOKEN_PRIVILEGES P;
        UINT8 B[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
    } Privileges;
    HANDLE Token;

    Privileges.P.PrivilegeCount = 2;
    Privileges.P.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Privileges.P.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueW(0, SE_BACKUP_NAME, &Privileges.P.Privileges[0].Luid) ||
        !LookupPrivilegeValueW(0, SE_RESTORE_NAME, &Privileges.P.Privileges[1].Luid))
        return FspNtStatusFromWin32(GetLastError());

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Token))
        return FspNtStatusFromWin32(GetLastError());

    if (!AdjustTokenPrivileges(Token, FALSE, &Privileges.P, 0, 0, 0))
    {
        CloseHandle(Token);

        return FspNtStatusFromWin32(GetLastError());
    }

    CloseHandle(Token);

    return STATUS_SUCCESS;
}

static ULONG wcstol_deflt(wchar_t* w, ULONG deflt)
{
    wchar_t* endp;
    ULONG ul = wcstol(w, &endp, 0);
    return L'\0' != w[0] && L'\0' == *endp ? ul : deflt;
}

VdufsService::VdufsService() : Service((PWSTR)L"" PROGNAME), fs(), _Host(fs)
{
}

NTSTATUS VdufsService::OnStart(ULONG argc, PWSTR* argv)
{

    PWSTR DebugLogFile = (PWSTR)L"vfsdebug.log";
    ULONG DebugFlags = 0;
    //PWSTR VolumePrefix = (PWSTR)L"\\?\\"; /* \\?\C:\ */
    PWSTR MountPoint = (PWSTR)L"X:";
    HANDLE DebugLogHandle = INVALID_HANDLE_VALUE;
    WCHAR PathBuf[MAX_PATH];
    NTSTATUS Result;

    PWSTR localappdata;
    size_t len;
    _wdupenv_s(&localappdata, &len, L"localappdata");

    swprintf_s(PathBuf, L"%s/%s", localappdata, L"VDU");

    CreateDirectory(PathBuf, NULL); //TODO: Check success?

    SetFileAttributes(PathBuf, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM
        | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_READONLY);

    EnableBackupRestorePrivileges();

    if (0 != DebugLogFile)
    {
        Result = Fsp::FileSystemHost::SetDebugLogFile(DebugLogFile);
        if (!NT_SUCCESS(Result))
        {
            fail(L"cannot open debug log file");
            return STATUS_UNSUCCESSFUL;
        }
    }

    Result = fs.SetPath(PathBuf);
    if (!NT_SUCCESS(Result))
    {
        fail(L"cannot create file system");
        return Result;
    }


    _Host.SetFileSystemName((PWSTR)L"VDU");
    //_Host.SetPrefix(VolumePrefix);
    Result = _Host.Mount(MountPoint, 0, FALSE, DebugFlags);
    if (!NT_SUCCESS(Result))
    {
        fail(L"cannot mount file system");
        return Result;
    }

    //MountPoint = _Host.MountPoint();

    return STATUS_SUCCESS;
}

NTSTATUS VdufsService::OnStop()
{
    _Host.Unmount();
    return STATUS_SUCCESS;
}

UINT vdufs_main(LPVOID)
{
    return VdufsService().Run();
}