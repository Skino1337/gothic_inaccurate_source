#include "include/zAccount.h"
#include "include/zSparseArray.h"
#include <cstring>

class zCOption {
public:

    int Parm(const zSTRING& name);
};

extern zCOption* zoptions;

static int accountInitialized;
static int accountEnabled;
static int accountFull;
static zCSparseArray<void*, zCAccountInfo> accountInfoTable(0x407);

void zAccountInit()
{
    if (!zoptions)
        return;
    accountInitialized = 1;
    accountEnabled = zoptions->Parm(zSTRING("ZACCOUNT"));
    accountFull = zoptions->Parm(zSTRING("ZACCOUNTFULL"));
    accountEnabled |= accountFull;
}

int Hash(const zSTRING& value)
{
    int result = 0;
    int multiplier = 1;
    const char* text = value.ToChar();
    for (int i = value.Length() - 1; i >= 0; --i) {
        result += text[i] * multiplier;
        multiplier *= 0x1d;
    }
    return result;
}

zTIndexA::~zTIndexA()
{
}

zTIndexB::~zTIndexB()
{
}

zCAccountInfo::~zCAccountInfo()
{
    for (int i = 0; i < files.numInArray; ++i)
        delete files.parray[i];
    for (int i = 0; i < lines.numInArray; ++i)
        delete lines.parray[i];
}

void zCAccountInfo::Add(const char* file, int line, int count)
{
    zTIndexA* fileEntry = 0;
    for (int i = 0; i < files.numInArray; ++i) {
        if (files.parray[i]->name == file) {
            fileEntry = files.parray[i];
            break;
        }
    }
    if (!fileEntry) {
        fileEntry = new zTIndexA;
        fileEntry->name = file;
        fileEntry->count = 0;
        files.InsertEnd(fileEntry);
    }
    zSTRING lineName(line);
    zTIndexB* lineEntry = 0;
    for (int i = 0; i < lines.numInArray; ++i) {
        if (lines.parray[i]->name == lineName) {
            lineEntry = lines.parray[i];
            break;
        }
    }
    if (!lineEntry) {
        lineEntry = new zTIndexB;
        lineEntry->name = lineName;
        lineEntry->count = 0;
        lines.InsertEnd(lineEntry);
    }
    fileEntry->count += count;
    lineEntry->count += count;
    total += count;
}

void zCAccountInfo::SumUp()
{
    for (int i = 0; i < files.numInArray; ++i) {
        zTIndexA* file = files.parray[i];
        if (!accountFull && !file->count)
            continue;
        for (int j = 0; j < lines.numInArray; ++j) {
            zTIndexB* line = lines.parray[j];
            zSTRING text = zSTRING("M: file: ") + file->name + zSTRING(", line: ")
                + line->name + zSTRING(", count: ") + zSTRING(line->count) + zSTRING("\n");
            zerr->Message(text);
        }
    }
}

struct zTAccountRecord {
    const void* owner;
    zCAccountInfo* info;
};

static zCArray<zTAccountRecord> accountRecords;
int zCAccount::s_actCounter;

static zCAccountInfo* FindAccountInfo(const void* owner, int create)
{
    for (int i = 0; i < accountRecords.numInArray; ++i) {
        if (accountRecords.parray[i].owner == owner)
            return accountRecords.parray[i].info;
    }
    if (!create)
        return 0;
    zTAccountRecord record;
    record.owner = owner;
    record.info = new zCAccountInfo;
    record.info->total = 0;
    accountRecords.InsertEnd(record);
    return record.info;
}

void zCAccount::Add(const void* owner, const char* file, int line, int count)
{
    if (s_actCounter <= 100 && accountEnabled) {
        if (!accountInitialized)
            zAccountInit();
        const char* fileName = file;
        const char* current = file;
        while (*current) {
            if (*current == '\\')
                fileName = current + 1;
            ++current;
        }
        FindAccountInfo(owner, 1)->Add(fileName, line, count);
    }
    --s_actCounter;
}

void zCAccount::Sum(const void* owner)
{
    if (!accountEnabled)
        return;
    if (!accountInitialized)
        zAccountInit();
    FindAccountInfo(owner, 1)->SumUp();
}

void zCAccount::Clear(const void* owner)
{
    if (!accountEnabled || !accountInitialized)
        return;
    for (int i = 0; i < accountRecords.numInArray; ++i) {
        if (accountRecords.parray[i].owner == owner) {
            delete accountRecords.parray[i].info;
            accountRecords.parray[i] = accountRecords.parray[accountRecords.numInArray - 1];
            --accountRecords.numInArray;
            return;
        }
    }
}

void zCAccount::Begin()
{
    ++s_actCounter;
}

void zCAccount::End()
{
    --s_actCounter;
}

int zCAccount::IsActive()
{
    return s_actCounter;
}
