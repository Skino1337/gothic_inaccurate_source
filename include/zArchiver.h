#ifndef ZARCHIVER_H
#define ZARCHIVER_H

#include "zBuffer.h"
#include "zFile.h"

struct zCOLOR;

enum zTArchiveMode {
    zARC_MODE_BINARY,
    zARC_MODE_ASCII,
    zARC_MODE_ASCII_PROPS,
    zARC_MODE_BINARY_SAFE,
    zARC_MODE_COUNT
};

enum zTArchiveFlags {
    zARC_FLAG_WRITE_BRIEF_HEADER = 1,
    zARC_FLAG_NO_SPY_MESSAGES = 2
};

enum zTArchiveTypeID {
    zARC2_ID_STRING = 1,
    zARC2_ID_INTEGER,
    zARC2_ID_FLOAT,
    zARC2_ID_BYTE = 4,
    zARC2_ID_WORD,
    zARC2_ID_INT,
    zARC2_ID_VEC3,
    zARC2_ID_COLOR,
    zARC2_ID_RAW,
    zARC2_ID_RAWFLOAT = 16,
    zARC2_ID_ENUM,
    zARC2_ID_HASH
};

const int READ_BUFFER_SIZE = 1024 * 8;

template <class K, class T>
class zCSparseArray;

class zCArchiverGeneric : public zCArchiver {
public:

    struct zTWriteObjectEntry {
        zCObject* object;
        int       objectIndex;
    };

    zCArchiverGeneric();
    virtual ~zCArchiverGeneric();
    static zCObject* _CreateNewInstance();
    static void TestClassIntegrity(void (*callback)(zCObject*, int, zCClassDef*, int&));
    virtual zCClassDef* _GetClassDef() const;
    static void operator delete(void* memory);
    void          DebugMessage(const zSTRING& message);
    void          WriteHeaderNumObj();
    void          Close();
    int           EndOfArchive();
    void          DeleteBuffer();
    void          GetBufferString(zSTRING& value);
    zCBuffer*     GetBuffer();
    int           OpenWriteBuffer(zCBuffer* buffer, zTArchiveMode mode, int saveGame,
        int flags, int owningMedium);
    void          OpenWriteFile(zFILE* file, zTArchiveMode mode, int saveGame,
        int flags, int owningMedium);
    int           OpenReadBuffer(zCBuffer& buffer, zTArchiveMode mode, int saveGame, int flags);
    void          OpenReadFile(zFILE* file, zTArchiveMode mode, int saveGame,
        int flags, int owningMedium);
    void          ReadHeader();
    void          CheckObjectListSize(int size);
    zCObject*     CreateObject(const zSTRING& className);
    zCClassDef*   GetClassDefByString(const zSTRING& className);
    void          RestoreBuffer(void* data, unsigned long size);
    void          RestoreStringEOL(zSTRING& value);
    void          RestoreString0(zSTRING& value);
    unsigned long RestoreGetPos();
    void          RestoreSeek(unsigned long position);
    void          WriteHeader(int saveGame);
    int           IsStringValid(const char* text);
    void          WriteASCIILine(const char* name, const char* type, const zSTRING& value);
    void          WriteInt(const char* name, int value);
    void          WriteByte(const char* name, unsigned char value);
    void          WriteWord(const char* name, unsigned short value);
    void          WriteFloat(const char* name, float value);
    void          WriteBool(const char* name, int value);
    void          WriteString(const char* name, const zSTRING& value);
    void          WriteVec3(const char* name, const zVEC3& value);
    void          WriteColor(const char* name, const zCOLOR& value);
    void          WriteRaw(const char* name, void* data, unsigned long size);
    void          WriteRawFloat(const char* name, void* data, unsigned long size);
    void          WriteEnum(const char* name, const char* enumName, int value);
    void          WriteChunkStart(const char* name, const char* className,
        unsigned short version, unsigned long objectIndex);
    void          WriteChunkStart(const char* name, unsigned short version);
    void          WriteChunkEnd();
    void          WriteObject(const char* name, zCObject& object);
    void          WriteObject(const char* name, zCObject* object);
    void          WriteObject(zCObject* object);
    zCObject*     ReadObject(const char* name, zCObject* object);
    zCObject*     ReadObject(zCObject* object);
    int           ReadEntryASCII(const char* name, zSTRING& value);
    float         ReadFloat(const char* name);
    void          ReadFloat(const char* name, float& value);
    zVEC3         ReadVec3(const char* name);
    void          ReadVec3(const char* name, zVEC3& value);
    int           ReadInt(const char* name);
    void          ReadInt(const char* name, int& value);
    unsigned char ReadByte(const char* name);
    void          ReadByte(const char* name, unsigned char& value);
    unsigned short ReadWord(const char* name);
    void          ReadWord(const char* name, unsigned short& value);
    int           ReadBool(const char* name);
    void          ReadBool(const char* name, int& value);
    zSTRING       ReadString(const char* name);
    void          ReadString(const char* name, zSTRING& value);
    zCOLOR        ReadColor(const char* name);
    void          ReadColor(const char* name, zCOLOR& value);
    int           ReadEnum(const char* name);
    void          ReadEnum(const char* name, int& value);
    void          ReadRaw(const char* name, void* data, unsigned long size);
    void          ReadRawFloat(const char* name, void* data, unsigned long size);
    void          ReadChunkStartASCII(const char* name, zSTRING& line);
    int           ReadChunkStart(const char* name);
    int           ReadChunkStart(zSTRING& name, unsigned short& version);
    int           ReadChunkStartNamed(const char* name, unsigned short& version);
    unsigned short GetCurrentChunkVersion();
    void          ReadChunkEnd();
    void          WriteGroupBegin(const char* name);
    void          WriteGroupEnd(const char* name);
    void          StoreBuffer(void* data, unsigned long size);
    unsigned long StoreGetPos();
    void          StoreSeek(unsigned long position);
    void          StoreString(const char* text);
    void          StoreStringEOL(const char* text);
    void          SkipChunk(int onlyCurrent);
    void          SkipOpenChunk();
    zFILE*        GetFile() const;
    void          SetStringEOL(const zSTRING& value);
    zSTRING       GetStringEOL() const;
    int           GetChecksumEnabled() const;
    void          SetChecksumEnabled(int enabled);
    void          SetNoReadSearchCycles(int cycles);
    int           InProperties() const;
    int           InSaveGame() const;
    int           InBinaryMode() const;
    zCObject*     GetParentObject();

    zFILE* file;
    zCBuffer* cbuffer;
    int owningMedium;
    int inSaveGame;
    int binaryMode;
    int accessMode;
    zCArray<zTChunkRecord> chunkStack;
    zCArray<zCObject*> objectList;
    unsigned long posNumObjects;
    unsigned long posChecksum;
    unsigned long posHeaderEnd;
    int checksumEnabled;
    int flags;
    int debugMessagesOn;
    int noReadSearchCycles;
    zSTRING tmpResultValue;
    int deleteFileOnClose;
    zCSparseArray<zCObject*, zTWriteObjectEntry> writeObjectList;
    int warnEntrysNotRead;
    int warnEntryNotFound;
    int warnWrongEntryOrder;

    static zCClassDef* classDef;
};

class GECString {
public:

    ~GECString();
    void Free();
    void Copy(const char* value);

    char* string;
};

struct zCArchiverChunkEntry {
    ~zCArchiverChunkEntry();

    zCArchiverChunkEntry* next;
    unsigned long         hash;
    unsigned long         value;
    GECString*            key;
    unsigned long*        data;
};

template <class T>
struct GETStringMap;

template <>
struct GETStringMap<unsigned long> {
    typedef zCArchiverChunkEntry GESNextEntry;

    unsigned long SetAt(const char* name, unsigned long* data);
    unsigned long SetAtForce(const char* name, unsigned long* data,
        unsigned long hash, unsigned long value);
    void          RemoveAll(bool deleteData);
    unsigned long Lookup(const char* name, unsigned long*& data,
        unsigned long value = 0xFFFFFFFF) const;
    bool          GetNextElement(unsigned long& hash, GECString& key,
        unsigned long*& data, unsigned long& value);
    GESNextEntry* GetElement(const char* name, unsigned long& bucket,
        unsigned long value = 0xFFFFFFFF) const;

    void*         reserved;
    GESNextEntry** table;
    unsigned long tableSize;
    long          count;
};

typedef GETStringMap<unsigned long> zCArchiverChunkMap;

class zCArchiverBinSafe : public zCArchiver {
public:

    zCArchiverBinSafe();
    virtual ~zCArchiverBinSafe();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    static void operator delete(void* memory);
    void          RestoreString(zSTRING& value);
    void          DebugMessage(const zSTRING& message);
    void          WriteHeaderNumObj();
    void          Close();
    int           EndOfArchive();
    void          DeleteBuffer();
    void          GetBufferString(zSTRING& value);
    zCBuffer*     GetBuffer();
    int           OpenWriteBuffer(zCBuffer* buffer, zTArchiveMode mode, int saveGame,
        int flags, int owningMedium);
    void          OpenWriteFile(zFILE* file, zTArchiveMode mode, int saveGame,
        int flags, int owningMedium);
    int           OpenReadBuffer(zCBuffer& buffer, zTArchiveMode mode, int saveGame, int flags);
    void          OpenReadFile(zFILE* file, zTArchiveMode mode, int saveGame,
        int flags, int owningMedium);
    void          ReadHeader();
    void          CheckObjectListSize(int size);
    zCObject*     CreateObject(const zSTRING& className);
    zCClassDef*   GetClassDefByString(const zSTRING& className);
    void          RestoreStringEOL(zSTRING& value);
    void          RestoreString0(zSTRING& value);
    unsigned long RestoreGetPos();
    void          RestoreSeek(unsigned long position);
    void          WriteHeader(int saveGame);
    void          StoreString(const char* text);
    void          StoreStringEOL(const char* text);
    unsigned long StoreGetPos();
    void          StoreSeek(unsigned long position);
    void          WriteType(zTArchiveTypeID type, void* data, unsigned long size);
    int           ReadType(zTArchiveTypeID type, void* data, unsigned long size);
    void          ClearChunkList();
    unsigned long InsertChunkInList(const char* name);
    unsigned long ResolveAndPosEntry(const char* name);
    void          StoreBuffer(void* data, unsigned long size);
    void          RestoreBuffer(void* data, unsigned long size);
    int           IsStringValid(const char* text);
    void          WriteASCIILine(const char* name, const char* type, const zSTRING& value);
    void          WriteInt(const char* name, int value);
    void          WriteByte(const char* name, unsigned char value);
    void          WriteWord(const char* name, unsigned short value);
    void          WriteFloat(const char* name, float value);
    void          WriteBool(const char* name, int value);
    void          WriteString(const char* name, const zSTRING& value);
    void          WriteVec3(const char* name, const zVEC3& value);
    void          WriteColor(const char* name, const zCOLOR& value);
    void          WriteRaw(const char* name, void* data, unsigned long size);
    void          WriteRawFloat(const char* name, void* data, unsigned long size);
    void          WriteEnum(const char* name, const char* enumName, int value);
    void          WriteGroupBegin(const char* name);
    void          WriteGroupEnd(const char* name);
    void          WriteChunkStart(const char* name, const char* className,
        unsigned short version, unsigned long objectIndex);
    void          WriteChunkStart(const char* name, unsigned short version);
    void          WriteChunkEnd();
    void          WriteObject(const char* name, zCObject& object);
    void          WriteObject(const char* name, zCObject* object);
    void          WriteObject(zCObject* object);
    zCObject*     ReadObject(const char* name, zCObject* object);
    zCObject*     ReadObject(zCObject* object);
    int           ReadEntryASCII(const char* name, zSTRING& value);
    float         ReadFloat(const char* name);
    void          ReadFloat(const char* name, float& value);
    zVEC3         ReadVec3(const char* name);
    void          ReadVec3(const char* name, zVEC3& value);
    int           ReadInt(const char* name);
    void          ReadInt(const char* name, int& value);
    unsigned char ReadByte(const char* name);
    void          ReadByte(const char* name, unsigned char& value);
    unsigned short ReadWord(const char* name);
    void          ReadWord(const char* name, unsigned short& value);
    int           ReadBool(const char* name);
    void          ReadBool(const char* name, int& value);
    zSTRING       ReadString(const char* name);
    void          ReadString(const char* name, zSTRING& value);
    zCOLOR        ReadColor(const char* name);
    void          ReadColor(const char* name, zCOLOR& value);
    int           ReadEnum(const char* name);
    void          ReadEnum(const char* name, int& value);
    void          ReadRaw(const char* name, void* data, unsigned long size);
    void          ReadRawFloat(const char* name, void* data, unsigned long size);
    void          ReadChunkStartASCII(const char* name, zSTRING& line);
    int           ReadChunkStart(const char* name);
    int           ReadChunkStart(zSTRING& name, unsigned short& version);
    int           ReadChunkStartNamed(const char* name, unsigned short& version);
    unsigned short GetCurrentChunkVersion();
    void          ReadChunkEnd();
    void          SkipChunk(int onlyCurrent);
    void          SkipOpenChunk();
    zFILE*        GetFile() const;
    void          SetStringEOL(const zSTRING& value);
    zSTRING       GetStringEOL() const;
    int           GetChecksumEnabled() const;
    void          SetChecksumEnabled(int enabled);
    void          SetNoReadSearchCycles(int cycles);
    int           InProperties() const;
    int           InSaveGame() const;
    int           InBinaryMode() const;
    zCObject*     GetParentObject();

    zFILE*    file;
    zCBuffer* cbuffer;
    int       owningMedium;
    int       inSaveGame;
    int       accessMode;
    zCArray<zCArchiver::zTChunkRecord> chunkStack;
    zCArray<zCObject*> objectList;
    unsigned long posNumObjects;
    unsigned long posChecksum;
    unsigned long posHeaderEnd;
    int checksumEnabled;
    int flags;
    int debugMessagesOn;
    int noReadSearchCycles;
    zSTRING tmpResultValue;
    int deleteFileOnClose;
    int warnEntrysNotRead;
    int warnEntryNotFound;
    int warnWrongEntryOrder;
    void* stringHashMap;
    unsigned long dummyVal;
    unsigned long posChunkList;
    char m_readTextBuffer[READ_BUFFER_SIZE];

    static zCClassDef* classDef;
};

class zCArchiverFactory {
public:

    int  EndOfArchive(zCBuffer* buffer, zFILE* file);
    void ReadLine(zSTRING& line, zCBuffer* buffer, zFILE* file);
    void ReadLineArg(zSTRING& key, zSTRING& value, zCBuffer* buffer, zFILE* file);
    void ReadHeader(int saveGame, zCBuffer* buffer, zFILE* file,
        zCArchiver*& archiver, zTArchiveMode& mode, int& flags);
    void WriteLine(const zSTRING& line, zCBuffer* buffer, zFILE* file);
    void WriteLine(const char* line, zCBuffer* buffer, zFILE* file);
    void WriteHeader(zCArchiver* archiver, zTArchiveMode mode, int flags, int saveGame,
        zCBuffer* buffer, zFILE* file);
    zCArchiver* CreateArchiverFromMode(zTArchiveMode mode);
    virtual zCArchiver* CreateArchiverRead(zCBuffer* buffer, int saveGame);
    virtual zCArchiver* CreateArchiverRead(zFILE* file, int saveGame);
    virtual zCArchiver* CreateArchiverRead(const zSTRING& fileName, int saveGame);
    virtual zCArchiver* CreateArchiverWrite(zCBuffer* buffer, zTArchiveMode mode,
        int flags, int saveGame);
    virtual zCArchiver* CreateArchiverWrite(zTArchiveMode mode, int flags, int saveGame);
    virtual zCArchiver* CreateArchiverWrite(zFILE* file, zTArchiveMode mode,
        int flags, int saveGame);
    virtual zCArchiver* CreateArchiverWrite(const zSTRING& fileName, zTArchiveMode mode,
        int flags, int saveGame);
};

#endif
