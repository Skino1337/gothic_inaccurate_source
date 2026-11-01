#ifndef ZCSLIB_H
#define ZCSLIB_H

#include "zVob.h"

class zCCSLib : public zCObject {
public:

    zCCSLib();
    virtual ~zCCSLib();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void DeleteLib();
    void RemoveFromLib(int index, int removeIndex);
    int ValidateToken(zSTRING& token);
    int IsLoaded();
    int WasChanged();
    int NextFree();
    int Add(zCCSBlock* block);
    void Change();
    zCCSBlock* GetOU(int index);
    void CompactLib();

    int loaded;
    int wasChanged;
    zCArraySort<zCCSBlock*> ouList;
    zCCSBlock* searchBlock;

    static zCClassDef* classDef;
};

#endif
