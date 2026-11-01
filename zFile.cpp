#include "zFile.h"
#include "include/zThread.h"

#include <cstdio>
#include <cerrno>
#include <ctime>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>

static int fileSystemState = -1;
static int filePathState = -1;
zSTRING zFILE::s_virtPathString;
zSTRING zFILE::s_physPathString;
zSTRING zFILE::s_rootDirString;
zCCriticalSection* zFILE::s_dirscanCiticalSection;
zFILE* zFILE::s_virtPath = new zFILE_FILE;
zCBuffer* zFILE::bufferList[6];
int zFILE::bufferFlag[6];
int zFILE::s_numFilesOpen;
int zFILE::s_maxFilesOpen;
bool zFILE::s_initialized;

static int directFileAccessDepth;

zSTRING zFILE::SetRootDirectory(zSTRING directory)
{
    s_rootDirString = directory;
    if (s_rootDirString.Length() > 0 && s_rootDirString[s_rootDirString.Length() - 1] == '\\')
        s_rootDirString.DeleteRight(1);
    s_rootDirString.Upper();
    return s_rootDirString;
}

void zFILE::DirectFileConvert(zSTRING& fileName)
{
    if (fileName.Length() > 0 && fileName[0] == '\\')
        fileName = s_rootDirString + fileName;
}

void zFILE::DirectFileAccess(bool direct)
{
    if (direct) {
        if (_chdir(s_physPathString.ToChar()))
            zerr->Warning(zSTRING("B: (DirectFileAccess) Couldn't change to directory: ") + s_physPathString);
        ++directFileAccessDepth;
        return;
    }
    const int oldDepth = directFileAccessDepth--;
    if (oldDepth == 1) {
        _chdir(s_rootDirString.ToChar());
        return;
    }
    if (directFileAccessDepth < 0) {
        zerr->Warning("B: zDSK: Unexpected DirectFileAccess(false)");
        directFileAccessDepth = 0;
    }
}

void zFILE::DirectFileAccess(bool direct, zSTRING& fileName)
{
    DirectFileConvert(fileName);
    DirectFileAccess(direct);
}

long zFILE::GetFreeDiskSpace()
{
    return 0x19000;
}

int zFILE::LockDirScan()
{
    if (!s_dirscanCiticalSection)
        return 0;
    s_dirscanCiticalSection->Lock(0xffffffff);
    return 1;
}

int zFILE::UnlockDirScan()
{
    if (!s_dirscanCiticalSection)
        return 0;
    s_dirscanCiticalSection->Unlock();
    return 1;
}

int operator==(const zDATE& first, const zDATE& second)
{
    return first.year == second.year && first.month == second.month && first.day == second.day &&
        first.hour == second.hour && first.minute == second.minute && first.second == second.second;
}

int operator<(const zDATE& first, const zDATE& second)
{
    if (first.year != second.year)
        return first.year < second.year;
    if (first.month != second.month)
        return first.month < second.month;
    if (first.day != second.day)
        return first.day < second.day;
    if (first.hour != second.hour)
        return first.hour < second.hour;
    if (first.minute != second.minute)
        return first.minute < second.minute;
    return first.second < second.second;
}

zDATE& zDATE::operator=(long value)
{
    time_t timeValue = (time_t)value;
    tm* converted = std::localtime(&timeValue);
    second = converted->tm_sec;
    minute = converted->tm_min;
    hour = converted->tm_hour;
    day = converted->tm_mday;
    month = converted->tm_mon + 1;
    year = converted->tm_year + 1900;
    return *this;
}

long zDATE::ToTime_t()
{
    tm converted = {};
    converted.tm_sec = second;
    converted.tm_min = minute;
    converted.tm_hour = hour;
    converted.tm_mday = day;
    converted.tm_mon = month - 1;
    converted.tm_year = year - 1900;
    return (long)std::mktime(&converted);
}

zSTRING zDATE::ToString(zTDATE_CONVERT conversion)
{
    zSTRING result;
    if (conversion == zDATE_ALL || conversion == zDATE_DATE)
        result.Sprintf("%i.%i.%i", day, month, year);
    if ((conversion == zDATE_ALL || conversion == zDATE_TIME) && (hour || minute || second)) {
        zSTRING time;
        time.Sprintf("  %i:%i:%i", hour, minute, second);
        result += time;
    }
    return result;
}

static FILE* GetStdFile(zFILE_FILE* file)
{
    return reinterpret_cast<FILE*>(file->file_handle);
}

zFILE_FILE::zFILE_FILE()
{
    Init(zSTRING());
}

zFILE_FILE::zFILE_FILE(const zSTRING& path)
{
    Init(path);
}

zFILE_FILE::~zFILE_FILE()
{
    if (file_handle)
        Close();

    buffer = 0;
}

void zFILE_FILE::Init(const zSTRING& path)
{
    buffer = 0;
    write = false;
    buffering = false;
    error = 0;
    file_handle = 0;
    find_handle = 0;
    file_mode = 0;
    SetPath(path);
}

int zFILE_FILE::HandleError(const zSTRING& operation, const zSTRING& path, zERROR_TYPE type)
{
    int result;
    zSTRING text;
    switch (errno) {
    case ENOENT:
        result = 0x13f2;
        text = "No such file or directory. (ENOENT)";
        break;
    case ENOEXEC:
        result = 0x13f3;
        text = "File is not executable. (ENOEXEC)";
        break;
    case EBADF:
        result = 0x13f0;
        text = "Bad file number. Possibly the file handle is not valid. (EBADF)";
        break;
    case EACCES:
        result = 0x13ed;
        text = "Permission denied. (EACCES)";
        break;
    case EEXIST:
        result = 0x13ee;
        text = "Filename already exists. (EEXIST)";
        break;
    case EXDEV:
        result = 0x13f5;
        text = "Cross-device link. An attempt was made to move a file to a different device. (EXDEV)";
        break;
    case EINVAL:
        result = 0x13ef;
        text = "??? Invalid flag or pmode argument. (EINVAL)";
        break;
    case EMFILE:
        result = 0x13f1;
        text = "Too many open files. No more file handles are available. (EMFILE)";
        break;
    case ENOSPC:
        result = 0x13f4;
        text = "No space left on device. Maybe your disk is full. (ENOSPC)";
        break;
    case 0x13f6:
        result = 0x13f6;
        text = "Don't set new filename while file is opened.";
        break;
    default:
        result = 0x13ec;
        text = "Unknown file-error! ";
        break;
    }
    zSTRING message = zSTRING("B: zDSK: ") + text + " in " + operation;
    if (path.Length())
        message += zSTRING(" -> \"") + path + "\"";
    if (type == zERR_TYPE_FATAL)
        zerr->Warning(message);
    else
        zerr->Message(message);
    error = result;
    return result;
}

bool zFILE_FILE::IsOpened()
{
    return file_handle != 0;
}

void zFILE_FILE::SetMode(long mode)
{
    file_mode = mode;
}

long zFILE_FILE::GetMode()
{
    return file_mode;
}

zGOTHIC_FILE* zFILE_FILE::GetFileHandle()
{
    return file_handle;
}

int zFILE_FILE::Close()
{
    if (!file_handle)
        return 0;

    if (buffer)
        FlushBuffer();

    std::fflush(GetStdFile(this));
    const int result = std::fclose(GetStdFile(this));

    if (!result)
        file_handle = 0;

    return result;
}

int zFILE_FILE::Reset()
{
    return Seek(0);
}

long zFILE_FILE::Size()
{
    if (!file_handle)
        return -1;

    const long position = Pos();
    std::fseek(GetStdFile(this), 0, SEEK_END);
    const long size = std::ftell(GetStdFile(this));
    std::fseek(GetStdFile(this), position, SEEK_SET);
    return size;
}

long zFILE_FILE::Pos()
{
    return std::ftell(GetStdFile(this));
}

int zFILE_FILE::Seek(long position)
{
    std::fseek(GetStdFile(this), position, SEEK_SET);
    return 0;
}

bool zFILE_FILE::Eof()
{
    return (file_handle->_flag >> 4) & 1;
}

long zFILE_FILE::FlushBuffer()
{
    if (!buffer)
        return 0;

    const long result = std::fwrite(buffer->GetBuffer(), buffer->GetSize(), 1, GetStdFile(this));
    buffer->Reset();
    return result;
}

static void NormalizeFilePart(zSTRING& value)
{
    for (int index = 0; index < value.Length(); ++index) {
        if (value[index] == '/')
            value[index] = '\\';
    }

    value.Upper();
}

void zFILE_FILE::SetPath(zSTRING path)
{
    drive.Clear();
    dir.Clear();
    filename.Clear();
    ext.Clear();
    NormalizeFilePart(path);
    const int pathDrive = path.Search(":", 1);
    const int rootDrive = zFILE::s_rootDirString.Search(":", 1);
    if (pathDrive >= 0 && rootDrive >= 0) {
        drive = path.Copied(0, pathDrive);
        if (drive == zFILE::s_rootDirString.Copied(0, rootDrive))
            drive.Clear();
        path.Delete(0, pathDrive + 1);
    }
    const int lastDot = path.SearchRev(".", 1);
    const int lastSlash = path.SearchRev("\\", 1);
    if (lastDot > lastSlash && lastDot >= 0) {
        ext = path.Copied(lastDot + 1, path.Length() - lastDot - 1);
        path.Delete(lastDot, path.Length() - lastDot);
    }
    if (lastSlash < 0) {
        filename = path;
        path.Clear();
    } else {
        filename = path.Copied(lastSlash + 1, path.Length() - lastSlash - 1);
        path.Delete(lastSlash + 1, path.Length() - lastSlash - 1);
    }
    dir = path;
    SetCompletePath();
}

zSTRING zFILE_FILE::GetFullPath()
{
    zSTRING path = GetPath();
    if (path.Length() && path[0] == '\\')
        return zFILE::s_rootDirString + path;
    if (path.Length() > 1 && path[1] == ':')
        return path;
    return zFILE::s_rootDirString + zFILE::s_virtPathString + path;
}

zSTRING& zFILE_FILE::SlashConvert(zSTRING& path)
{
    for (int i = 0; i < path.Length(); ++i) {
        if (path[i] == '/')
            path[i] = '\\';
    }
    return path;
}

bool zFILE_FILE::Exists()
{
    if (file_handle)
        return true;

    return _access(GetFullPath().ToChar(), 0) == 0;
}

bool zFILE_FILE::Exists(const zSTRING& path)
{
    SetPath(path);
    return Exists();
}

int zFILE_FILE::Open(bool write)
{
    if (file_handle)
        return 0;

    if (!complete_path.Length())
        return 2;

    file_handle = reinterpret_cast<zGOTHIC_FILE*>(std::fopen(complete_path.ToChar(), write ? "wb" : "rb"));
    return file_handle ? 0 : 1;
}

int zFILE_FILE::Open(const zSTRING& path, bool write)
{
    SetPath(path);
    return Open(write);
}

int zFILE_FILE::Create()
{
    if (!complete_path.Length())
        return 2;

    file_handle = reinterpret_cast<zGOTHIC_FILE*>(std::fopen(complete_path.ToChar(), "wb"));
    return file_handle ? 0 : 1;
}

int zFILE_FILE::Create(const zSTRING& path)
{
    SetPath(path);
    return Create();
}

int zFILE_FILE::Write(const char* text)
{
    if (buffer) {
        buffer->Write(text, std::strlen(text));
        return 0;
    }

    if (std::fputs(text, GetStdFile(this)) < 0)
        return -1;

    std::fflush(GetStdFile(this));
    return 0;
}

int zFILE_FILE::Write(const zSTRING& text)
{
    return Write(text.ToChar());
}

long zFILE_FILE::Write(const void* data, long size)
{
    if (!size)
        return 0;

    if (buffer) {
        buffer->Write(data, size);
        return size;
    }

    return std::fwrite(data, size, 1, GetStdFile(this)) * size;
}

int zFILE_FILE::ReadChar(char& value)
{
    return Read(&value, 1) == 1 ? 0 : 2;
}

int zFILE_FILE::Read(char* text)
{
    std::fgets(text, 0x400, GetStdFile(this));
    return 0;
}

int zFILE_FILE::Read(zSTRING& text)
{
    text.Clear();
    char chunk[0x400];

    while (std::fgets(chunk, sizeof(chunk), GetStdFile(this))) {
        text.Append(zSTRING(chunk));
        const size_t length = std::strlen(chunk);

        if (length && chunk[length - 1] == '\n')
            break;
    }

    return 0;
}

long zFILE_FILE::Read(void* data, long size)
{
    return std::fread(data, size, 1, GetStdFile(this)) * size;
}

zSTRING zFILE_FILE::SeekText(const zSTRING& text)
{
    zSTRING line;

    while (!Eof()) {
        Read(line);

        if (line.Search(text) >= 0)
            return line;
    }

    return zSTRING();
}

zSTRING zFILE_FILE::ReadBlock(long start, long numLines)
{
    Seek(start);
    zSTRING result;
    ReadLines(result, numLines);
    return result;
}

int zFILE_FILE::UpdateBlock(const zSTRING& text, long start, long numLines)
{
    zFILE* backupFile = zfactory->CreateZFile(complete_path);
    backupFile->SetExt("bak");
    FileCopy(backupFile);
    backupFile->Open(false);
    Create();
    const long sourceSize = backupFile->Size();
    char* source = new char[sourceSize + 1];
    const long sourceRead = backupFile->Read(source, sourceSize);
    Write(source, sourceRead);
    Write(text);
    backupFile->ReadBlock(start, numLines);
    const long remainingSize = backupFile->Size() - backupFile->Pos();
    const long remainingRead = backupFile->Read(source, remainingSize);
    Write(source, remainingRead);
    backupFile->Close();
    Close();
    delete backupFile;
    delete[] source;
    return 0;
}

int zFILE_FILE::ReadCodeline(zSTRING& text)
{
    if (Eof()) {
        text = zSTRING("<EOF>");
        return 0;
    }

    Read(text);

    if (text.Length() && text[text.Length() - 1] == '\n')
        text.DeleteRight(1);

    return 0;
}

int zFILE_FILE::ReadLines(zSTRING& text, long numLines)
{
    for (long index = 0; index < numLines; ++index) {
        zSTRING line;
        Read(line);
        text.Append(line);
    }

    return 0;
}

void zFILE_FILE::Append()
{
    std::fseek(GetStdFile(this), 0, SEEK_END);
}

static void SetFileDate(zDATE& date, time_t value)
{
    const tm* parts = std::localtime(&value);
    date.second = parts->tm_sec;
    date.minute = parts->tm_min;
    date.hour = parts->tm_hour;
    date.day = parts->tm_mday;
    date.month = parts->tm_mon + 1;
    date.year = parts->tm_year + 1900;
}

int zFILE_FILE::GetStats(zFILE_STATS& stats)
{
    if (!Exists())
        return 0x13EE;

    struct _stat info;
    if (_stat(GetFullPath().ToChar(), &info))
        return 0x13EE;

    SetFileDate(stats.createDate, info.st_ctime);
    SetFileDate(stats.accessDate, info.st_atime);
    SetFileDate(stats.modifyDate, info.st_mtime);
    stats.size = info.st_size;
    return 0;
}

zSTRING zFILE_FILE::GetPath()
{
    return complete_path;
}

zSTRING zFILE_FILE::GetDirectoryPath()
{
    return complete_directory;
}

zSTRING zFILE_FILE::GetDrive()
{
    return drive;
}

zSTRING zFILE_FILE::GetDir()
{
    return dir;
}

zSTRING zFILE_FILE::GetFile()
{
    zSTRING result = filename;
    result.Append(zSTRING("."));
    result.Append(ext);
    return result;
}

zSTRING zFILE_FILE::GetFilename()
{
    return filename;
}

zSTRING zFILE_FILE::GetExt()
{
    return ext;
}

void zFILE_FILE::SetDrive(zSTRING value)
{
    NormalizeFilePart(value);
    drive = value;
    drive.Upper();
    SetCompletePath();
}

void zFILE_FILE::SetDir(zSTRING value)
{
    NormalizeFilePart(value);

    if (value.Length() && value[value.Length() - 1] != '\\')
        value.Append(zSTRING("\\"));

    dir = value;
    dir.Upper();
    SetCompletePath();
}

void zFILE_FILE::SetFile(zSTRING value)
{
    value.Upper();
    char* const text = value.ToChar();
    const char* const dot = std::strrchr(text, '.');

    if (dot) {
        ext = zSTRING(dot + 1);
        filename = value;
        filename.Delete(dot - text, std::strlen(dot));
    }
    else {
        filename = value;
        ext.Clear();
    }

    SetCompletePath();
}

void zFILE_FILE::SetFilename(zSTRING value)
{
    NormalizeFilePart(value);
    filename = value;
    filename.Upper();
    SetCompletePath();
}

void zFILE_FILE::SetExt(zSTRING value)
{
    NormalizeFilePart(value);
    ext = value;
    ext.Upper();
    SetCompletePath();
}

void zFILE_FILE::SetCompletePath()
{
    complete_path.Clear();

    if (drive.Length()) {
        complete_path.Append(drive);
        complete_path.Append(zSTRING(":"));
    }

    if (dir.Length()) {
        if (dir[dir.Length() - 1] != '\\')
            dir.Append(zSTRING("\\"));

        complete_path.Append(dir);
    }

    complete_directory = complete_path;
    complete_path.Append(filename);

    if (ext.Length()) {
        complete_path.Append(zSTRING("."));
        complete_path.Append(ext);
    }
}

zSTRING zFILE_FILE::SetCurrentDir()
{
    s_virtPathString = complete_path;
    SetPath(s_virtPathString);
    return GetPath();
}

int zFILE_FILE::ChangeDir(bool useRoot)
{
    s_virtPathString = complete_directory;

    if (useRoot) {
        s_physPathString = s_rootDirString;
        s_physPathString.Append(complete_directory);
    }
    else {
        s_physPathString = complete_directory;
    }

    return 0;
}

zSTRING zFILE_FILE::DirStepInto(zSTRING value)
{
    if (value == zSTRING("..")) {
        char* const text = dir.ToChar();
        char* const slash = std::strrchr(text, '\\');

        if (slash) {
            dir.Delete(slash - text, dir.Length() - (slash - text));
        }
        else {
            dir.Clear();
        }
    }
    else {
        dir.Append(zSTRING("\\"));
        dir.Append(value);
    }

    SetCompletePath();
    return dir;
}

bool zFILE_FILE::DirCreate()
{
    zSTRING path = s_rootDirString;
    path.Append(GetDirectoryPath());
    path.Lower();
    return _mkdir(path.ToChar()) == 0;
}

bool zFILE_FILE::DirRemove()
{
    zSTRING path = s_rootDirString;
    path.Append(GetDirectoryPath());
    return _rmdir(path.ToChar()) == 0;
}

bool zFILE_FILE::DirExists()
{
    char current[0x104];
    _getcwd(current, sizeof(current));
    const bool result = _chdir(GetDirectoryPath().ToChar()) == 0;
    _chdir(current);
    return result;
}

bool zFILE_FILE::FileDelete()
{
    if (IsOpened())
        Close();

    return std::remove(GetFullPath().ToChar()) == 0;
}

bool zFILE_FILE::FileMove(zSTRING path, bool updatePath)
{
    zFILE_FILE other(path);
    const bool result = FileMove(&other);

    if (result && updatePath)
        SetPath(other.GetFullPath());

    return result;
}

bool zFILE_FILE::FileMove(zFILE* other)
{
    if (IsOpened())
        Close();

    zFILE_FILE* otherFile = static_cast<zFILE_FILE*>(other);

    if (otherFile->IsOpened())
        otherFile->Close();

    return std::rename(GetFullPath().ToChar(), otherFile->GetFullPath().ToChar()) == 0;
}

bool zFILE_FILE::FileCopy(zSTRING path, bool updatePath)
{
    zFILE_FILE other(path);
    const bool result = FileCopy(&other);

    if (result && updatePath)
        SetPath(other.GetFullPath());

    return result;
}

bool zFILE_FILE::FileCopy(zFILE* other)
{
    zFILE_FILE* otherFile = static_cast<zFILE_FILE*>(other);

    if (!file_handle) {
        file_handle = reinterpret_cast<zGOTHIC_FILE*>(std::fopen(GetFullPath().ToChar(), "rb"));
        if (!file_handle)
            return false;
    }

    FILE* destination = std::fopen(otherFile->GetFullPath().ToChar(), "wb");
    if (!destination)
        return false;

    char data[0x18000];
    size_t count;

    while ((count = std::fread(data, 1, sizeof(data), GetStdFile(this))) > 0)
        std::fwrite(data, 1, count, destination);

    std::fclose(destination);
    return true;
}

bool zFILE_FILE::FindFirst(const zSTRING& mask)
{
    if (find_handle)
        _findclose(find_handle);

    zSTRING pattern = s_physPathString;
    pattern.Append(mask);
    find_handle = _findfirst(pattern.ToChar(), reinterpret_cast<_finddata_t*>(&find_fileinfo));

    if (find_handle >= 0) {
        if (find_fileinfo.attrib & 0x10) {
            zSTRING path = find_fileinfo.name;
            path.Append(zSTRING("\\"));
            SetPath(path);
        }
        else {
            SetPath(zSTRING(find_fileinfo.name));
        }
    }

    return find_handle >= 0;
}

bool zFILE_FILE::FindNext()
{
    if (!find_handle)
        return false;

    if (_findnext(find_handle, reinterpret_cast<_finddata_t*>(&find_fileinfo))) {
        _findclose(find_handle);
        find_handle = 0;
        return false;
    }

    if (find_fileinfo.attrib & 0x10) {
        zSTRING path = find_fileinfo.name;
        path.Append(zSTRING("\\"));
        SetPath(path);
    }
    else {
        SetPath(zSTRING(find_fileinfo.name));
    }

    return true;
}

int zFILE_FILE::SearchFile(const zSTRING& filePath, const zSTRING& fileName, int)
{
    if (filePath.Length())
        SetPath(filePath);

    if (fileName.Length())
        SetFile(fileName);

    return Exists() ? 0 : 0x138B;
}

zCBuffer* zFILE::GetFreeBuffer()
{
    for (int i = 0; i < 6; ++i) {
        if (bufferFlag[i])
            continue;
        bufferFlag[i] = 1;
        if (!bufferList[i])
            bufferList[i] = new zCBuffer;
        bufferList[i]->Reset();
        bufferList[i]->Resize(0x19000);
        return bufferList[i];
    }
    zerr->Warning("B: DISK: Too many files (6) are open.");
    return NULL;
}

void zFILE::ReleaseBuffer(zCBuffer* buffer)
{
    if (!buffer)
        return;
    for (int i = 0; i < 6; ++i) {
        if (bufferList[i] == buffer) {
            buffer->Reset();
            bufferFlag[i] = 0;
        }
    }
}

bool zFILE::InitFileSystem()
{
    zerr->Message("B: zDSK: InitFileSystem");
    delete s_dirscanCiticalSection;
    s_dirscanCiticalSection = new zCCriticalSection;
    for (int i = 0; i < 6; ++i) {
        bufferList[i] = NULL;
        bufferFlag[i] = 0;
    }
    s_numFilesOpen = 0;
    s_maxFilesOpen = 0;
    s_initialized = true;
    return true;
}

bool zFILE::DeinitFileSystem()
{
    delete s_dirscanCiticalSection;
    s_dirscanCiticalSection = NULL;
    if (s_numFilesOpen > 0)
        zerr->Warning(zSTRING("B: zDSK: ") + zSTRING(s_numFilesOpen) + " files are still open!");
    for (int i = 0; i < 6; ++i) {
        delete bufferList[i];
        bufferList[i] = NULL;
        bufferFlag[i] = 0;
    }
    s_initialized = false;
    return true;
}

bool zFILE_FILE::InitFileSystem()
{
    if (!zFILE::s_virtPath)
        zFILE::s_virtPath = new zFILE_FILE;
    return zFILE::InitFileSystem();
}

bool zFILE_FILE::DeinitFileSystem()
{
    delete zFILE::s_virtPath;
    zFILE::s_virtPath = NULL;
    return zFILE::DeinitFileSystem();
}

void Dir_Clear(const zSTRING& directory, const zSTRING& mask)
{
    zFILE::DirectFileAccess(true);
    _mkdir(directory.ToChar());
    const zSTRING pattern = directory + mask;
    _finddata_t data;
    const long handle = _findfirst(pattern.ToChar(), &data);
    if (handle != -1) {
        do {
            if (data.attrib & 0x10)
                continue;
            const zSTRING path = directory + data.name;
            _chmod(path.ToChar(), 0x80);
            _remove(path.ToChar());
        } while (_findnext(handle, &data) == 0);
        _findclose(handle);
    }
    zFILE::DirectFileAccess(false);
}

void Dir_Copy(const zSTRING& source, const zSTRING& destination)
{
    Dir_Clear(destination, "\\*.*");
    zFILE::DirectFileAccess(true);
    _mkdir(destination.ToChar());
    zFILE::DirectFileAccess(false);
    const zSTRING pattern = source + "\\*.*";
    _finddata_t data;
    const long handle = _findfirst(pattern.ToChar(), &data);
    if (handle != -1) {
        do {
            if (data.attrib & 0x10)
                continue;
            zFILE_FILE sourceFile(source + "\\" + data.name);
            sourceFile.FileCopy(destination + "\\" + data.name, true);
        } while (_findnext(handle, &data) == 0);
        _findclose(handle);
    }
}
