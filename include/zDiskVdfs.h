#ifndef ZDISKVDFS_H
#define ZDISKVDFS_H

#include "zFile.h"

struct TVDFFINDDATA {
    char name[64];
    long size;
    long attr;
    long datetime;
};

class zFILE_VDFS : public zFILE_FILE {
public:

    zFILE_VDFS();
    zFILE_VDFS(const zSTRING& path);
    virtual ~zFILE_VDFS();
    void Init(const zSTRING& path);
    int HandleError(const zSTRING& operation, const zSTRING& path, zERROR_TYPE type);
    virtual bool IsOpened();
    virtual int Open(bool write);
    virtual int Open(const zSTRING& path, bool write);
    virtual bool Exists();
    virtual bool Exists(const zSTRING& path);
    virtual int Close();
    virtual int Reset();
    virtual long Size();
    virtual bool Eof();
    virtual int Seek(long position);
    virtual long Pos();
    virtual void Append();
    virtual zSTRING SetCurrentDir();
    virtual zSTRING GetFullPath();
    virtual int ChangeDir(bool useRoot);
    virtual zSTRING SeekText(const zSTRING& text);
    virtual int GetStats(zFILE_STATS& stats);
    virtual int SearchFile(const zSTRING& filePath, const zSTRING& fileName, int recursive);
    virtual bool FindFirst(const zSTRING& mask);
    virtual bool FindNext();
    virtual int Read(char* text);
    virtual int Read(zSTRING& text);
    virtual long Read(void* data, long size);
    static bool InitFileSystem();
    static bool DeinitFileSystem();
    static bool LockCriticalSection();
    static bool UnlockCriticalSection();

    TVDFFINDDATA find_filedata;
    zSTRING vdfHandle;
    zSTRING vdfResult;
    bool vdfEOF;
    long vdfFindHandle;
    zSTRING vdfFindExt;

    static bool s_vdfsInitialized;
    static bool s_physical_first;
    static zCCriticalSection* s_criticalSection;
};

#endif
