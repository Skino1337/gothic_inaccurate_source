#include "include/zCsLib.h"

void sysEvent();

static int zCCSLibClassDefState = -1;
static zCClassDef zCCSLibClassDef("zCCSLib", "zCObject",
    zCCSLib::_CreateNewInstance, 0, 0x40, 0, "");
zCClassDef* zCCSLib::classDef = &zCCSLibClassDef;

zCCSLib::zCCSLib()
    : loaded(0), wasChanged(0), searchBlock(NULL)
{
    searchBlock = new zCCSBlock;
    zCClassDef::ObjectCreated(searchBlock, zCCSBlock::classDef);
}

zCCSLib::~zCCSLib()
{
    if (searchBlock) {
        searchBlock->Release();
        searchBlock = NULL;
    }
    DeleteLib();
}

zCObject* zCCSLib::_CreateNewInstance()
{
    zCCSLib* library = new zCCSLib;
    zCClassDef::ObjectCreated(library, classDef);
    return library;
}

void zCCSLib::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSLib::_GetClassDef() const
{
    return classDef;
}

void zCCSLib::Archive(zCArchiver& archiver)
{
    int count = 0;
    for (int i = 0; i < ouList.numInArray; ++i) {
        if (ouList.parray[i])
            ++count;
    }
    archiver.WriteInt(count);
    for (int i = 0; i < ouList.numInArray; ++i) {
        if (ouList.parray[i])
            archiver.WriteObject(ouList.parray[i]);
    }
}

void zCCSLib::Unarchive(zCArchiver& archiver)
{
    if (zerr->filter_level >= 3)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CSLib: loading library ...", 3, 1, 0x21, 0, 0);
    const int count = archiver.ReadInt();
    if (zerr->filter_level >= 3)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CSLib: Number of items: " + zSTRING(count)
            + " (UnArc)", 3, 0, 0x27, 0, 0);
    if (ouList.numAlloc < 15000)
        ouList.AllocDelta(15000 - ouList.numAlloc);
    for (int i = 0; i < count; ++i) {
        zCCSBlock* block = dynamic_cast<zCCSBlock*>(archiver.ReadObject(NULL));
        if (block)
            ouList.InsertEnd(block);
        if (!(i % 250))
            sysEvent();
    }
    if (zerr->filter_level >= 3)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CSLib: Sorting library ...", 3, 0, 0x39, 0, 0);
    ouList.QuickSort();
    loaded = 1;
    wasChanged = 0;
    if (zerr->filter_level >= 3)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CSLib: finished loading library", 3, 2, 0x40, 0, 0);
}

void zCCSLib::DeleteLib()
{
    for (int i = 0; i < ouList.numInArray; ++i) {
        if (ouList.parray[i]) {
            ouList.parray[i]->Release();
            ouList.parray[i] = NULL;
        }
    }
    ouList.DeleteList();
    loaded = 0;
    wasChanged = 0;
}

void zCCSLib::RemoveFromLib(int index, int removeIndex)
{
    if (ouList.parray[index]) {
        ouList.parray[index]->Release();
        ouList.parray[index] = NULL;
    }
    if (removeIndex) {
        for (int i = index; i < ouList.numInArray - 1; ++i)
            ouList.parray[i] = ouList.parray[i + 1];
        if (index < ouList.numInArray)
            --ouList.numInArray;
    }
}

int zCCSLib::ValidateToken(zSTRING& token)
{
    searchBlock->SetRoleName(token);
    return ouList.Search(searchBlock);
}

int zCCSLib::IsLoaded()
{
    return loaded;
}

int zCCSLib::WasChanged()
{
    return wasChanged;
}

int zCCSLib::NextFree()
{
    return ouList.numInArray;
}

int zCCSLib::Add(zCCSBlock* block)
{
    if (!loaded)
        return -1;
    if (block) {
        ouList.InsertEnd(block);
        block->AddRef();
    }
    wasChanged = 1;
    return ouList.numInArray - 1;
}

void zCCSLib::Change()
{
    wasChanged = 1;
}

zCCSBlock* zCCSLib::GetOU(int index)
{
    return ouList.parray[index];
}

void zCCSLib::CompactLib()
{
    for (int i = 0; i < ouList.numInArray;) {
        if (ouList.parray[i]) {
            ++i;
            continue;
        }
        for (int j = i; j < ouList.numInArray - 1; ++j)
            ouList.parray[j] = ouList.parray[j + 1];
        --ouList.numInArray;
    }
}
