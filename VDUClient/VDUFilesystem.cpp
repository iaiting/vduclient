#include "pch.h"
#include "VDUFilesystem.h"

CVDUFileSystem::CVDUFileSystem() : FileSystemBase(), _Path()
{
}

CVDUFileSystem::~CVDUFileSystem()
{
    delete[] _Path;
}

NTSTATUS CVDUFileSystem::SetPath(PWSTR Path)
{
    WCHAR FullPath[FULLPATH_SIZE];
    ULONG Length;
    HANDLE Handle;
    FILETIME CreationTime;
    DWORD LastError;

    Handle = CreateFileW(
        Path, FILE_READ_ATTRIBUTES, 0, 0,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (INVALID_HANDLE_VALUE == Handle)
        return NtStatusFromWin32(GetLastError());

    Length = GetFinalPathNameByHandle(Handle, FullPath, FULLPATH_SIZE - 1, 0);
    if (0 == Length)
    {
        LastError = GetLastError();
        CloseHandle(Handle);
        return NtStatusFromWin32(LastError);
    }
    if (_T('\\') == FullPath[Length - 1])
        FullPath[--Length] = _T('\0');

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

NTSTATUS CVDUFileSystem::GetFileInfoInternal(HANDLE Handle, FileInfo* FileInfo)
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

NTSTATUS CVDUFileSystem::Init(PVOID Host0)
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

NTSTATUS CVDUFileSystem::GetVolumeInfo(VolumeInfo* VolumeInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif
    //In order to count files in our work path, we add an asterisk
    CString wildcardPath = _Path;
    wildcardPath += "\\*";

    VolumeInfo->TotalSize = VolumeInfo->FreeSize = 0;
    WIN32_FIND_DATA FindFileData;
    SecureZeroMemory(&FindFileData, sizeof(FindFileData));
    HANDLE hFind;

    //Count all files to the total size
    hFind = FindFirstFile(wildcardPath, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            VolumeInfo->TotalSize += ((UINT64)FindFileData.nFileSizeHigh << 32) | (UINT64)FindFileData.nFileSizeLow;
        } while (FindNextFile(hFind, &FindFileData));
    }

    VolumeInfo->FreeSize = VolumeInfo->TotalSize * 2;

    return STATUS_SUCCESS;
}

NTSTATUS CVDUFileSystem::GetSecurityByName(
    PWSTR FileName,
    PUINT32 PFileAttributes/* or ReparsePointIndex */,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    SIZE_T* PSecurityDescriptorSize)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s(%ws)%s", buf, __FUNCTION__, FileName, "\r\n");
#endif

    WCHAR FullPath[FULLPATH_SIZE];
    HANDLE Handle;
    FILE_ATTRIBUTE_TAG_INFO AttributeTagInfo;
    DWORD SecurityDescriptorSizeNeeded;
    NTSTATUS Result;

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    Handle = CreateFile(FullPath,
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

NTSTATUS CVDUFileSystem::Create(
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
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    WCHAR FullPath[FULLPATH_SIZE];
    SECURITY_ATTRIBUTES SecurityAttributes;
    ULONG CreateFlags;
    VdufsFileDesc* FileDesc;

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    FileDesc = new VdufsFileDesc;

    SecurityAttributes.nLength = sizeof SecurityAttributes;
    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    CreateFlags = FILE_FLAG_BACKUP_SEMANTICS;
    if (CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        CreateFlags |= FILE_FLAG_DELETE_ON_CLOSE;
    }

    if (CreateOptions & FILE_DIRECTORY_FILE)
    {
        //Attempting to create directory.. not supported by our simple file system
        delete FileDesc;
        return STATUS_NOT_SUPPORTED;
    }
    else
        FileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

    //If programs are creating temporary files, hide them from user
    FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
    //Temporary flag is good because it doesnt save those temporary files on physical disk
    FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;

    FileDesc->Handle = CreateFileW(FullPath,
        GrantedAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &SecurityAttributes,
        OPEN_ALWAYS, CreateFlags | FileAttributes, 0);
    if (INVALID_HANDLE_VALUE == FileDesc->Handle)
    {
        delete FileDesc;
        NTSTATUS result = NtStatusFromWin32(GetLastError());

        //More friendly message, dont create files..
        if (result == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_NOT_SUPPORTED;

        return result;
    }

    *PFileDesc = FileDesc;

    return GetFileInfoInternal(FileDesc->Handle, &OpenFileInfo->FileInfo);
}

NTSTATUS CVDUFileSystem::Open(
    PWSTR FileName,
    UINT32 CreateOptions,
    UINT32 GrantedAccess,
    PVOID* PFileNode,
    PVOID* PFileDesc,
    OpenFileInfo* OpenFileInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s(%ws)\n", buf, __FUNCTION__, FileName);
#endif

    WCHAR FullPath[FULLPATH_SIZE];
    ULONG CreateFlags;
    VdufsFileDesc* FileDesc;

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    FileDesc = new VdufsFileDesc();

    CreateFlags = FILE_FLAG_BACKUP_SEMANTICS;

    CVDUFile vdufile = APP->GetFileSystemService()->GetVDUFileByName(PathFindFileName(FileName));

    //File is about to be deleted, (i.e. from explorer)
    if (CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        //Send delete request and pretend file was deleted
        //If it wasnt, the file will reappear on explorer window refresh
        //This is to smooth out user experience
        if (vdufile.IsValid())
        {
            CString newMd5b64 = APP->GetFileSystemService()->CalcFileMD5Base64(vdufile);
            if (newMd5b64 != vdufile.m_md5base64)
            {
                //vdufile.m_md5base64 = newMd5b64;
                APP->GetFileSystemService()->UpdateVDUFile(vdufile);
            }
            APP->GetFileSystemService()->DeleteVDUFile(vdufile);
            //delete FileDesc;
            *PFileDesc = FileDesc;
            return STATUS_SUCCESS;
        }
    }

    if (vdufile.IsValid())
    {
        if (!vdufile.m_canWrite &&
            (GrantedAccess & GENERIC_WRITE || GrantedAccess & WRITE_DAC || GrantedAccess & WRITE_OWNER))
        {
            //You dont have rights for this access to VDU file
            return STATUS_ACCESS_DENIED;
        }
    }

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

NTSTATUS CVDUFileSystem::Overwrite(
    PVOID FileNode,
    PVOID FileDesc,
    UINT32 FileAttributes,
    BOOLEAN ReplaceFileAttributes,
    UINT64 AllocationSize,
    FileInfo* FileInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

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

VOID CVDUFileSystem::Cleanup(
    PVOID FileNode,
    PVOID FileDesc,
    PWSTR FileName,
    ULONG Flags)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    HANDLE Handle = HandleFromFileDesc(FileDesc);

    if (Flags & CleanupSetLastWriteTime || Flags & CleanupSetChangeTime)
    {
        WCHAR FullPath[FULLPATH_SIZE];
        ULONG Length = GetFinalPathNameByHandle(HandleFromFileDesc(FileDesc), FullPath, ARRAYSIZE(FullPath) - 1, 0);
        FullPath[Length] = '\0';

        CString fname = PathFindFileName(FullPath);

        CVDUFile vdufile = APP->GetFileSystemService()->GetVDUFileByName(fname);
        if (vdufile.IsValid())
        {
            //Get file size, calculate hash, and try updating
            HANDLE hFile = CreateFile(FullPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                vdufile.m_length = GetFileSize(hFile, NULL);
                CloseHandle(hFile);

                //NOTE: Ignoring first clean up, data is written after second
                if (vdufile.m_length > 0)
                {
                    CString newMd5b64 = APP->GetFileSystemService()->CalcFileMD5Base64(vdufile);
                    if (newMd5b64 != vdufile.m_md5base64)
                    {
                        //vdufile.m_md5base64 = newMd5b64;
                        APP->GetFileSystemService()->UpdateVDUFile(vdufile);
                    }
                }
            }
        }
    }
    if (Flags & CleanupDelete)
    {
        CloseHandle(Handle);

        /* this will make all future uses of Handle to fail with STATUS_INVALID_HANDLE */
        HandleFromFileDesc(FileDesc) = INVALID_HANDLE_VALUE;
    }
}

VOID CVDUFileSystem::Close(
    PVOID FileNode,
    PVOID FileDesc0)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    VdufsFileDesc* FileDesc = (VdufsFileDesc*)FileDesc0;

    delete FileDesc;
}

NTSTATUS CVDUFileSystem::Read(
    PVOID FileNode,
    PVOID FileDesc,
    PVOID Buffer,
    UINT64 Offset,
    ULONG Length,
    PULONG PBytesTransferred)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    HANDLE Handle = HandleFromFileDesc(FileDesc);
    OVERLAPPED Overlapped = { 0 };

    Overlapped.Offset = (DWORD)Offset;
    Overlapped.OffsetHigh = (DWORD)(Offset >> 32);

    if (!ReadFile(Handle, Buffer, Length, PBytesTransferred, &Overlapped))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS CVDUFileSystem::Write(
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
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

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

NTSTATUS CVDUFileSystem::Flush(
    PVOID FileNode,
    PVOID FileDesc,
    FileInfo* FileInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    HANDLE Handle = HandleFromFileDesc(FileDesc);

    /* we do not flush the whole volume, so just return SUCCESS */
    if (0 == Handle)
        return STATUS_SUCCESS;

    if (!FlushFileBuffers(Handle))
        return NtStatusFromWin32(GetLastError());

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS CVDUFileSystem::GetFileInfo(
    PVOID FileNode,
    PVOID FileDesc,
    FileInfo* FileInfo)
{
    HANDLE Handle = HandleFromFileDesc(FileDesc);

    return GetFileInfoInternal(Handle, FileInfo);
}

NTSTATUS CVDUFileSystem::SetBasicInfo(
    PVOID FileNode,
    PVOID FileDesc,
    UINT32 FileAttributes,
    UINT64 CreationTime,
    UINT64 LastAccessTime,
    UINT64 LastWriteTime,
    UINT64 ChangeTime,
    FileInfo* FileInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

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

NTSTATUS CVDUFileSystem::SetFileSize(
    PVOID FileNode,
    PVOID FileDesc,
    UINT64 NewSize,
    BOOLEAN SetAllocationSize,
    FileInfo* FileInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

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

NTSTATUS CVDUFileSystem::CanDelete(
    PVOID FileNode,
    PVOID FileDesc,
    PWSTR FileName)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    WCHAR FullPath[FULLPATH_SIZE];
    ULONG Length = GetFinalPathNameByHandle(HandleFromFileDesc(FileDesc), FullPath, ARRAYSIZE(FullPath) - 1, 0);
    FullPath[Length] = '\0';

    CString fname = PathFindFileName(FullPath);

    CVDUFile vdufile = APP->GetFileSystemService()->GetVDUFileByName(fname);
    if (vdufile.IsValid())
    {
        //VDU Files are not deletable
        return STATUS_UNSUCCESSFUL;
    }

    HANDLE Handle = HandleFromFileDesc(FileDesc);
    FILE_DISPOSITION_INFO DispositionInfo;

    DispositionInfo.DeleteFile = TRUE;

    if (!SetFileInformationByHandle(Handle,
        FileDispositionInfo, &DispositionInfo, sizeof DispositionInfo))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS CVDUFileSystem::Rename(
    PVOID FileNode,
    PVOID FileDesc,
    PWSTR FileName,
    PWSTR NewFileName,
    BOOLEAN ReplaceIfExists)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    WCHAR FullPath[FULLPATH_SIZE], NewFullPath[FULLPATH_SIZE];

    if (!ConcatPath(FileName, FullPath))
        return STATUS_OBJECT_NAME_INVALID;

    if (!ConcatPath(NewFileName, NewFullPath))
        return STATUS_OBJECT_NAME_INVALID;

    CString oldname = PathFindFileName(FileName);
    CString newname = PathFindFileName(NewFileName);

    //We handle renaming by requesting it from the server and waiting. 
    CVDUFile vdufile = APP->GetFileSystemService()->GetVDUFileByName(oldname);

    //Explanation for ReplaceIfExists:
    //  Some editors save files by creating a new temporary file with saved content, and renaming the old one
    //  with ReplaceIfExists tag. This is not a valid file renaming action that should be shared with the server
    if (vdufile.IsValid() && !ReplaceIfExists)
    {
        //We force close the handle prematurely, to get immediate access to our file
        CloseHandle(HandleFromFileDesc(FileDesc));
        HandleFromFileDesc(FileDesc) = INVALID_HANDLE_VALUE;

        //Handle renaming by requesting it from the server and waiting for result
        //Yes, its blocking, hopefully for not too long..
        INT result = APP->GetFileSystemService()->UpdateVDUFile(vdufile, newname);

        if (result != EXIT_SUCCESS)
            return STATUS_UNSUCCESSFUL;
    }

    if (!MoveFileEx(FullPath, NewFullPath, ReplaceIfExists ? MOVEFILE_REPLACE_EXISTING : 0))
        return NtStatusFromWin32(GetLastError());

    //After successful move, update file internally
    if (vdufile.IsValid() && !ReplaceIfExists)
    {
        vdufile.m_name = newname;
        APP->GetFileSystemService()->UpdateFileInternal(vdufile);
    }

    return STATUS_SUCCESS;
}

NTSTATUS CVDUFileSystem::GetSecurity(
    PVOID FileNode,
    PVOID FileDesc,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    SIZE_T* PSecurityDescriptorSize)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

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

NTSTATUS CVDUFileSystem::SetSecurity(
    PVOID FileNode,
    PVOID FileDesc,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ModificationDescriptor)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif
    
    HANDLE Handle = HandleFromFileDesc(FileDesc);

    if (!SetKernelObjectSecurity(Handle, SecurityInformation, ModificationDescriptor))
        return NtStatusFromWin32(GetLastError());

    return STATUS_SUCCESS;
}

NTSTATUS CVDUFileSystem::ReadDirectory(
    PVOID FileNode,
    PVOID FileDesc0,
    PWSTR Pattern,
    PWSTR Marker,
    PVOID Buffer,
    ULONG Length,
    PULONG PBytesTransferred)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    VdufsFileDesc* FileDesc = (VdufsFileDesc*)FileDesc0;
    return BufferedReadDirectory(&FileDesc->DirBuffer,
        FileNode, FileDesc, Pattern, Marker, Buffer, Length, PBytesTransferred);
}

NTSTATUS CVDUFileSystem::ReadDirectoryEntry(
    PVOID FileNode,
    PVOID FileDesc0,
    PWSTR Pattern,
    PWSTR Marker,
    PVOID* PContext,
    DirInfo* DirInfo)
{
#ifdef DEBUG_PRINT_FILESYSTEM_CALLS
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    fprintf(stderr, "[%s] %s", buf, __FUNCTION__"\n");
#endif

    VdufsFileDesc* FileDesc = (VdufsFileDesc*)FileDesc0;
    HANDLE Handle = FileDesc->Handle;
    WCHAR FullPath[FULLPATH_SIZE];
    ULONG Length, PatternLength;
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;

    if (*PContext == NULL)
    {
        if (!Pattern)
            Pattern = (PWSTR)_T("*");
        PatternLength = (ULONG)wcslen(Pattern);

        Length = GetFinalPathNameByHandleW(Handle, FullPath, FULLPATH_SIZE - 1, 0);
        if (0 == Length)
            return NtStatusFromWin32(GetLastError());
        if ((UINT64)Length + 1 + PatternLength >= FULLPATH_SIZE)
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

    //Check if file is in our accessible files vector
    /*if (wcscmp(FindData.cFileName, _T(".")) && wcscmp(FindData.cFileName, _T("..")))
    {
        if (!APP->GetFileSystemService()->GetVDUFileByName(FindData.cFileName))
            return STATUS_NO_MORE_FILES;
    }*/

    memset(DirInfo, 0, sizeof * DirInfo);
    Length = (ULONG)wcslen(FindData.cFileName);
    DirInfo->Size = (UINT16)(FIELD_OFFSET(CVDUFileSystem::DirInfo, FileNameBuf) + Length * sizeof(WCHAR));
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

CVDUFileSystemService::CVDUFileSystemService(CString DriveLetter) : Service(_T(PROGNAME)), m_fs(), m_host(m_fs), m_filesLock(SRWLOCK_INIT)//, m_hWorkDir(INVALID_HANDLE_VALUE)
{
    StringCchCopy(m_driveLetter, ARRAYSIZE(m_driveLetter), DriveLetter);
}

Fsp::FileSystemHost& CVDUFileSystemService::GetHost()
{
    return this->m_host;
}

CString CVDUFileSystemService::GetDrivePath()
{
    return CString(m_driveLetter) + _T("\\");
}

CString CVDUFileSystemService::GetWorkDirPath()
{
    return m_workDirPath;
}

CVDUFile CVDUFileSystemService::GetVDUFileByName(CString name)
{
    AcquireSRWLockExclusive(&m_filesLock);
    CVDUFile pFile = CVDUFile::InvalidFile;
    for (auto it = m_files.begin(); it != m_files.end(); it++)
    {
        CVDUFile& f = (*it);

        if (f.m_name.CompareNoCase(name) == 0)
        {
            pFile = f;
            break;
        }
    }
    ReleaseSRWLockExclusive(&m_filesLock);

    return pFile;
}

CVDUFile CVDUFileSystemService::GetVDUFileByToken(CString token)
{
    AcquireSRWLockExclusive(&m_filesLock);
    CVDUFile pFile = CVDUFile::InvalidFile;
    for (auto it = m_files.begin(); it != m_files.end(); it++)
    {
        CVDUFile& f = (*it);

        if (f.m_token == token)
        {
            pFile = f;
            break;
        }
    }
    ReleaseSRWLockExclusive(&m_filesLock);

    return pFile;
}

size_t CVDUFileSystemService::GetVDUFileCount()
{
    AcquireSRWLockExclusive(&m_filesLock);
    size_t size = m_files.size();
    ReleaseSRWLockExclusive(&m_filesLock);
    return size;
}

void CVDUFileSystemService::DeleteFileInternal(CString token)
{
    AcquireSRWLockExclusive(&m_filesLock);

    //DONT call the Get function, dont deadlock us...
    CVDUFile* pFile = NULL;
    for (auto it = m_files.begin(); it != m_files.end(); it++)
    {
        CVDUFile* f = &(*it);

        if (f->m_token == token)
        {
            pFile = f;
            break;
        }
    }

    CVDUFile vdufile = pFile ? *pFile : CVDUFile::InvalidFile;
    if (vdufile.IsValid()) //Already deleted?
    {
        //Delete the file on disk
        if (DeleteFile(GetWorkDirPath() + _T("\\") + vdufile.m_name))
        {
            BOOL deletedFile = FALSE;
            for (auto it = m_files.begin(); it != m_files.end(); it++)
            {
                CVDUFile f = (*it);

                if (f == vdufile)
                {
                    m_files.erase(it);
                    deletedFile = TRUE;
                    break;
                }
            }

            if (!deletedFile)
                WND->MessageBoxNB(_T("Failed to find file to delete!"), TITLENAME, MB_ICONWARNING);
        }
        else
        {
            WND->MessageBoxNB(_T("Failed to delete file!"), TITLENAME, MB_ICONWARNING);
        }
    }

    ReleaseSRWLockExclusive(&m_filesLock);
}

void CVDUFileSystemService::UpdateFileInternal(CVDUFile newfile)
{
    AcquireSRWLockExclusive(&m_filesLock);

    //DONT call the Get function, dont deadlock us...
    for (auto it = m_files.begin(); it != m_files.end(); it++)
    {
        CVDUFile& f = (*it);

        if (f.m_token == newfile.m_token)
        {
            //Now proceed to update the file internally
            f = newfile;
            break;
        }
    }

    ReleaseSRWLockExclusive(&m_filesLock);
}

NTSTATUS CVDUFileSystemService::OnStart(ULONG argc, PWSTR* argv)
{
   // PWSTR DebugLogFile = _T("vfsdebug.log");
    //ULONG DebugFlags = 0;
    //HANDLE DebugLogHandle = INVALID_HANDLE_VALUE;
    TCHAR PathBuf[MAX_PATH + 2] = { 0 }; //In case of SHFileOperation, path must be double zero terminated
    NTSTATUS Result;

    TCHAR tempBuf[MAX_PATH + 1] = { 0 };
    GetTempPath(ARRAYSIZE(tempBuf), tempBuf);

    CString randomFolderName = APP->GetProfileString(SECTION_SETTINGS, _T("WorkDir"), _T(""));

    //If not generated yet, or folder was deleted, generate a brand new one
    //In case the folder is not empty, program must have been shut down unsuccessfully
    //If user wants to recover unsaved files, the old folder will be there until Windows decides to remove it
    if (randomFolderName.IsEmpty() || GetFileAttributes(CString(tempBuf) + randomFolderName) == 0xFFFFFFFF ||
        !PathIsDirectoryEmpty(CString(tempBuf) + randomFolderName))
    {
        GUID guid;
        if (CoCreateGuid(&guid) != S_OK)
        {
            fail(_T("Cannot create GUID")); //TODO: Only one fail?
            AfxMessageBox(_T("Cannot create GUID."), MB_ICONERROR);
            WND->OnTrayExitCommand();
            return STATUS_UNSUCCESSFUL;
        }

        if (StringFromGUID2(guid, PathBuf, ARRAYSIZE(PathBuf)) <= 0)
        {
            fail(_T("Cannot create random string"));
            WND->OnTrayExitCommand();
            return STATUS_UNSUCCESSFUL;
        }

        //Generated a new guid
        randomFolderName = PathBuf;
        APP->WriteProfileString(SECTION_SETTINGS, _T("WorkDir"), randomFolderName);
    }

    swprintf_s(PathBuf, _T("%s%s"), tempBuf, randomFolderName.GetBuffer());

    if (CreateDirectory(PathBuf, NULL))
        SetFileAttributes(PathBuf, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_READONLY);

    EnableBackupRestorePrivileges();

    Result = m_fs.SetPath(PathBuf);
    if (!NT_SUCCESS(Result))
    {
        fail(_T("cannot create file system"));
        return Result;
    }

    m_workDirPath = PathBuf;
    m_host.SetFileSystemName(_T("VDU"));

    Result = m_host.Mount(m_driveLetter);
    if (!NT_SUCCESS(Result))
    {
        fail(_T("cannot mount file system"));
        return Result;
    }

#if defined(_DEBUG) && defined(DEBUG_PRINT_FILESYSTEM_CALLS)
    if (AllocConsole())
    {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    return STATUS_SUCCESS;
}

NTSTATUS CVDUFileSystemService::OnStop()
{
#if defined(_DEBUG) && defined(DEBUG_PRINT_FILESYSTEM_CALLS)
    FreeConsole();
#endif
    m_host.Unmount();
    return STATUS_SUCCESS;
}

CString CVDUFileSystemService::CalcFileMD5Base64(CVDUFile file)
{
    CString finalHash;
    BYTE rgbHash[MD5_LEN] = { 0 };
    CString filePath = GetWorkDirPath() + _T("\\") + file.m_name;

    HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD lastError = GetLastError();
        return finalHash;
    }

    HCRYPTPROV hProv;
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        //err?
        CloseHandle(hFile);
        return finalHash;
    }

    HCRYPTPROV hHash;
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return finalHash;
    }

    BYTE rgbFile[0x400];
    DWORD readLen;
    BOOL bResult = FALSE;
    while (bResult = ReadFile(hFile, rgbFile, ARRAYSIZE(rgbFile), &readLen, NULL))
    {
        if (readLen <= 0)
            break;
        if (!CryptHashData(hHash, rgbFile, readLen, 0))
        {
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return finalHash;
        }
    }

    if (!bResult)
    {
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        return finalHash;
    }

    DWORD cbHash = MD5_LEN;
    bResult = CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    if (bResult)
    {
        BYTE md5base64[0x400] = { 0 };
        INT md5base64len = ARRAYSIZE(md5base64);
        Base64Encode(rgbHash, cbHash, (LPSTR)md5base64, &md5base64len);
        finalHash = CString(md5base64);
    }

    return finalHash;
}

NTSTATUS CVDUFileSystemService::Remount(CString DriveLetter)
{
    if (m_host.MountPoint() && wcslen(m_host.MountPoint()) > 0) 
        m_host.Unmount();

    StringCchCopy(m_driveLetter, ARRAYSIZE(m_driveLetter), DriveLetter);
    return m_host.Mount((PWSTR)m_driveLetter);
}

BOOL CVDUFileSystemService::CreateVDUFile(CVDUFile vdufile, CHttpFile* httpfile)
{
    if (GetVDUFileByToken(vdufile.m_token).IsValid())
        return FALSE;

    TCHAR tempBuf[MAX_PATH + 1] = { 0 };
    GetTempPath(ARRAYSIZE(tempBuf), tempBuf);

    TCHAR tmpFilePath[MAX_PATH] = { 0 };
    if (GetTempFileName(tempBuf, _T("vdu"), 0, tmpFilePath) < 1)
        return FALSE;

    HANDLE hFile = CreateFile(tmpFilePath, GENERIC_ALL, NULL, NULL, CREATE_ALWAYS, NULL, NULL);

    //Cant open file?
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    TRY
    {
        WND->GetProgressBar()->SetState(PBST_NORMAL);
        WND->GetProgressBar()->SetPos(0);
        WND->UpdateStatus();

        BYTE buf[0x1000] = { 0 };
        UINT readLen, readTotal = 0;
        while ((readLen = httpfile->Read(buf, ARRAYSIZE(buf))) > 0)
        {
            if (!WriteFile(hFile, buf, readLen, NULL, NULL))
            {
                DWORD errid = GetLastError();
                return FALSE;
            }
            readTotal += readLen;
            int newpos = (int) (((double)readTotal / vdufile.m_length) * 100);
            if (newpos != WND->GetProgressBar()->GetPos())
            {
                WND->GetProgressBar()->SetPos(newpos);
                WND->UpdateStatus();
            }
        }
    } CATCH(CInternetException, e)
    {
        e->GetErrorMessage(CVDUConnection::LastError, ARRAYSIZE(CVDUConnection::LastError));
        CloseHandle(hFile);
        DeleteFile(tmpFilePath);
        WND->GetProgressBar()->SetState(PBST_ERROR);
        WND->UpdateStatus();
         
        return FALSE;
    }
    END_CATCH;

    WND->GetProgressBar()->SetState(PBST_PAUSED);
    WND->GetProgressBar()->SetPos(0);
    WND->UpdateStatus();

    FILETIME lastModified;
    if (SystemTimeToFileTime(&vdufile.m_lastModified, &lastModified))
        SetFileTime(hFile, NULL, NULL, &lastModified);

    CloseHandle(hFile);

    //Make sure directory exists
    if (CreateDirectory(GetWorkDirPath(), NULL))
        SetFileAttributes(GetWorkDirPath(), FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_READONLY);

    CString finalPath = GetWorkDirPath() + _T("\\") + vdufile.m_name;
    if (!MoveFileEx(tmpFilePath, finalPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED))
    {
        DWORD errid = GetLastError();
        DeleteFile(tmpFilePath);
        return FALSE;
    }


    //Make sure MD5 hash matches
    if (CalcFileMD5Base64(vdufile) != vdufile.m_md5base64)
    {
        //MD5 hash check failed?
        DeleteFile(finalPath);
        return FALSE;
    }

    m_files.push_back(vdufile);

    //Put this into main thread
    //ShutdownBlockReasonCreate(WND->GetSafeHwnd(), _T("There are still unsaved files!"));
    return TRUE;
}

INT CVDUFileSystemService::UpdateVDUFile(CVDUFile vdufile, CString newName)
{
    CString length;
    length.Format(_T("%d"), vdufile.m_length);
    CString headers;
    headers += _T("Content-Encoding: ") + vdufile.m_encoding + _T("\r\n");
    headers += _T("Content-Type: ") + vdufile.m_type + _T("\r\n");
    headers += _T("Content-Location: ") + (newName.IsEmpty() ? vdufile.m_name : newName) + _T("\r\n");
    headers += _T("Content-Length: ") + length + _T("\r\n");
    headers += _T("Content-MD5: ") + CalcFileMD5Base64(vdufile) + _T("\r\n");

    //If sync, we wait for this thread to finish to get its exit code
    if (newName.IsEmpty())
    {
        AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)new CVDUConnection(APP->GetSession()->GetServerURL(), VDUAPIType::POST_FILE,
            CVDUSession::CallbackUploadFile, headers, vdufile.m_token, GetWorkDirPath() + _T("\\") + vdufile.m_name));
    }
    else
    {
        CWinThread* t = AfxBeginThread(CVDUConnection::ThreadProc, (LPVOID)new CVDUConnection(APP->GetSession()->GetServerURL(), VDUAPIType::POST_FILE,
            CVDUSession::CallbackUploadFile, headers, vdufile.m_token, GetWorkDirPath() + _T("\\") + vdufile.m_name), 0, CREATE_SUSPENDED);
        t->m_bAutoDelete = FALSE;
        DWORD resumeResult = t->ResumeThread();

        if (resumeResult != 0xFFFFFFFF) //Should not happen, but dont get stuck
            WaitForSingleObject(t->m_hThread, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeThread(t->m_hThread, &exitCode);

        //With m_bAutoDelete we are responsibile for deleting the thread
        delete t;
        return exitCode;
    }

    return EXIT_SUCCESS;
}

void CVDUFileSystemService::DeleteVDUFile(CVDUFile vdufile)
{
    AfxBeginThread(CVDUConnection::ThreadProc,
        (LPVOID)new CVDUConnection(APP->GetSession()->GetServerURL(), VDUAPIType::DELETE_FILE, CVDUSession::CallbackInvalidateFileToken, _T(""), vdufile.m_token));
}