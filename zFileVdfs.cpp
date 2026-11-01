#include "include/zDiskVdfs.h"
#include <cstring>
#include <windows.h>

extern "C" {
long vdf_fopen(const char* fileName, long mode);
long vdf_fexists(const char* fileName, long mode);
long vdf_fclose(long handle);
long vdf_ffilesize(long handle);
long vdf_fseek(long handle, long position);
long vdf_ftell(long handle);
const char* vdf_getdir();
const char* vdf_getlasterror();
long vdf_searchfile(const char* fileName, char* result);
long vdf_findopen(const char* mask, long mode);
long vdf_findnext(long handle, TVDFFINDDATA* data);
long vdf_findclose(long handle);
long vdf_fread(long handle, void* data, long size);
long vdf_initall(long reserved, const char* root, long* numFiles, long* numDrives);
void vdf_exitall();
}

bool zFILE_VDFS::s_vdfsInitialized;
bool zFILE_VDFS::s_physical_first;
zCCriticalSection* zFILE_VDFS::s_criticalSection;

zFILE_VDFS::zFILE_VDFS()
    : vdfHandle(-1), vdfResult(0), vdfEOF(false), vdfFindHandle(0)
{
    Init("");
}

zFILE_VDFS::zFILE_VDFS(const zSTRING& path)
    : zFILE_FILE(path), vdfHandle(-1), vdfResult(0), vdfEOF(false), vdfFindHandle(0)
{
    Init(path);
}

zFILE_VDFS::~zFILE_VDFS()
{
    Close();
    vdfHandle = -1;
    vdfFindHandle = 0;
}

void zFILE_VDFS::Init(const zSTRING&)
{
    find_filedata.datetime = 0;
    find_filedata.size = -1;
    find_filedata.attr = 0;
    SetMode(s_physical_first ? 7 : 3);
}

int zFILE_VDFS::HandleError(const zSTRING& operation, const zSTRING& path, zERROR_TYPE type)
{
    return zFILE_FILE::HandleError(operation, path, type);
}

bool zFILE_VDFS::IsOpened()
{
    if (!s_vdfsInitialized || write)
        return file_handle != 0;
    return vdfHandle >= 0;
}

int zFILE_VDFS::Open(const zSTRING& path, bool writeMode)
{
    SetPath(path);
    return Open(writeMode);
}

int zFILE_VDFS::Open(bool writeMode)
{
    if (!s_vdfsInitialized || writeMode)
        return zFILE_FILE::Open(writeMode);
    if (vdfHandle >= 0) {
        zerr->Warning(zSTRING("B: VFILE: File is still opened. ") + GetFilename());
        return 0;
    }
    if (s_criticalSection)
        s_criticalSection->Lock(0xffffffff);
    vdfHandle = vdf_fopen(GetFullPath().ToChar(), GetMode());
    int result = 0;
    if (vdfHandle < 0) {
        zerr->Warning(zSTRING("B: VFILE: ") + vdf_getlasterror());
        result = 0x13ec;
    }
    if (s_criticalSection)
        s_criticalSection->Unlock();
    return result;
}

bool zFILE_VDFS::Exists(const zSTRING& path)
{
    SetPath(path);
    return Exists();
}

bool zFILE_VDFS::Exists()
{
    if (!s_vdfsInitialized)
        return zFILE_FILE::Exists();
    if (s_criticalSection)
        s_criticalSection->Lock(0xffffffff);
    find_filedata.size = vdf_fexists(GetFullPath().ToChar(), GetMode());
    if (s_criticalSection)
        s_criticalSection->Unlock();
    return find_filedata.size > 0;
}

int zFILE_VDFS::Close()
{
    zFILE_FILE::Close();
    if (s_vdfsInitialized && vdfHandle >= 0) {
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        find_filedata.size = vdf_fclose(vdfHandle);
        vdfHandle = -1;
        if (s_criticalSection)
            s_criticalSection->Unlock();
    }
    return 0;
}

int zFILE_VDFS::Reset()
{
    return Seek(0);
}

long zFILE_VDFS::Size()
{
    if (s_vdfsInitialized && vdfHandle >= 0) {
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        find_filedata.size = vdf_ffilesize(vdfHandle);
        if (s_criticalSection)
            s_criticalSection->Unlock();
        return find_filedata.size;
    }
    return zFILE_FILE::Size();
}

bool zFILE_VDFS::Eof()
{
    if (s_vdfsInitialized && vdfHandle >= 0)
        return vdfEOF;
    return zFILE_FILE::Eof();
}

int zFILE_VDFS::Seek(long position)
{
    if (!s_vdfsInitialized || vdfHandle < 0)
        return zFILE_FILE::Seek(position);
    if (s_criticalSection)
        s_criticalSection->Lock(0xffffffff);
    find_filedata.size = vdf_fseek(vdfHandle, position);
    if (find_filedata.size >= 0)
        vdfEOF = false;
    if (s_criticalSection)
        s_criticalSection->Unlock();
    return 0;
}

long zFILE_VDFS::Pos()
{
    if (s_vdfsInitialized && vdfHandle >= 0) {
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        find_filedata.size = vdf_ftell(vdfHandle);
        if (s_criticalSection)
            s_criticalSection->Unlock();
        return find_filedata.size;
    }
    return zFILE_FILE::Pos();
}

void zFILE_VDFS::Append()
{
    Seek(Size());
}

zSTRING zFILE_VDFS::SetCurrentDir()
{
    if (s_criticalSection)
        s_criticalSection->Lock(0xffffffff);
    zSTRING directory = vdf_getdir();
    if (s_criticalSection)
        s_criticalSection->Unlock();
    SetPath(directory + "\\");
    return GetPath();
}

zSTRING zFILE_VDFS::GetFullPath()
{
    const zSTRING path = GetPath();
    if (path.Length() && path[0] == '\\')
        return path;
    return zFILE::s_virtPathString + path;
}

int zFILE_VDFS::ChangeDir(bool)
{
    zFILE_FILE::ChangeDir(false);
    return 0;
}

zSTRING zFILE_VDFS::SeekText(const zSTRING& text)
{
    if (!s_vdfsInitialized || vdfHandle < 0)
        return zFILE_FILE::SeekText(text);
    zSTRING line;
    while (true) {
        Read(line);
        if (line.Search(text) >= 0)
            return line;
        if (Eof())
            return "";
    }
}

int zFILE_VDFS::GetStats(zFILE_STATS& stats)
{
    return zFILE_FILE::GetStats(stats);
}

int zFILE_VDFS::SearchFile(const zSTRING& filePath, const zSTRING& fileName, int recursive)
{
    find_filedata.size = -1;
    if (s_vdfsInitialized) {
        char result[1024] = {};
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        find_filedata.size = vdf_searchfile(GetFullPath().ToChar(), result);
        if (s_criticalSection)
            s_criticalSection->Unlock();
        if (find_filedata.size > 0) {
            char* separator = std::strchr(result, '|');
            if (separator)
                *separator = 0;
            SetPath(result);
            return 0;
        }
    }
    return zFILE_FILE::SearchFile(filePath, fileName, recursive);
}

bool zFILE_VDFS::FindFirst(const zSTRING& mask)
{
    if (vdfFindHandle) {
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        vdf_findclose(vdfFindHandle);
        vdfFindHandle = 0;
        if (s_criticalSection)
            s_criticalSection->Unlock();
    }
    if (mask.SearchRev("\\", 1) >= 0)
        vdfResult = mask.Copied(0, mask.SearchRev("\\", 1)) + vdfHandle + "|";
    if (vdfResult == ".*|")
        vdfResult.Clear();
    vdfHandle.Upper();
    if (s_criticalSection)
        s_criticalSection->Lock(0xffffffff);
    vdfFindHandle = vdf_findopen((zFILE::s_virtPathString + mask).ToChar(), GetMode());
    if (s_criticalSection)
        s_criticalSection->Unlock();
    if (!vdfFindHandle)
        return false;
    return FindNext();
}

bool zFILE_VDFS::FindNext()
{
    if (!vdfFindHandle)
        return false;
    while (true) {
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        const long result = vdf_findnext(vdfFindHandle, &find_filedata);
        if (s_criticalSection)
            s_criticalSection->Unlock();
        if (result < 0) {
            vdfFindHandle = 0;
            return false;
        }
        if (find_filedata.attr & 0x10)
            continue;
        zSTRING name = find_filedata.name;
        name.Upper();
        if (!vdfFindExt.IsEmpty() && name.Search(vdfFindExt) < 0)
            continue;
        SetPath(find_filedata.name);
        return true;
    }
}

zSTRING operator+(char first, const zSTRING& second)
{
    return zSTRING(first) + second;
}

int zFILE_VDFS::Read(char* text)
{
    if (!s_vdfsInitialized)
        return zFILE_FILE::Read(text);
    text[0] = 0;
    while (true) {
        char value;
        if (s_criticalSection)
            s_criticalSection->Lock(0xffffffff);
        const long result = vdf_fread(vdfHandle, &value, 1);
        if (s_criticalSection)
            s_criticalSection->Unlock();
        if (result < 1) {
            vdfEOF = true;
            break;
        }
        if (value == '\n')
            break;
        const size_t length = std::strlen(text);
        text[length] = value;
        text[length + 1] = 0;
        if (vdfEOF)
            break;
    }
    return 0;
}

int zFILE_VDFS::Read(zSTRING& text)
{
    if (!s_vdfsInitialized)
        return zFILE_FILE::Read(text);
    char buffer[256];
    Read(buffer);
    text = buffer;
    while (text.Length() > 0 && text[text.Length() - 1] == '\n')
        text.DeleteRight(1);
    while (text.Length() > 0 && text[text.Length() - 1] == '\r')
        text.DeleteRight(1);
    return 0;
}

long zFILE_VDFS::Read(void* data, long size)
{
    if (!s_vdfsInitialized)
        return zFILE_FILE::Read(data, size);
    if (s_criticalSection)
        s_criticalSection->Lock(0xffffffff);
    find_filedata.size = vdf_fread(vdfHandle, data, size);
    if (s_criticalSection)
        s_criticalSection->Unlock();
    if (find_filedata.size < 0) {
        zerr->Warning(zSTRING("B: VFILE: Error while reading from file ") + GetFile());
        return find_filedata.size;
    }
    if (find_filedata.size < size)
        vdfEOF = true;
    return find_filedata.size;
}

bool zFILE_VDFS::InitFileSystem()
{
    zerr->Message("B: VFILE: Initializing filesystem (VDFS)");
    zFILE_FILE::InitFileSystem();
    char modulePath[260] = {};
    char helperPath[260] = {};
    GetModuleFileNameA(NULL, modulePath, 260);
    char* separator = std::strrchr(modulePath, '\\');
    if (!separator)
        separator = std::strrchr(modulePath, '/');
    if (separator)
        *separator = 0;
    std::strcpy(helperPath, modulePath);
    std::strcat(modulePath, "\\Vdfs32g.exe");
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = 5;
    PROCESS_INFORMATION process = {};
    if (CreateProcessA(modulePath, NULL, NULL, NULL, FALSE, 0, NULL, helperPath,
        &startup, &process)) {
        WaitForSingleObject(process.hProcess, INFINITE);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
    delete s_criticalSection;
    s_criticalSection = new zCCriticalSection;
    long numFiles = 0;
    long numDrives = 0;
    sysEvent();
    const long result = vdf_initall(0, "G2ADDON1", &numFiles, &numDrives);
    sysEvent();
    s_vdfsInitialized = result == 0;
    if (s_vdfsInitialized)
        zerr->Message("B: VFILE: VDFS successfully initialized: " + zSTRING(numDrives)
            + " drives, " + zSTRING(numFiles) + " found.");
    else if (result == -1)
        zerr->Warning("B: VFILE: VDFS not initialized (-1: No Files)");
    else
        zerr->Warning(zSTRING("B: VFILE: VDFS not initialized (") + vdf_getlasterror() + ")");
    return s_vdfsInitialized;
}

bool zFILE_VDFS::DeinitFileSystem()
{
    zerr->Message("B: VFILE: Deinitializing filesystem (VDFS)");
    if (s_vdfsInitialized) {
        vdf_exitall();
        s_vdfsInitialized = false;
    } else {
        zerr->Warning("B: VFILE: VDFS not initialized");
    }
    delete s_criticalSection;
    s_criticalSection = NULL;
    return zFILE_FILE::DeinitFileSystem();
}

bool zFILE_VDFS::LockCriticalSection()
{
    if (!s_criticalSection)
        return false;
    s_criticalSection->Lock(0xffffffff);
    return true;
}

bool zFILE_VDFS::UnlockCriticalSection()
{
    if (!s_criticalSection)
        return false;
    s_criticalSection->Unlock();
    return true;
}
