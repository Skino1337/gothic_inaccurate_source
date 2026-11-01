#ifndef ZFILE_H
#define ZFILE_H

#include "zString.h"
#include "zBuffer.h"

struct zGOTHIC_FILE {
    char* _ptr;
    int   _cnt;
    char* _base;
    int   _flag;
    int   _file;
    int   _charbuf;
    int   _bufsiz;
    char* _tmpfname;
};

struct _finddata32i64_t_vc6 {
    unsigned int attrib;
    long long    time_create;
    long long    time_access;
    long long    time_write;
    unsigned long size;
    char          name[260];
};

enum zTDATE_CONVERT {
    zDATE_ALL,
    zDATE_DATE,
    zDATE_TIME
};

struct zDATE {
    zDATE& operator=(long value);
    long ToTime_t();
    zSTRING ToString(zTDATE_CONVERT conversion);

    unsigned int   year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
};

int operator==(const zDATE& first, const zDATE& second);
int operator<(const zDATE& first, const zDATE& second);

struct zFILE_STATS {
    zDATE         accessDate;
    zDATE         createDate;
    zDATE         modifyDate;
    unsigned long size;
};

class zCCriticalSection;

class zFILE {
public:

    static zSTRING s_virtPathString;
    static zSTRING s_physPathString;
    static zSTRING s_rootDirString;
    static zCCriticalSection* s_dirscanCiticalSection;
    static zFILE* s_virtPath;
    static zCBuffer* bufferList[6];
    static int bufferFlag[6];
    static int s_numFilesOpen;
    static int s_maxFilesOpen;
    static bool s_initialized;
    static zCBuffer* GetFreeBuffer();
    static void ReleaseBuffer(zCBuffer* buffer);
    static bool InitFileSystem();
    static bool DeinitFileSystem();
    static zSTRING SetRootDirectory(zSTRING directory);
    static void    DirectFileConvert(zSTRING& fileName);
    static void    DirectFileAccess(bool direct);
    static void    DirectFileAccess(bool direct, zSTRING& fileName);
    static int     LockDirScan();
    static int     UnlockDirScan();

    zFILE() : write(false), buffering(0) {}
    zFILE(const zSTRING&) : write(false), buffering(0) {}
    virtual ~zFILE() {}

    bool write;
    int buffering;

    virtual long GetFreeDiskSpace();
    virtual int  Create();
    virtual int  Open(int mode);
    virtual int  Exists();
    virtual int  Append();
    virtual int  Close();
    virtual long Pos();
    virtual long Size();
    virtual int  Seek(long position);
    virtual void Reset();
    virtual void GetStats(zFILE_STATS& stats);
    virtual int  Write(const zSTRING& text);
    virtual long Write(const void* data, long size);
    virtual long    Read(void* data, long size);
    virtual int     Read(zSTRING& text);
    virtual bool    Eof();
    virtual zSTRING GetFilename();
};

class zFILE_FILE : public zFILE {
public:

    static bool InitFileSystem();
    static bool DeinitFileSystem();

    zFILE_FILE();
    zFILE_FILE(const zSTRING& path);
    virtual ~zFILE_FILE();

    void         Init(const zSTRING& path);
    int          HandleError(const zSTRING& operation, const zSTRING& path, zERROR_TYPE type);
    bool         IsOpened();
    void         SetMode(long mode);
    long         GetMode();
    zGOTHIC_FILE* GetFileHandle();
    int          Close();
    int          Reset();
    long         Size();
    long         Pos();
    int          Seek(long position);
    bool         Eof();
    long         FlushBuffer();
    void         SetPath(zSTRING path);
    zSTRING      GetFullPath();
    zSTRING&     SlashConvert(zSTRING& path);
    bool         Exists();
    bool         Exists(const zSTRING& path);
    int          Open(bool write);
    int          Open(const zSTRING& path, bool write);
    int          Create();
    int          Create(const zSTRING& path);
    int          Write(const char* text);
    int          Write(const zSTRING& text);
    long         Write(const void* data, long size);
    int          ReadChar(char& value);
    int          Read(char* text);
    int          Read(zSTRING& text);
    long         Read(void* data, long size);
    zSTRING      SeekText(const zSTRING& text);
    zSTRING      ReadBlock(long start, long numLines);
    int          UpdateBlock(const zSTRING& text, long start, long numLines);
    int          ReadCodeline(zSTRING& text);
    int          ReadLines(zSTRING& text, long numLines);
    void         Append();
    int          GetStats(zFILE_STATS& stats);
    zSTRING      GetPath();
    zSTRING      GetDirectoryPath();
    zSTRING      GetDrive();
    zSTRING      GetDir();
    zSTRING      GetFile();
    zSTRING      GetFilename();
    zSTRING      GetExt();
    void         SetCompletePath();
    void         SetDrive(zSTRING value);
    void         SetDir(zSTRING value);
    void         SetFile(zSTRING value);
    void         SetFilename(zSTRING value);
    void         SetExt(zSTRING value);
    zSTRING      SetCurrentDir();
    int          ChangeDir(bool useRoot);
    zSTRING      DirStepInto(zSTRING value);
    bool         DirCreate();
    bool         DirRemove();
    bool         DirExists();
    bool         FileDelete();
    bool         FileMove(zSTRING path, bool updatePath);
    bool         FileMove(zFILE* other);
    bool         FileCopy(zSTRING path, bool updatePath);
    bool         FileCopy(zFILE* other);
    bool         FindFirst(const zSTRING& mask);
    bool         FindNext();
    int          SearchFile(const zSTRING& filePath, const zSTRING& fileName, int recursive);

    zCBuffer*     buffer;
    bool          write;
    bool          buffering;
    zSTRING       dir;
    zSTRING       drive;
    zSTRING       filename;
    zSTRING       ext;
    zSTRING       complete_path;
    zSTRING       complete_directory;
    int           error;
    zGOTHIC_FILE* file_handle;
    long          file_mode;
    _finddata32i64_t_vc6 find_fileinfo;
    long          find_handle;
};

#endif
