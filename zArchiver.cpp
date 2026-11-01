#include "include/zArchiver.h"
#include "include/zAccount.h"
#include "include/zError.h"
#include "include/zModel.h"
#include "include/zSky.h"
#include <cstring>

static int archiverState = -1;
static zSTRING archiverSignature = "ZenGin Archive";
static zSTRING archiverFormatBinary = "BINARY";
static zSTRING archiverFormatAscii = "ASCII";
static zSTRING archiverFormatAsciiProps = "ASCII_PROPS";
static zSTRING archiverFormatBinSafe = "BIN_SAFE";
static zSTRING archiverBracketOpen = " [";
static zSTRING archiverBracketClose = " ]";
static int archiverBinSafeState = -1;
static int archiverGenericState = -1;
zCArchiverFactory zarcFactory;
static zCClassDef zCArchiverGenericClassDef("zCArchiverGeneric", "zCArchiver", NULL, 0, 0xBC, 0, "");
zCClassDef* zCArchiverGeneric::classDef = &zCArchiverGenericClassDef;
static zCClassDef zCArchiverBinSafeClassDef("zCArchiverBinSafe", "zCArchiver", NULL, 0, 0x20A4, 0, "");
zCClassDef* zCArchiverBinSafe::classDef = &zCArchiverBinSafeClassDef;

int zCArchiverFactory::EndOfArchive(zCBuffer* buffer, zFILE* file)
{
    if (!file)
        return buffer->GetPos() >= buffer->GetSize();
    return file->Eof() || file->Pos() + 3 > file->Size();
}

void zCArchiverFactory::ReadLine(zSTRING& line, zCBuffer* buffer, zFILE* file)
{
    char text[1024];
    int length = 0;
    char value = 0;

    while (length < 1023) {
        if (buffer)
            buffer->Read(&value, 1);
        else
            file->Read(&value, 1);
        if (value == '\n')
            break;
        text[length++] = value;
    }
    if (length == 1023)
        zerr->Warning("D: zCArchiverFactory: corrupt archive header, line too long");
    text[length] = 0;
    line = text;
}

void zCArchiverFactory::ReadLineArg(zSTRING& key, zSTRING& value,
    zCBuffer* buffer, zFILE* file)
{
    zSTRING line;
    ReadLine(line, buffer, file);
    key = line.PickWord(1, " ", "");
    value = line.PickWord(2, " ", "");
}

void zCArchiverFactory::ReadHeader(int saveGame, zCBuffer* buffer, zFILE* file,
    zCArchiver*& archiver, zTArchiveMode& mode, int& flags)
{
    archiver = NULL;
    mode = zARC_MODE_BINARY;
    flags = 0;

    zSTRING line;
    ReadLine(line, buffer, file);
    if (line != "ZENGIN ARCHIVE") {
        zerr->Warning("D: zCArchiverFactory: file ist not a ZenGin Archive");
        return;
    }

    zSTRING key;
    zSTRING value;
    ReadLineArg(key, value, buffer, file);
    if (key == "ver")
        archiver = CreateArchiverFromMode(value.ToInt() > 0
            ? zARC_MODE_BINARY_SAFE : zARC_MODE_ASCII);
    if (!archiver) {
        zerr->Warning("D: zCArchiverFactory: could not create specified archiver");
        return;
    }

    while (!EndOfArchive(buffer, file)) {
        const long position = buffer ? buffer->GetPos() : file->Pos();
        ReadLineArg(key, value, buffer, file);
        if (key == "arcMode") {
            mode = static_cast<zTArchiveMode>(value.ToInt());
            if (mode < 0 || mode >= zARC_MODE_COUNT) {
                zerr->Warning("D: zCArchiverFactory: unknown arcMode");
                archiver = NULL;
                return;
            }
        }
        else if (key == "saveGame") {
            flags = value.ToInt();
        }
        else if (key == "objects") {
            const unsigned long objectPosition = value.ToInt();
            if (buffer)
                buffer->SetPos(objectPosition);
            else
                file->Seek(objectPosition);
            break;
        }
        else if (key == "END") {
            if (buffer)
                buffer->SetPos(position);
            else
                file->Seek(position);
            break;
        }
    }
}

void zCArchiverFactory::WriteLine(const zSTRING& line, zCBuffer* buffer, zFILE* file)
{
    if (buffer) {
        buffer->Write(line.ToChar(), line.Length());
        buffer->WriteByte('\n', 8);
        return;
    }

    file->Write(line.ToChar(), line.Length());
    file->Write("\n", 1);
}

void zCArchiverFactory::WriteLine(const char* line, zCBuffer* buffer, zFILE* file)
{
    WriteLine(zSTRING(line), buffer, file);
}

void zCArchiverFactory::WriteHeader(zCArchiver* archiver, zTArchiveMode mode,
    int flags, int saveGame, zCBuffer* buffer, zFILE* file)
{
    WriteLine("ZENGIN ARCHIVE", buffer, file);
    WriteLine("ver 1", buffer, file);
    WriteLine("arcMode " + zSTRING(int(mode)), buffer, file);
    WriteLine("saveGame " + zSTRING(saveGame), buffer, file);
    WriteLine("END", buffer, file);
}

zCArchiver* zCArchiverFactory::CreateArchiverFromMode(zTArchiveMode mode)
{
    if (mode == zARC_MODE_BINARY_SAFE)
        return new zCArchiverBinSafe;
    return new zCArchiverGeneric;
}

zCArchiver* zCArchiverFactory::CreateArchiverRead(zCBuffer* buffer, int saveGame)
{
    buffer->SetMode(READ);
    buffer->SetPos(0);
    zCArchiver* archiver;
    zTArchiveMode mode;
    int flags;
    ReadHeader(saveGame, buffer, NULL, archiver, mode, flags);
    return archiver;
}

zCArchiver* zCArchiverFactory::CreateArchiverRead(zFILE* file, int saveGame)
{
    zCArchiver* archiver;
    zTArchiveMode mode;
    int flags;
    ReadHeader(saveGame, NULL, file, archiver, mode, flags);
    return archiver;
}

zCArchiver* zCArchiverFactory::CreateArchiverRead(const zSTRING& fileName, int saveGame)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    if (!file->Exists() || file->Open(0)) {
        zerr->Warning("D: zArchiver(::OpenFile): file does not exist: " + fileName);
        delete file;
        return NULL;
    }

    zCArchiver* archiver = CreateArchiverRead(file, saveGame);
    if (!archiver)
        delete file;
    return archiver;
}

zCArchiver* zCArchiverFactory::CreateArchiverWrite(zCBuffer* buffer, zTArchiveMode mode,
    int flags, int saveGame)
{
    if (!buffer)
        buffer = new zCBuffer(0x10000);
    buffer->SetMode(WRITE);
    buffer->SetPos(0);
    zCArchiver* archiver = CreateArchiverFromMode(mode);
    WriteHeader(archiver, mode, flags, saveGame, buffer, NULL);
    return archiver;
}

zCArchiver* zCArchiverFactory::CreateArchiverWrite(zTArchiveMode mode, int flags, int saveGame)
{
    return CreateArchiverWrite(NULL, mode, flags, saveGame);
}

zCArchiver* zCArchiverFactory::CreateArchiverWrite(zFILE* file, zTArchiveMode mode,
    int flags, int saveGame)
{
    zCArchiver* archiver = CreateArchiverFromMode(mode);
    WriteHeader(archiver, mode, flags, saveGame, NULL, file);
    return archiver;
}

zCArchiver* zCArchiverFactory::CreateArchiverWrite(const zSTRING& fileName,
    zTArchiveMode mode, int flags, int saveGame)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    if (file->Create()) {
        delete file;
        return NULL;
    }
    return CreateArchiverWrite(file, mode, flags, saveGame);
}

void zCArchiverGeneric::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCArchiverGeneric::zCArchiverGeneric()
{
    medium = 0;
    mode = 0;
    file = NULL;
    cbuffer = NULL;
    owningMedium = 0;
    inSaveGame = 0;
    binaryMode = 0;
    accessMode = 0;
    posNumObjects = 0;
    posChecksum = 0;
    posHeaderEnd = 0;
    checksumEnabled = 0;
    flags = 0;
    debugMessagesOn = 1;
    noReadSearchCycles = 0;
    deleteFileOnClose = 0;
    warnEntrysNotRead = 0;
    warnEntryNotFound = 0;
    warnWrongEntryOrder = 0;
    writeObjectList.Create(0x407);
}

zCObject* zCArchiverGeneric::_CreateNewInstance()
{
    zCArchiverGeneric* archiver = new zCArchiverGeneric;
    zCClassDef::ObjectCreated(archiver, classDef);
    return archiver;
}

zCClassDef* zCArchiverGeneric::_GetClassDef() const
{
    return classDef;
}

void zCArchiverGeneric::SkipOpenChunk()
{
    SkipChunk(1);
}

zFILE* zCArchiverGeneric::GetFile() const
{
    return file;
}

void zCArchiverGeneric::SetStringEOL(const zSTRING& value)
{
    stringEOL = value;
}

zSTRING zCArchiverGeneric::GetStringEOL() const
{
    return stringEOL;
}

int zCArchiverGeneric::GetChecksumEnabled() const
{
    return checksumEnabled;
}

void zCArchiverGeneric::SetChecksumEnabled(int enabled)
{
    checksumEnabled = enabled;
}

void zCArchiverGeneric::SetNoReadSearchCycles(int cycles)
{
    noReadSearchCycles = cycles;
}

int zCArchiverGeneric::InProperties() const
{
    return mode == zARC_MODE_ASCII_PROPS;
}

int zCArchiverGeneric::InSaveGame() const
{
    return inSaveGame;
}

int zCArchiverGeneric::InBinaryMode() const
{
    return mode == zARC_MODE_BINARY || mode == zARC_MODE_BINARY_SAFE;
}

zCArchiverGeneric::~zCArchiverGeneric()
{
    Close();
    writeObjectList.Clear();
    writeObjectList.Destroy();
    file = NULL;
    cbuffer = NULL;
}

void zCArchiverGeneric::DebugMessage(const zSTRING& message)
{
    if (debugMessagesOn && !(flags & zARC_FLAG_NO_SPY_MESSAGES))
        zerr->Message(message);
}

void zCArchiverGeneric::WriteHeaderNumObj()
{
    if (accessMode != 2 || (medium == 0 && !file))
        return;

    unsigned long position = StoreGetPos();
    StoreSeek(posNumObjects);
    StoreString(zSTRING(objectList.numInArray).ToChar());
    StoreSeek(position);
}

void zCArchiverGeneric::Close()
{
    if (accessMode == 0)
        return;

    if (accessMode == 2)
        WriteHeaderNumObj();

    if (owningMedium) {
        DebugMessage("D: ARC: .. Closing (" + zSTRING(objectList.numInArray) + " objects)");
        if (medium != 0) {
            DeleteBuffer();
        }
        else if (file) {
            file->Close();
            delete file;
            file = NULL;
        }
    }

    for (int i = objectList.numInArray - 1; i >= 0; --i) {
        if (objectList.parray[i]) {
            objectList.parray[i]->Release();
            objectList.parray[i] = NULL;
        }
    }
    objectList.numInArray = 0;
    chunkStack.numInArray = 0;
    writeObjectList.Clear();
    file = NULL;
    cbuffer = NULL;
    accessMode = 0;
}

int zCArchiverGeneric::EndOfArchive()
{
    if (medium != 0)
        return cbuffer->GetPos() >= cbuffer->GetSize();
    return file->Eof() || file->Pos() + 3 > file->Size();
}

void zCArchiverGeneric::DeleteBuffer()
{
    if (owningMedium && cbuffer)
        delete cbuffer;
    cbuffer = NULL;
}

void zCArchiverGeneric::GetBufferString(zSTRING& value)
{
    value.Clear();
    if (cbuffer && mode != zARC_MODE_BINARY) {
        WriteHeaderNumObj();
        const char terminator = 0;
        cbuffer->Write(&terminator, 1);
        cbuffer->SetPosBegin();
        value = reinterpret_cast<const char*>(cbuffer->GetCursor());
    }
}

zCBuffer* zCArchiverGeneric::GetBuffer()
{
    WriteHeaderNumObj();
    return cbuffer;
}

int zCArchiverGeneric::OpenWriteBuffer(zCBuffer* buffer, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags, int ownMedium)
{
    Close();
    medium = 1;
    mode = archiveMode;
    inSaveGame = saveGame;
    accessMode = 2;
    owningMedium = ownMedium;
    binaryMode = archiveMode == zARC_MODE_BINARY;
    flags = archiveFlags;
    debugMessagesOn = archiveMode != zARC_MODE_ASCII_PROPS;
    cbuffer = buffer;
    cbuffer->SetMode(WRITE);
    DebugMessage("D: ARC: Writing archive buffer (" + zSTRING(int(mode)) + ") ..");
    WriteHeader(0);
    return 1;
}

void zCArchiverGeneric::OpenWriteFile(zFILE* targetFile, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags, int ownMedium)
{
    if (!targetFile)
        return;

    Close();
    inSaveGame = saveGame;
    mode = archiveMode;
    medium = 0;
    accessMode = 2;
    file = targetFile;
    binaryMode = archiveMode == zARC_MODE_BINARY;
    owningMedium = ownMedium;
    flags = archiveFlags;
    debugMessagesOn = archiveMode != zARC_MODE_ASCII_PROPS;
    WriteHeader(0);
}

void zCArchiverGeneric::StoreBuffer(void* data, unsigned long size)
{
    if (size == 0)
        return;
    if (medium == 0)
        file->Write(data, size);
    else
        cbuffer->Write(data, size);
}

void zCArchiverGeneric::StoreString(const char* text)
{
    const unsigned long length = std::strlen(text);
    if (length > 0)
        StoreBuffer(const_cast<char*>(text), length);
}

unsigned long zCArchiverGeneric::StoreGetPos()
{
    if (medium != 0)
        return cbuffer->GetPos();
    return file->Pos();
}

void zCArchiverGeneric::StoreSeek(unsigned long position)
{
    if (medium != 0)
        cbuffer->SetPos(position);
    else
        file->Seek(position);
}

void zCArchiverGeneric::StoreStringEOL(const char* text)
{
    StoreString(text);
    StoreString(stringEOL.ToChar());
}

void zCArchiverGeneric::WriteHeader(int saveGame)
{
    StoreString("objects ");
    posNumObjects = StoreGetPos();
    StoreStringEOL("0        ");
    StoreStringEOL("END");
    StoreString(stringEOL.ToChar());
    posHeaderEnd = StoreGetPos();
}

int zCArchiverGeneric::IsStringValid(const char* text)
{
    for (const char* cursor = text; *cursor; ++cursor) {
        if (*cursor == '=' || *cursor == '%' || *cursor == '\xA7'
            || *cursor == '[' || *cursor == ']')
            return 0;
    }
    return 1;
}

void zCArchiverGeneric::WriteASCIILine(const char* name, const char* type,
    const zSTRING& value)
{
    if (!IsStringValid(name) || !IsStringValid(type)) {
        zerr->Fault("D: zArchiver(zCArchiverGeneric::WriteASCIILine): "
            "string contains illegal characters: \"" + zSTRING(name) + "\" / " + zSTRING(type));
    }

    for (int i = 0; i < chunkStack.numInArray; ++i)
        StoreBuffer(const_cast<char*>("\t"), 1);
    StoreString(name);
    StoreBuffer(const_cast<char*>("="), 1);
    StoreString(type);
    StoreBuffer(const_cast<char*>(":"), 1);
    StoreStringEOL(value.ToChar());
}

void zCArchiverGeneric::WriteInt(const char* name, int value)
{
    if (binaryMode) {
        StoreBuffer(&value, 4);
        return;
    }
    WriteASCIILine(name, "int", zSTRING(value));
}

void zCArchiverGeneric::WriteByte(const char* name, unsigned char value)
{
    if (binaryMode) {
        StoreBuffer(&value, 1);
        return;
    }
    WriteASCIILine(name, "int", zSTRING(int(value)));
}

void zCArchiverGeneric::WriteWord(const char* name, unsigned short value)
{
    if (binaryMode) {
        StoreBuffer(&value, 2);
        return;
    }
    WriteASCIILine(name, "int", zSTRING(int(value)));
}

void zCArchiverGeneric::WriteFloat(const char* name, float value)
{
    if (binaryMode) {
        StoreBuffer(&value, 4);
        return;
    }
    WriteASCIILine(name, "float", zSTRING(value, 8));
}

void zCArchiverGeneric::WriteBool(const char* name, int value)
{
    if (binaryMode) {
        unsigned char storedValue = static_cast<unsigned char>(value);
        StoreBuffer(&storedValue, 1);
        return;
    }
    WriteASCIILine(name, "bool", zSTRING(value));
}

void zCArchiverGeneric::WriteString(const char* name, const zSTRING& value)
{
    if (!binaryMode) {
        WriteASCIILine(name, "string", value);
        return;
    }

    StoreString(value.ToChar());
    const char terminator = 0;
    StoreBuffer(const_cast<char*>(&terminator), 1);
}

void zCArchiverGeneric::WriteVec3(const char* name, const zVEC3& value)
{
    if (binaryMode) {
        StoreBuffer(const_cast<zVEC3*>(&value), 12);
        return;
    }

    zSTRING text = zSTRING(value.n[0], 8) + " " + zSTRING(value.n[1], 8)
        + " " + zSTRING(value.n[2], 8);
    WriteASCIILine(name, "vec3", text);
}

void zCArchiverGeneric::WriteColor(const char* name, const zCOLOR& value)
{
    if (binaryMode) {
        StoreBuffer(const_cast<zCOLOR*>(&value), 4);
        return;
    }
    WriteASCIILine(name, "color", value.GetDescription());
}

void zCArchiverGeneric::WriteRaw(const char* name, void* data, unsigned long size)
{
    if (binaryMode) {
        StoreBuffer(data, size);
        return;
    }

    static const char digits[] = "0123456789abcdef";
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    zSTRING text;
    for (unsigned long i = 0; i < size; ++i) {
        char encoded[3];
        encoded[0] = digits[bytes[i] >> 4];
        encoded[1] = digits[bytes[i] & 15];
        encoded[2] = 0;
        text += encoded;
    }
    WriteASCIILine(name, "raw", text);
}

void zCArchiverGeneric::WriteRawFloat(const char* name, void* data, unsigned long size)
{
    if (size & 3) {
        zerr->Fatal("D: zArchiver(zCArchiverGeneric::WriteRawFloat): "
            "raw float buffer has illegal size (mod4!=0): " + zSTRING(size));
    }
    if (binaryMode) {
        StoreBuffer(data, size);
        return;
    }

    const float* values = static_cast<const float*>(data);
    zSTRING text;
    for (unsigned long i = 0; i < size / 4; ++i)
        text = text + zSTRING(values[i], 8) + " ";
    WriteASCIILine(name, "rawFloat", text);
}

void zCArchiverGeneric::WriteEnum(const char* name, const char* enumName, int value)
{
    if (binaryMode) {
        WriteByte(name, static_cast<unsigned char>(value));
        return;
    }

    zSTRING type = "enum";
    if (InProperties())
        type = "enum;" + zSTRING(enumName);
    WriteASCIILine(name, type.ToChar(), zSTRING(value));
}

void zCArchiverGeneric::WriteChunkStart(const char* name, const char* className,
    unsigned short version, unsigned long objectIndex)
{
    zTChunkRecord record;
    record.startPos = StoreGetPos();
    record.size = 0;
    record.version = version;
    record.objectIndex = objectIndex;
    record.name = name;
    record.className = className;

    if (binaryMode) {
        StoreBuffer(&record.size, 4);
        StoreBuffer(&record.version, 2);
        StoreBuffer(&record.objectIndex, 4);
        StoreString(name);
        const char terminator = 0;
        StoreBuffer(const_cast<char*>(&terminator), 1);
        StoreString(className);
        StoreBuffer(const_cast<char*>(&terminator), 1);
    }
    else {
        for (int i = 0; i < chunkStack.numInArray; ++i)
            StoreBuffer(const_cast<char*>("\t"), 1);
        zSTRING text = "[" + record.name + " " + record.className + " "
            + zSTRING(int(version)) + " " + zSTRING(objectIndex) + "]";
        StoreStringEOL(text.ToChar());
    }
    chunkStack.InsertEnd(record);
}

void zCArchiverGeneric::WriteChunkStart(const char* name, unsigned short version)
{
    WriteChunkStart(name, "", version, 0);
}

void zCArchiverGeneric::WriteChunkEnd()
{
    if (chunkStack.numInArray == 0) {
        zerr->Fatal("D: zArchiver(zCArchiverGeneric::WriteChunkEnd): "
            "write chunk end with no open chunk !");
        return;
    }

    zTChunkRecord record = chunkStack.parray[chunkStack.numInArray - 1];
    --chunkStack.numInArray;

    if (!binaryMode) {
        for (int i = 0; i < chunkStack.numInArray; ++i)
            StoreBuffer(const_cast<char*>("\t"), 1);
        StoreStringEOL("[]");
        return;
    }

    record.size = StoreGetPos() - record.startPos;
    const unsigned long position = StoreGetPos();
    StoreSeek(record.startPos);
    StoreBuffer(&record.size, 4);
    StoreSeek(position);
}

void zCArchiverGeneric::WriteObject(const char* name, zCObject& object)
{
    zCClassDef* objectClass = object._GetClassDef();
    if (objectClass->classFlags & 2) {
        WriteObject(name, NULL);
        return;
    }
    if (!objectClass) {
        zerr->Fatal("D: zArchiver(zCArchiverGeneric::WriteObject): "
            "zCObject without ClassDef, zCLASS_.. macros missing");
        return;
    }

    zSTRING className = objectClass->className;
    for (zCClassDef* baseClass = objectClass->baseClassDef; baseClass;
        baseClass = baseClass->baseClassDef) {
        if (baseClass->createNewInstance)
            className = className + ":" + baseClass->className;
    }

    const int objectIndex = objectList.numInArray;
    object.AddRef();
    objectList.InsertEnd(&object);
    zTWriteObjectEntry* entry = static_cast<zTWriteObjectEntry*>(
        writeObjectList.Insert(&object));
    entry->object = &object;
    entry->objectIndex = objectIndex;

    WriteChunkStart(name, className.ToChar(), objectClass->archiveVersion, objectIndex);
    object.Archive(*this);
    WriteChunkEnd();
}

void zCArchiverGeneric::WriteObject(const char* name, zCObject* object)
{
    if (object && !(object->_GetClassDef()->classFlags & 2)) {
        zTWriteObjectEntry* entry = static_cast<zTWriteObjectEntry*>(
            writeObjectList.Element(&object));
        if (entry && entry->objectIndex >= 0) {
            WriteChunkStart(name, "\xA7", 0, entry->objectIndex);
            WriteChunkEnd();
            return;
        }
        WriteObject(name, *object);
        return;
    }

    WriteChunkStart(name, "%", 0, 0);
    WriteChunkEnd();
}

void zCArchiverGeneric::WriteObject(zCObject* object)
{
    WriteObject("", object);
}

void zCArchiverGeneric::WriteGroupBegin(const char* name)
{
    if (InProperties())
        WriteASCIILine(name, "groupBegin", zSTRING(""));
}

void zCArchiverGeneric::WriteGroupEnd(const char* name)
{
    if (InProperties())
        WriteASCIILine(name, "groupEnd", zSTRING(""));
}

int zCArchiverGeneric::OpenReadBuffer(zCBuffer& buffer, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags)
{
    Close();
    medium = 1;
    accessMode = 1;
    binaryMode = archiveMode == zARC_MODE_BINARY;
    inSaveGame = saveGame;
    owningMedium = 0;
    flags = archiveFlags;
    mode = archiveMode;
    cbuffer = &buffer;
    cbuffer->SetMode(READ);

    if (EndOfArchive()) {
        Close();
        return 0;
    }

    ReadHeader();
    binaryMode = archiveMode == zARC_MODE_BINARY;
    debugMessagesOn = archiveMode != zARC_MODE_ASCII_PROPS;
    DebugMessage("D: ARC: Reading archive buffer (" + zSTRING(int(mode)) + ") ..");
    return 1;
}

void zCArchiverGeneric::OpenReadFile(zFILE* sourceFile, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags, int ownMedium)
{
    if (!sourceFile)
        return;

    Close();
    inSaveGame = saveGame;
    binaryMode = archiveMode == zARC_MODE_BINARY;
    file = sourceFile;
    owningMedium = ownMedium;
    medium = 0;
    accessMode = 1;
    mode = archiveMode;
    ReadHeader();
    binaryMode = archiveMode == zARC_MODE_BINARY;
    debugMessagesOn = archiveMode != zARC_MODE_ASCII_PROPS;
}

void zCArchiverGeneric::RestoreBuffer(void* data, unsigned long size)
{
    if (medium != 0)
        cbuffer->Read(data, size);
    else
        file->Read(data, size);
}

void zCArchiverGeneric::RestoreStringEOL(zSTRING& value)
{
    value.Clear();
    char text[5000];
    int length = 0;

    while (true) {
        char current;
        RestoreBuffer(&current, 1);
        if ((!binaryMode && current == '\n') || (binaryMode && current == 0))
            break;
        if (!binaryMode && (current == '\t' || current == '\r'))
            continue;

        text[length++] = current;
        if (length == 5000) {
            text[length] = 0;
            value += text;
            length = 0;
        }
    }

    text[length] = 0;
    value += text;
}

void zCArchiverGeneric::RestoreString0(zSTRING& value)
{
    RestoreStringEOL(value);
}

unsigned long zCArchiverGeneric::RestoreGetPos()
{
    if (medium != 0)
        return cbuffer->GetPos();
    return file->Pos();
}

void zCArchiverGeneric::RestoreSeek(unsigned long position)
{
    if (position == RestoreGetPos())
        return;
    if (medium != 0)
        cbuffer->SetPos(position);
    else
        file->Seek(position);
}

void zCArchiverGeneric::ReadHeader()
{
    zSTRING line;
    RestoreStringEOL(line);
    const zSTRING keyword = line.PickWord(1, " ", " ");

    if (keyword != zSTRING("objects")) {
        zerr->Fatal("D: zCArchiverGeneric: cannot read archive, 'objects' keyword expected");
    }
    else {
        const int count = line.PickWord(2, " ", " ").ToInt();
        CheckObjectListSize(count);
    }

    RestoreStringEOL(line);
    RestoreStringEOL(line);
    posHeaderEnd = RestoreGetPos();
}

void zCArchiverGeneric::CheckObjectListSize(int size)
{
    const int oldSize = objectList.numInArray;
    if (size <= oldSize)
        return;

    const int newSize = size + size / 2;
    if (objectList.numAlloc < newSize)
        objectList.AllocDelta(newSize - objectList.numAlloc);

    objectList.numInArray = objectList.numAlloc;
    for (int i = oldSize; i < objectList.numInArray; ++i)
        objectList.parray[i] = NULL;
}

zCObject* zCArchiverGeneric::CreateObject(const zSTRING& className)
{
    zCClassDef* objectClass = GetClassDefByString(className);
    if (!objectClass)
        return NULL;

    zCObject* object = objectClass->CreateNewInstance();
    if (!object) {
        zerr->Fault("D: zArchiver(zCArchiverGeneric::CreateObject): "
            "could not create object for: " + className);
    }
    return object;
}

zCClassDef* zCArchiverGeneric::GetClassDefByString(const zSTRING& className)
{
    int word = 1;
    zCClassDef* result;
    zSTRING name;

    do {
        name = className.PickWord(word++, ":", ":");
        result = zCClassDef::GetClassDef(name);
        if (result)
            break;
    } while (name.Length() > 0);

    return result;
}

void zCArchiverGeneric::ReadChunkStartASCII(const char* name, zSTRING& result)
{
    result.Clear();
    const unsigned long startPosition = RestoreGetPos();
    int searchCycle = noReadSearchCycles;
    zSTRING line;

    do {
        RestoreStringEOL(line);
        const char* text = line.ToChar();
        if (text[0] != '[')
            continue;

        if (text[1] == ']') {
            if (searchCycle == 1 || chunkStack.numInArray == 0) {
                RestoreSeek(startPosition);
                break;
            }
            RestoreSeek(chunkStack.parray[chunkStack.numInArray - 1].startPos);
            ++searchCycle;
            continue;
        }

        zSTRING chunkName = line.PickWord(1, "[ ]", "[ ]");
        if (chunkName.IsEmpty() || zSTRING(name).IsEmpty() || chunkName == zSTRING(name)) {
            result = line;
            break;
        }
        SkipChunk(0);
    } while (!EndOfArchive());
}

int zCArchiverGeneric::ReadChunkStart(const char* name)
{
    zTChunkRecord record;
    record.startPos = RestoreGetPos();
    record.size = 0;

    if (binaryMode) {
        RestoreBuffer(&record.size, 4);
        RestoreBuffer(&record.version, 2);
        RestoreBuffer(&record.objectIndex, 4);
        RestoreString0(record.name);
        RestoreString0(record.className);

        if (name && *name && record.name != zSTRING(name)) {
            RestoreSeek(record.startPos);
            return 0;
        }
    }
    else {
        zSTRING line;
        ReadChunkStartASCII(name, line);
        if (line.IsEmpty())
            return 0;

        record.name = line.PickWord(1, "[ ]", "[ ]");
        record.className = line.PickWord(2, "[ ]", "[ ]");
        record.version = static_cast<unsigned short>(line.PickWord(3, "[ ]", "[ ]").ToInt());
        record.objectIndex = line.PickWord(4, "[ ]", "[ ]").ToInt();
    }

    chunkStack.InsertEnd(record);
    return 1;
}

int zCArchiverGeneric::ReadChunkStart(zSTRING& name, unsigned short& version)
{
    const int result = ReadChunkStart("");
    if (!result) {
        name.Clear();
        return result;
    }

    const zTChunkRecord& record = chunkStack.parray[chunkStack.numInArray - 1];
    name = record.name;
    version = record.version;
    return result;
}

int zCArchiverGeneric::ReadChunkStartNamed(const char* name, unsigned short& version)
{
    const int result = ReadChunkStart(name);
    if (result)
        version = chunkStack.parray[chunkStack.numInArray - 1].version;
    return result;
}

unsigned short zCArchiverGeneric::GetCurrentChunkVersion()
{
    if (chunkStack.numInArray > 0)
        return chunkStack.parray[chunkStack.numInArray - 1].version;
    return 0;
}

void zCArchiverGeneric::ReadChunkEnd()
{
    SkipChunk(1);
}

zCObject* zCArchiverGeneric::ReadObject(const char* name, zCObject* object)
{
    if (!ReadChunkStart(name))
        return object;

    const zTChunkRecord& record = chunkStack.parray[chunkStack.numInArray - 1];
    const char* className = record.className.ToChar();
    zCObject* result = object;

    if (className[0] == '%') {
        SkipChunk(1);
        return NULL;
    }

    if (className[0] == '\xA7') {
        const unsigned long index = record.objectIndex;
        if (index >= static_cast<unsigned long>(objectList.numInArray)) {
            zerr->Fault("D: zArchiver(::ReadObject): objRef, illegal object index: "
                + zSTRING(index));
            result = object;
        }
        else {
            result = objectList.parray[index];
            if (!result) {
                zerr->Fault("D: zArchiver(::ReadObject): objRef, referenced object has not "
                    "been read, object index: " + zSTRING(index));
                result = object;
            }
            else if (!object) {
                result->AddRef();
            }
            else if (result != object) {
                zerr->Warning("D: zArchiver(::ReadObject): found an objectPtr in file, "
                    "tried to write this to an 'objectUseThis'");
                result = object;
            }
        }
        SkipChunk(1);
        return result;
    }

    int skip = 0;
    if (!result) {
        result = CreateObject(record.className);
    }
    else if (result->_GetClassDef() != GetClassDefByString(record.className)) {
        zerr->Warning("D: zArchiver(::ReadObject): class from file does not match class "
            "from 'objectUseThis'");
        if (binaryMode) {
            CheckObjectListSize(record.objectIndex + 1);
            objectList.parray[record.objectIndex] = result;
            result->AddRef();
            skip = 1;
        }
    }

    if (!result || skip) {
        SkipChunk(1);
        return result;
    }

    if (binaryMode && result->_GetClassDef()->archiveVersion != record.version) {
        zerr->Warning("D: Archiver(::ReadObject): skipping object-chunk, tried to read "
            "object-chunk with out-of-date version: " + record.className);
        SkipChunk(1);
        if (!object)
            result->Release();
        return object;
    }

    CheckObjectListSize(record.objectIndex + 1);
    if (objectList.parray[record.objectIndex]) {
        objectList.parray[record.objectIndex]->Release();
        objectList.parray[record.objectIndex] = NULL;
    }
    result->AddRef();
    objectList.parray[record.objectIndex] = result;
    result->Unarchive(*this);
    ReadChunkEnd();
    return result;
}

zCObject* zCArchiverGeneric::ReadObject(zCObject* object)
{
    return ReadObject("", object);
}

void zCArchiverGeneric::SkipChunk(int onlyCurrent)
{
    if (chunkStack.numInArray <= 0)
        return;

    if (!binaryMode) {
        int depth = 1;
        int linesRead = 0;
        zSTRING line;
        while (depth > 0) {
            RestoreStringEOL(line);
            const char* text = line.ToChar();
            if (text[0] == '[') {
                if (text[1] == ']')
                    --depth;
                else
                    ++depth;
            }
            ++linesRead;
            if (linesRead > 1)
                warnEntrysNotRead = 1;
        }
    }
    else {
        const zTChunkRecord& record = chunkStack.parray[chunkStack.numInArray - 1];
        RestoreSeek(record.startPos + record.size);
    }

    if (onlyCurrent)
        --chunkStack.numInArray;
}

int zCArchiverGeneric::ReadEntryASCII(const char* name, zSTRING& value)
{
    int searchCycle = noReadSearchCycles;
    zSTRING line;

    while (!EndOfArchive()) {
        RestoreStringEOL(line);
        const char* text = line.ToChar();
        if (text[0] == '[') {
            if (text[1] == ']') {
                RestoreSeek(chunkStack.parray[chunkStack.numInArray - 1].startPos);
                if (searchCycle == 1) {
                    warnEntryNotFound = 1;
                    zerr->Warning("D: expected entry not found: " + zSTRING(name));
                    return 0;
                }
                ++searchCycle;
            }
            else {
                SkipChunk(0);
            }
            continue;
        }

        if (!*text)
            continue;

        const zSTRING key = line.PickWord(1, " =\t", " =\t");
        if (key == zSTRING(name)) {
            int colon = 0;
            while (text[colon] && text[colon] != ':')
                ++colon;
            if (text[colon] == ':') {
                value = line;
                value.Copy(colon + 1, 0xFFFFFFFF);
                return 1;
            }
        }
        warnWrongEntryOrder = 1;
    }
    return 0;
}

float zCArchiverGeneric::ReadFloat(const char* name)
{
    float value = 0.0f;
    ReadFloat(name, value);
    return value;
}

zVEC3 zCArchiverGeneric::ReadVec3(const char* name)
{
    zVEC3 value;
    ReadVec3(name, value);
    return value;
}

int zCArchiverGeneric::ReadInt(const char* name)
{
    int value = 0;
    ReadInt(name, value);
    return value;
}

unsigned char zCArchiverGeneric::ReadByte(const char* name)
{
    unsigned char value = 0;
    ReadByte(name, value);
    return value;
}

unsigned short zCArchiverGeneric::ReadWord(const char* name)
{
    unsigned short value = 0;
    ReadWord(name, value);
    return value;
}

int zCArchiverGeneric::ReadBool(const char* name)
{
    int value = 0;
    ReadBool(name, value);
    return value;
}

zSTRING zCArchiverGeneric::ReadString(const char* name)
{
    zSTRING value;
    ReadString(name, value);
    return value;
}

zCOLOR zCArchiverGeneric::ReadColor(const char* name)
{
    zCOLOR value(0);
    ReadColor(name, value);
    return value;
}

int zCArchiverGeneric::ReadEnum(const char* name)
{
    int value = 0;
    ReadEnum(name, value);
    return value;
}

void zCArchiverGeneric::ReadRaw(const char* name, void* data, unsigned long size)
{
    if (binaryMode) {
        RestoreBuffer(data, size);
        return;
    }

    ReadEntryASCII(name, tmpResultValue);
    const int available = tmpResultValue.Length() / 2;
    if (available != size) {
        zerr->Fault("D: zArchiver(zCArchiverGeneric::ReadRaw): size of requested data "
            "does not equal size of available data: " + zSTRING(available));
    }

    const char* text = tmpResultValue.ToChar();
    unsigned char* output = static_cast<unsigned char*>(data);
    for (int i = 0; i < available; ++i) {
        const char high = text[i * 2];
        const char low = text[i * 2 + 1];
        const int highValue = high <= '9' ? high - '0'
            : (high & ~0x20) - 'A' + 10;
        const int lowValue = low <= '9' ? low - '0'
            : (low & ~0x20) - 'A' + 10;
        output[i] = static_cast<unsigned char>((highValue << 4) | lowValue);
    }
}

void zCArchiverGeneric::ReadRawFloat(const char* name, void* data, unsigned long size)
{
    if (binaryMode) {
        RestoreBuffer(data, size);
        return;
    }

    ReadEntryASCII(name, tmpResultValue);
    float* values = static_cast<float*>(data);
    for (unsigned long i = 0; i < size / 4; ++i)
        values[i] = tmpResultValue.PickWord(i + 1, " ", " ").ToFloat();
}

void zCArchiverGeneric::ReadFloat(const char* name, float& value)
{
    if (binaryMode) {
        RestoreBuffer(&value, 4);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value = tmpResultValue.ToFloat();
}

void zCArchiverGeneric::ReadVec3(const char* name, zVEC3& value)
{
    if (binaryMode) {
        RestoreBuffer(&value, 12);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue)) {
        for (int i = 0; i < 3; ++i)
            value.n[i] = tmpResultValue.PickWord(i + 1, " ", " ").ToFloat();
    }
}

void zCArchiverGeneric::ReadInt(const char* name, int& value)
{
    if (binaryMode) {
        RestoreBuffer(&value, 4);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value = tmpResultValue.ToInt();
}

void zCArchiverGeneric::ReadByte(const char* name, unsigned char& value)
{
    if (binaryMode) {
        RestoreBuffer(&value, 1);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value = static_cast<unsigned char>(tmpResultValue.ToInt());
}

void zCArchiverGeneric::ReadWord(const char* name, unsigned short& value)
{
    if (binaryMode) {
        RestoreBuffer(&value, 2);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value = static_cast<unsigned short>(tmpResultValue.ToInt());
}

void zCArchiverGeneric::ReadBool(const char* name, int& value)
{
    if (binaryMode) {
        unsigned char storedValue;
        RestoreBuffer(&storedValue, 1);
        value = storedValue;
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value = tmpResultValue.ToInt();
}

void zCArchiverGeneric::ReadString(const char* name, zSTRING& value)
{
    if (!binaryMode) {
        ReadEntryASCII(name, value);
        return;
    }
    RestoreString0(value);
}

void zCArchiverGeneric::ReadColor(const char* name, zCOLOR& value)
{
    if (binaryMode) {
        RestoreBuffer(&value, 4);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value.SetByDescription(tmpResultValue);
}

void zCArchiverGeneric::ReadEnum(const char* name, int& value)
{
    if (binaryMode) {
        value = ReadByte(name);
        return;
    }
    if (ReadEntryASCII(name, tmpResultValue))
        value = tmpResultValue.ToInt();
}

void zCArchiverGeneric::TestClassIntegrity(
    void (*callback)(zCObject*, int, zCClassDef*, int&))
{
    const zTArchiveMode modes[] = {
        zARC_MODE_BINARY,
        zARC_MODE_ASCII,
        zARC_MODE_ASCII_PROPS,
        zARC_MODE_BINARY_SAFE
    };

    for (int classIndex = 0; classIndex < zCClassDef::classDefList->numInArray; ++classIndex) {
        zCClassDef* objectClass = zCClassDef::classDefList->array[classIndex];
        if (!objectClass->createNewInstance)
            continue;

        int testCount = 1;
        if (callback)
            callback(NULL, 0, objectClass, testCount);
        if (testCount <= 0)
            continue;

        zerr->Message("D: ARC: >>> Testing class: " + objectClass->className);
        for (int testIndex = 0; testIndex < testCount; ++testIndex) {
            for (int testMode = 0; testMode < 4; ++testMode) {
                zCObject* first = objectClass->CreateNewInstance();
                zCObject* second = objectClass->CreateNewInstance();
                zCArchiver* writer = zarcFactory->CreateArchiverWrite(NULL, modes[testMode], 2, 0);

                if (callback)
                    callback(first, testMode, objectClass, testIndex);
                writer->WriteObject(first);
                zCBuffer* buffer = writer->GetBuffer();

                zCArchiver* reader = zarcFactory->CreateArchiverRead(buffer, 2);
                if (callback)
                    callback(second, testMode, objectClass, testIndex);
                reader->ReadObject(second);
                reader->Close();
                reader->Release();

                buffer->SetMode(READ);
                reader = zarcFactory->CreateArchiverRead(buffer, 2);
                reader->Close();
                reader->Release();
                writer->Release();
                first->Release();
                second->Release();
            }
        }
    }
}

zCObject* zCArchiverGeneric::GetParentObject()
{
    for (int i = chunkStack.numInArray - 2; i >= 0; --i) {
        const zTChunkRecord& record = chunkStack.parray[i];
        if (!record.className.IsEmpty() && record.className.ToChar()[0] != '%')
            return objectList.parray[record.objectIndex];
    }
    return NULL;
}

zCArchiverChunkEntry::~zCArchiverChunkEntry()
{
    if (key) {
        delete key;
        key = NULL;
    }
}

unsigned long GETStringMap<unsigned long>::SetAt(const char* name, unsigned long* data)
{
    unsigned long hash = 0;
    for (const char* cursor = name; *cursor; ++cursor)
        hash = hash * 33 + *cursor;

    const unsigned long bucket = hash % tableSize;
    if (table) {
        for (GESNextEntry* entry = table[bucket]; entry; entry = entry->next) {
            if (entry->key->string && std::strcmp(entry->key->string, name) == 0)
                return entry->value;
        }
    }

    if (count < 0)
        return 0xFFFFFFFF;
    if (!table) {
        table = new GESNextEntry*[tableSize];
        std::memset(table, 0, tableSize * sizeof(GESNextEntry*));
    }

    GESNextEntry* entry = new GESNextEntry;
    entry->hash = hash;
    entry->value = count++;
    entry->key = new GECString;
    entry->key->string = NULL;
    entry->key->Copy(name);
    entry->data = data;
    entry->next = table[bucket];
    table[bucket] = entry;
    return entry->value;
}

unsigned long GETStringMap<unsigned long>::SetAtForce(const char* name, unsigned long* data,
    unsigned long hash, unsigned long value)
{
    if (count < 0)
        return 0xFFFFFFFF;
    if (!table) {
        table = new GESNextEntry*[tableSize];
        std::memset(table, 0, tableSize * sizeof(GESNextEntry*));
    }

    const unsigned long bucket = hash % tableSize;
    GESNextEntry* entry = new GESNextEntry;
    entry->hash = hash;
    entry->value = value;
    entry->key = new GECString;
    entry->key->string = NULL;
    if (name)
        entry->key->Copy(name);
    entry->data = data;
    entry->next = table[bucket];
    table[bucket] = entry;
    ++count;
    return value;
}

void GETStringMap<unsigned long>::RemoveAll(bool deleteData)
{
    if (table) {
        for (unsigned long i = 0; i < tableSize; ++i) {
            GESNextEntry* entry = table[i];
            while (entry) {
                GESNextEntry* next = entry->next;
                if (deleteData)
                    delete entry->data;
                delete entry;
                entry = next;
            }
        }
        delete[] table;
        table = NULL;
    }
    count = 0;
    reserved = NULL;
}

unsigned long GETStringMap<unsigned long>::Lookup(const char* name, unsigned long*& data,
    unsigned long value) const
{
    unsigned long bucket;
    GESNextEntry* entry = GetElement(name, bucket, value);
    if (!entry)
        return 0xFFFFFFFF;
    data = entry->data;
    return entry->value;
}

bool GETStringMap<unsigned long>::GetNextElement(unsigned long& hash, GECString& key,
    unsigned long*& data, unsigned long& value)
{
    if (count == 0 || !table)
        return false;

    GESNextEntry* current = static_cast<GESNextEntry*>(reserved);
    if (current && current->next) {
        reserved = current->next;
    }
    else {
        reserved = NULL;
        const unsigned long firstBucket = current ? current->hash + 1 : 0;
        for (unsigned long i = firstBucket; i < tableSize && !reserved; ++i)
            reserved = table[i];
    }

    if (!reserved)
        return false;

    current = static_cast<GESNextEntry*>(reserved);
    if (current->key->string)
        key.Copy(current->key->string);
    else
        key.Free();
    value = current->value;
    data = current->data;
    hash = current->hash;
    return true;
}

GETStringMap<unsigned long>::GESNextEntry* GETStringMap<unsigned long>::GetElement(
    const char* name, unsigned long& bucket, unsigned long value) const
{
    unsigned long hash = 0;
    for (const char* cursor = name; *cursor; ++cursor)
        hash = hash * 33 + *cursor;

    bucket = hash % tableSize;
    if (table) {
        for (GESNextEntry* entry = table[bucket]; entry; entry = entry->next) {
            if (value != 0xFFFFFFFF) {
                if (entry->value == value)
                    return entry;
            }
            else if (entry->key->string && std::strcmp(entry->key->string, name) == 0) {
                return entry;
            }
        }
    }
    return NULL;
}

zCArchiverBinSafe::zCArchiverBinSafe()
{
    medium = 0;
    mode = 0;
    file = NULL;
    cbuffer = NULL;
    owningMedium = 0;
    inSaveGame = 0;
    accessMode = 0;
    posNumObjects = 0;
    posChecksum = 0;
    posHeaderEnd = 0;
    checksumEnabled = 0;
    flags = 0;
    debugMessagesOn = 1;
    noReadSearchCycles = 0;
    deleteFileOnClose = 0;
    warnEntrysNotRead = 0;
    warnEntryNotFound = 0;
    warnWrongEntryOrder = 0;
    dummyVal = 0;
    posChunkList = 0;
    m_readTextBuffer[0] = 0;

    zCArchiverChunkMap* map = new zCArchiverChunkMap;
    map->reserved = NULL;
    map->table = NULL;
    map->tableSize = 97;
    map->count = 0;
    stringHashMap = map;
}

zCObject* zCArchiverBinSafe::_CreateNewInstance()
{
    zCArchiverBinSafe* archiver = new zCArchiverBinSafe;
    zCClassDef::ObjectCreated(archiver, classDef);
    return archiver;
}

zCClassDef* zCArchiverBinSafe::_GetClassDef() const
{
    return classDef;
}

void zCArchiverBinSafe::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCArchiverBinSafe::RestoreString(zSTRING& value)
{
    tmpResultValue = value;
}

void zCArchiverBinSafe::WriteType(zTArchiveTypeID type, void* data, unsigned long size)
{
    unsigned char storedType = static_cast<unsigned char>(type);
    StoreBuffer(&storedType, 1);

    if (type == zARC2_ID_STRING || type == zARC2_ID_RAW || type == zARC2_ID_RAWFLOAT) {
        unsigned short storedSize = static_cast<unsigned short>(size);
        StoreBuffer(&storedSize, 2);
    }
    StoreBuffer(data, size);
}

int zCArchiverBinSafe::ReadType(zTArchiveTypeID type, void* data, unsigned long size)
{
    unsigned char storedType;
    RestoreBuffer(&storedType, 1);

    if (storedType == type) {
        if (type == zARC2_ID_STRING) {
            unsigned short storedSize;
            RestoreBuffer(&storedSize, 2);
            if (storedSize > 0)
                RestoreBuffer(m_readTextBuffer, storedSize);
            m_readTextBuffer[storedSize] = 0;
            *static_cast<zSTRING*>(data) = m_readTextBuffer;
        }
        else if (type == zARC2_ID_RAW || type == zARC2_ID_RAWFLOAT) {
            unsigned short storedSize;
            RestoreBuffer(&storedSize, 2);
            RestoreBuffer(data, storedSize);
        }
        else {
            RestoreBuffer(data, size);
        }
        return 1;
    }

    unsigned char discarded[12];
    unsigned short storedSize;

    switch (storedType) {
    case zARC2_ID_STRING:
        RestoreBuffer(&storedSize, 2);
        RestoreBuffer(m_readTextBuffer, storedSize);
        m_readTextBuffer[storedSize] = 0;
        break;
    case zARC2_ID_INTEGER:
    case zARC2_ID_FLOAT:
    case zARC2_ID_INT:
    case zARC2_ID_COLOR:
    case zARC2_ID_ENUM:
    case zARC2_ID_HASH:
        RestoreBuffer(discarded, 4);
        break;
    case zARC2_ID_BYTE:
        RestoreBuffer(discarded, 1);
        break;
    case zARC2_ID_WORD:
        RestoreBuffer(discarded, 2);
        break;
    case zARC2_ID_VEC3:
        RestoreBuffer(discarded, 12);
        break;
    case zARC2_ID_RAW:
    case zARC2_ID_RAWFLOAT:
        RestoreBuffer(&storedSize, 2);
        RestoreBuffer(m_readTextBuffer, storedSize);
        break;
    default:
        break;
    }
    return 0;
}

void zCArchiverBinSafe::ClearChunkList()
{
    zCArchiverChunkMap* map = static_cast<zCArchiverChunkMap*>(stringHashMap);

    if (map->table) {
        for (unsigned long i = 0; i < map->tableSize; ++i) {
            zCArchiverChunkEntry* entry = map->table[i];
            while (entry) {
                zCArchiverChunkEntry* next = entry->next;
                delete entry;
                entry = next;
            }
        }
        delete[] map->table;
        map->table = NULL;
    }
    map->count = 0;
    delete map;
}

void GECString::Free()
{
    if (string) {
        delete[] string;
        string = NULL;
    }
}

unsigned long zCArchiverBinSafe::InsertChunkInList(const char* name)
{
    zCArchiverChunkMap* map = static_cast<zCArchiverChunkMap*>(stringHashMap);
    unsigned long hash = 0;
    for (const char* cursor = name; *cursor; ++cursor)
        hash = hash * 33 + *cursor;

    if (!map->table) {
        map->table = new zCArchiverChunkEntry*[map->tableSize];
        std::memset(map->table, 0, map->tableSize * sizeof(zCArchiverChunkEntry*));
    }

    const unsigned long bucket = hash % map->tableSize;
    for (zCArchiverChunkEntry* entry = map->table[bucket]; entry; entry = entry->next) {
        if (entry->key->string && std::strcmp(entry->key->string, name) == 0)
            return entry->value;
    }

    zCArchiverChunkEntry* entry = new zCArchiverChunkEntry;
    entry->hash = hash;
    entry->value = map->count++;
    entry->key = new GECString;
    entry->key->string = new char[std::strlen(name) + 1];
    std::strcpy(entry->key->string, name);
    entry->data = &dummyVal;
    entry->next = map->table[bucket];
    map->table[bucket] = entry;
    return entry->value;
}

unsigned long zCArchiverBinSafe::ResolveAndPosEntry(const char* name)
{
    zCArchiverChunkMap* map = static_cast<zCArchiverChunkMap*>(stringHashMap);
    unsigned long hash = 0;
    for (const char* cursor = name; *cursor; ++cursor)
        hash = hash * 33 + *cursor;

    if (map->table) {
        const unsigned long bucket = hash % map->tableSize;
        for (zCArchiverChunkEntry* entry = map->table[bucket]; entry; entry = entry->next) {
            if (entry->key->string && std::strcmp(entry->key->string, name) == 0)
                return entry->value;
        }
    }
    return 0xFFFFFFFF;
}

static zCClassDef zCArchiverClassDef("zCArchiver", "zCObject", NULL, 0, 0x2C, 0, "");
zCClassDef* zCArchiver::classDef = &zCArchiverClassDef;
zSTRING zCArchiver::stringEOL;

zCClassDef* zCArchiver::_GetClassDef() const
{
    return classDef;
}

zCArchiver::~zCArchiver()
{
}

void zCArchiverBinSafe::SkipOpenChunk()
{
    SkipChunk(1);
}

zFILE* zCArchiverBinSafe::GetFile() const
{
    return file;
}

void zCArchiverBinSafe::SetStringEOL(const zSTRING& value)
{
    stringEOL = value;
}

zSTRING zCArchiverBinSafe::GetStringEOL() const
{
    return stringEOL;
}

int zCArchiverBinSafe::GetChecksumEnabled() const
{
    return checksumEnabled;
}

void zCArchiverBinSafe::SetChecksumEnabled(int enabled)
{
    checksumEnabled = enabled;
}

void zCArchiverBinSafe::SetNoReadSearchCycles(int cycles)
{
    noReadSearchCycles = cycles;
}

int zCArchiverBinSafe::InProperties() const
{
    return mode == zARC_MODE_ASCII_PROPS;
}

int zCArchiverBinSafe::InSaveGame() const
{
    return inSaveGame;
}

int zCArchiverBinSafe::InBinaryMode() const
{
    return 1;
}

int zCArchiverBinSafe::EndOfArchive()
{
    if (medium != 0)
        return cbuffer->GetPos() >= cbuffer->GetSize();
    return file->Eof() || file->Pos() + 3 > file->Size();
}

void zCArchiverBinSafe::DeleteBuffer()
{
    if (owningMedium && cbuffer)
        delete cbuffer;
    cbuffer = NULL;
}

void zCArchiverBinSafe::GetBufferString(zSTRING& value)
{
    value.Clear();
    if (cbuffer && mode != zARC_MODE_BINARY) {
        WriteHeaderNumObj();
        const char terminator = 0;
        cbuffer->Write(&terminator, 1);
        cbuffer->SetPosBegin();
        value = reinterpret_cast<const char*>(cbuffer->GetCursor());
    }
}

zCBuffer* zCArchiverBinSafe::GetBuffer()
{
    WriteHeaderNumObj();
    return cbuffer;
}

int zCArchiverBinSafe::OpenWriteBuffer(zCBuffer* buffer, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags, int ownMedium)
{
    Close();
    inSaveGame = saveGame;
    medium = 1;
    mode = archiveMode;
    accessMode = 2;
    owningMedium = ownMedium;
    flags = archiveFlags;
    debugMessagesOn = archiveMode != zARC_MODE_ASCII_PROPS;
    cbuffer = buffer;
    cbuffer->SetMode(WRITE);
    DebugMessage("D: ARC: Writing archive buffer (" + zSTRING(int(mode)) + ") ..");
    WriteHeader(0);
    return 1;
}

void zCArchiverBinSafe::OpenWriteFile(zFILE* targetFile, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags, int ownMedium)
{
    if (!targetFile)
        return;

    Close();
    inSaveGame = saveGame;
    owningMedium = ownMedium;
    mode = archiveMode;
    medium = 0;
    accessMode = 2;
    debugMessagesOn = archiveMode != zARC_MODE_ASCII_PROPS;
    file = targetFile;
    flags = archiveFlags;
    WriteHeader(0);
}

void zCArchiverBinSafe::StoreBuffer(void* data, unsigned long size)
{
    if (size == 0)
        return;
    if (medium == 0)
        file->Write(data, size);
    else
        cbuffer->Write(data, size);
}

void zCArchiverBinSafe::StoreString(const char* text)
{
    const unsigned long length = std::strlen(text);
    if (length > 0)
        WriteType(zARC2_ID_STRING, const_cast<char*>(text), length);
}

unsigned long zCArchiverBinSafe::StoreGetPos()
{
    if (medium != 0)
        return cbuffer->GetPos();
    return file->Pos();
}

void zCArchiverBinSafe::StoreSeek(unsigned long position)
{
    if (medium != 0)
        cbuffer->SetPos(position);
    else
        file->Seek(position);
}

void zCArchiverBinSafe::StoreStringEOL(const char* text)
{
    StoreString(text);
}

void zCArchiverBinSafe::WriteHeader(int saveGame)
{
    int version = 2;
    int value = 0;
    StoreBuffer(&version, 4);
    posNumObjects = StoreGetPos();
    StoreBuffer(&value, 4);
    posChunkList = StoreGetPos();
    StoreBuffer(&value, 4);
    posHeaderEnd = StoreGetPos();
}

int zCArchiverBinSafe::IsStringValid(const char* text)
{
    for (const char* cursor = text; *cursor; ++cursor) {
        if (*cursor == '%' || *cursor == '\xA7' || *cursor == '[' || *cursor == ']')
            return 0;
    }
    return 1;
}

void zCArchiverBinSafe::WriteASCIILine(const char* name, const char* type,
    const zSTRING& value)
{
    if (!IsStringValid(name)) {
        zerr->Fault("D: zArchiver(zCArchiverBinSafe::WriteASCIILine): "
            "string contains illegal characters: \"" + zSTRING(name) + "\" / " + value);
    }

    unsigned long index = ResolveAndPosEntry(name);
    if (index == 0xFFFFFFFF)
        index = InsertChunkInList(name);
    if (index != 0xFFFFFFFF)
        WriteType(zARC2_ID_HASH, &index, 4);
}

void zCArchiverBinSafe::WriteInt(const char* name, int value)
{
    WriteASCIILine(name, "int", zSTRING(value));
    WriteType(zARC2_ID_INTEGER, &value, 4);
}

void zCArchiverBinSafe::WriteByte(const char* name, unsigned char value)
{
    WriteASCIILine(name, "int", zSTRING(int(value)));
    WriteType(zARC2_ID_BYTE, &value, 1);
}

void zCArchiverBinSafe::WriteWord(const char* name, unsigned short value)
{
    WriteASCIILine(name, "int", zSTRING(int(value)));
    WriteType(zARC2_ID_WORD, &value, 2);
}

void zCArchiverBinSafe::WriteFloat(const char* name, float value)
{
    WriteASCIILine(name, "float", zSTRING(value, 8));
    WriteType(zARC2_ID_FLOAT, &value, 4);
}

void zCArchiverBinSafe::WriteBool(const char* name, int value)
{
    WriteASCIILine(name, "bool", zSTRING(value));
    WriteType(zARC2_ID_INT, &value, 4);
}

void zCArchiverBinSafe::WriteString(const char* name, const zSTRING& value)
{
    WriteASCIILine(name, "string", value);
    WriteType(zARC2_ID_STRING, value.ToChar(), value.Length());
}

void zCArchiverBinSafe::WriteVec3(const char* name, const zVEC3& value)
{
    zSTRING text = zSTRING(value.n[0], 8) + " " + zSTRING(value.n[1], 8)
        + " " + zSTRING(value.n[2], 8);
    WriteASCIILine(name, "vec3", text);
    WriteType(zARC2_ID_VEC3, const_cast<zVEC3*>(&value), 12);
}

void zCArchiverBinSafe::WriteColor(const char* name, const zCOLOR& value)
{
    WriteASCIILine(name, "color", value.GetDescription());
    WriteType(zARC2_ID_COLOR, const_cast<zCOLOR*>(&value), 4);
}

void zCArchiverBinSafe::WriteRaw(const char* name, void* data, unsigned long size)
{
    WriteASCIILine(name, "raw", zSTRING(size));
    WriteType(zARC2_ID_RAW, data, size);
}

void zCArchiverBinSafe::WriteRawFloat(const char* name, void* data, unsigned long size)
{
    if (size & 3) {
        zerr->Fault("D: zArchiver(zCArchiverBinSafe::WriteRawFloat): "
            "raw float buffer has illegal size (mod4!=0): " + zSTRING(size));
    }
    WriteASCIILine(name, "rawFloat", zSTRING(""));
    WriteType(zARC2_ID_RAWFLOAT, data, size);
}

void zCArchiverBinSafe::WriteEnum(const char* name, const char* enumName, int value)
{
    zSTRING type = "enum";
    if (InProperties())
        type = "enum;" + zSTRING(enumName);
    WriteASCIILine(name, type.ToChar(), zSTRING(value));
    WriteType(zARC2_ID_ENUM, &value, 4);
}

void zCArchiverBinSafe::WriteGroupBegin(const char* name)
{
}

void zCArchiverBinSafe::WriteGroupEnd(const char* name)
{
}

zCArchiver::zTChunkRecord::zTChunkRecord()
{
}

zCArchiver::zTChunkRecord::~zTChunkRecord()
{
}

void zCArchiverBinSafe::WriteChunkStart(const char* name, const char* className,
    unsigned short version, unsigned long objectIndex)
{
    zTChunkRecord record;
    record.startPos = StoreGetPos();
    record.size = 0;
    record.version = version;
    record.objectIndex = objectIndex;
    record.name = name;
    record.className = className;

    zSTRING text = record.name + " " + record.className + " "
        + zSTRING(int(version)) + " " + zSTRING(objectIndex);
    StoreStringEOL(text.ToChar());
    chunkStack.InsertEnd(record);
}

void zCArchiverBinSafe::WriteChunkStart(const char* name, unsigned short version)
{
    WriteChunkStart(name, "", version, 0);
}

void zCArchiverBinSafe::WriteChunkEnd()
{
    if (chunkStack.numInArray == 0) {
        zerr->Fatal("D: zArchiver(zCArchiverBinSafe::WriteChunkEnd): "
            "write chunk end with no open chunk !");
        return;
    }

    zTChunkRecord record = chunkStack.parray[chunkStack.numInArray - 1];
    record.size = StoreGetPos() - record.startPos;
    --chunkStack.numInArray;
    StoreStringEOL(record.name.ToChar());
}

void zCArchiverBinSafe::WriteObject(const char* name, zCObject& object)
{
    zCClassDef* objectClass = object._GetClassDef();
    if (objectClass->classFlags & 2) {
        WriteObject(name, NULL);
        return;
    }
    if (!objectClass) {
        zerr->Fatal("D: zArchiver(zCArchiverBinSafe::WriteObject): "
            "zCObject without ClassDef, zCLASS_.. macros missing");
        return;
    }

    zSTRING className = objectClass->className;
    for (zCClassDef* baseClass = objectClass->baseClassDef; baseClass;
        baseClass = baseClass->baseClassDef) {
        if (baseClass->createNewInstance)
            className = className + ":" + baseClass->className;
    }

    const unsigned long objectIndex = objectList.numInArray;
    object.AddRef();
    objectList.InsertEnd(&object);
    WriteChunkStart(name, className.ToChar(), objectClass->archiveVersion, objectIndex);
    object.Archive(*this);
    WriteChunkEnd();
}

void zCArchiverBinSafe::WriteObject(const char* name, zCObject* object)
{
    if (object && !(object->_GetClassDef()->classFlags & 2)) {
        for (int i = 0; i < objectList.numInArray; ++i) {
            if (objectList.parray[i] == object) {
                WriteChunkStart(name, "\xA7", 0, i);
                WriteChunkEnd();
                return;
            }
        }
        WriteObject(name, *object);
        return;
    }

    WriteChunkStart(name, "%", 0, 0);
    WriteChunkEnd();
}

void zCArchiverBinSafe::WriteObject(zCObject* object)
{
    WriteObject("", object);
}

int zCArchiverBinSafe::OpenReadBuffer(zCBuffer& buffer, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags)
{
    Close();
    inSaveGame = saveGame;
    medium = 1;
    accessMode = 1;
    owningMedium = 0;
    flags = archiveFlags;
    mode = archiveMode;
    cbuffer = &buffer;
    cbuffer->SetMode(READ);

    if (EndOfArchive()) {
        Close();
        return 0;
    }

    ReadHeader();
    debugMessagesOn = mode != zARC_MODE_ASCII_PROPS;
    DebugMessage("D: ARC: Reading archive buffer (" + zSTRING(int(mode)) + ") ..");
    return 1;
}

void zCArchiverBinSafe::OpenReadFile(zFILE* sourceFile, zTArchiveMode archiveMode,
    int saveGame, int archiveFlags, int ownMedium)
{
    if (!sourceFile)
        return;

    Close();
    inSaveGame = saveGame;
    owningMedium = ownMedium;
    medium = 0;
    accessMode = 1;
    file = sourceFile;
    mode = archiveMode;
    ReadHeader();
    debugMessagesOn = mode != zARC_MODE_ASCII_PROPS;
}

void zCArchiverBinSafe::RestoreBuffer(void* data, unsigned long size)
{
    if (medium != 0)
        cbuffer->Read(data, size);
    else
        file->Read(data, size);
}

void zCArchiverBinSafe::RestoreStringEOL(zSTRING& value)
{
    ReadType(zARC2_ID_STRING, &value, 0);
}

void zCArchiverBinSafe::RestoreString0(zSTRING& value)
{
    RestoreStringEOL(value);
}

unsigned long zCArchiverBinSafe::RestoreGetPos()
{
    if (medium != 0)
        return cbuffer->GetPos();
    return file->Pos();
}

void zCArchiverBinSafe::RestoreSeek(unsigned long position)
{
    if (position == RestoreGetPos())
        return;
    if (medium != 0)
        cbuffer->SetPos(position);
    else
        file->Seek(position);
}

void zCArchiverBinSafe::ReadHeader()
{
    int version;
    RestoreBuffer(&version, 4);
    if (version < 2) {
        zerr->Fatal("OH: ReadHeader - old and defect version of ArchiverBinSafe found. "
            "This version is no longer supported. V" + zSTRING(version));
        return;
    }

    int objectCount;
    RestoreBuffer(&objectCount, 4);
    CheckObjectListSize(objectCount);
    RestoreBuffer(&posChunkList, 4);

    unsigned long position = RestoreGetPos();
    RestoreSeek(posChunkList);

    zCArchiverChunkMap* map = static_cast<zCArchiverChunkMap*>(stringHashMap);
    RestoreBuffer(&map->count, 4);
    const int chunkCount = map->count;

    if (map->table) {
        for (unsigned long i = 0; i < map->tableSize; ++i) {
            zCArchiverChunkEntry* entry = map->table[i];
            while (entry) {
                zCArchiverChunkEntry* next = entry->next;
                delete entry;
                entry = next;
            }
            map->table[i] = NULL;
        }
    }
    map->count = 0;

    for (int i = 0; i < chunkCount; ++i) {
        unsigned short nameSize;
        unsigned short chunkIndex;
        unsigned long hash;
        RestoreBuffer(&nameSize, 2);
        RestoreBuffer(&chunkIndex, 2);
        RestoreBuffer(&hash, 4);
        RestoreBuffer(m_readTextBuffer, nameSize);
        m_readTextBuffer[nameSize] = 0;

        if (!map->table) {
            map->table = new zCArchiverChunkEntry*[map->tableSize];
            std::memset(map->table, 0, map->tableSize * sizeof(zCArchiverChunkEntry*));
        }

        zCArchiverChunkEntry* entry = new zCArchiverChunkEntry;
        entry->hash = hash;
        entry->value = chunkIndex;
        entry->key = new GECString;
        entry->key->string = NULL;
        entry->key->Copy(m_readTextBuffer);
        entry->data = &dummyVal;
        const unsigned long bucket = hash % map->tableSize;
        entry->next = map->table[bucket];
        map->table[bucket] = entry;
        ++map->count;
    }

    RestoreSeek(position);
    posHeaderEnd = RestoreGetPos();
}

void zCArchiverBinSafe::CheckObjectListSize(int size)
{
    const int oldSize = objectList.numInArray;
    if (size <= oldSize)
        return;

    const int newSize = size + size / 2;
    if (objectList.numAlloc < newSize)
        objectList.AllocDelta(newSize - objectList.numAlloc);

    objectList.numInArray = objectList.numAlloc;
    for (int i = oldSize; i < objectList.numInArray; ++i)
        objectList.parray[i] = NULL;
}

zCObject* zCArchiverBinSafe::CreateObject(const zSTRING& className)
{
    zCClassDef* objectClass = GetClassDefByString(className);
    if (!objectClass)
        return NULL;

    zCObject* object = objectClass->CreateNewInstance();
    if (!object) {
        zerr->Fault("D: zArchiver(zCArchiverBinSafe::CreateObject): "
            "could not create object for: " + className);
    }
    return object;
}

zCClassDef* zCArchiverBinSafe::GetClassDefByString(const zSTRING& className)
{
    int word = 1;
    zCClassDef* result;
    zSTRING name;

    do {
        name = className.PickWord(word++, ":", ":");
        result = zCClassDef::GetClassDef(name);
        if (result)
            break;
    } while (name.Length() > 0);

    return result;
}

void zCArchiverBinSafe::ReadChunkStartASCII(const char* name, zSTRING& result)
{
    result.Clear();
    const unsigned long startPosition = RestoreGetPos();
    int searchCycle = noReadSearchCycles;
    zSTRING line;

    do {
        line.Clear();
        RestoreStringEOL(line);
        const char* text = line.ToChar();
        if (text[0] != '[')
            continue;

        if (text[1] == ']') {
            if (searchCycle == 1 || chunkStack.numInArray == 0) {
                RestoreSeek(startPosition);
                break;
            }
            RestoreSeek(chunkStack.parray[chunkStack.numInArray - 1].startPos);
            ++searchCycle;
            continue;
        }

        zSTRING chunkName = line.PickWord(1, "[ ]", "[ ]");
        if (chunkName.IsEmpty() || zSTRING(name).IsEmpty() || chunkName == zSTRING(name)) {
            result = line;
            break;
        }
        SkipChunk(0);
    } while (!EndOfArchive());
}

int zCArchiverBinSafe::ReadChunkStart(const char* name)
{
    zSTRING line;
    ReadChunkStartASCII(name, line);
    if (line.IsEmpty())
        return 0;

    zTChunkRecord record;
    record.startPos = RestoreGetPos();
    record.size = 0;
    record.name = line.PickWord(1, "[ ]", "[ ]");
    record.className = line.PickWord(2, "[ ]", "[ ]");
    record.version = static_cast<unsigned short>(line.PickWord(3, "[ ]", "[ ]").ToInt());
    record.objectIndex = line.PickWord(4, "[ ]", "[ ]").ToInt();
    chunkStack.InsertEnd(record);
    return 1;
}

zCArchiver::zTChunkRecord& zCArchiver::zTChunkRecord::operator=(const zTChunkRecord& other)
{
    startPos = other.startPos;
    size = other.size;
    version = other.version;
    objectIndex = other.objectIndex;
    name = other.name;
    className = other.className;
    return *this;
}

int zCArchiverBinSafe::ReadChunkStart(zSTRING& name, unsigned short& version)
{
    const int result = ReadChunkStart("");
    if (!result) {
        name.Clear();
        return result;
    }

    const zTChunkRecord& record = chunkStack.parray[chunkStack.numInArray - 1];
    name = record.name;
    version = record.version;
    return result;
}

int zCArchiverBinSafe::ReadChunkStartNamed(const char* name, unsigned short& version)
{
    const int result = ReadChunkStart(name);
    if (result)
        version = chunkStack.parray[chunkStack.numInArray - 1].version;
    return result;
}

unsigned short zCArchiverBinSafe::GetCurrentChunkVersion()
{
    if (chunkStack.numInArray > 0)
        return chunkStack.parray[chunkStack.numInArray - 1].version;
    return 0;
}

void zCArchiverBinSafe::ReadChunkEnd()
{
    SkipChunk(1);
}

zCObject* zCArchiverBinSafe::ReadObject(const char* name, zCObject* object)
{
    if (!ReadChunkStart(name))
        return object;

    const zTChunkRecord& record = chunkStack.parray[chunkStack.numInArray - 1];
    const char* className = record.className.ToChar();
    zCObject* result = object;

    if (className[0] == '%') {
        SkipChunk(1);
        return NULL;
    }

    if (className[0] == '\xA7') {
        const unsigned long index = record.objectIndex;
        if (index >= static_cast<unsigned long>(objectList.numInArray)) {
            zerr->Fault("D: zArchiver(::ReadObject): objRef, illegal object index: "
                + zSTRING(index));
            result = object;
        }
        else {
            result = objectList.parray[index];
            if (!result) {
                zerr->Fault("D: zArchiver(::ReadObject): objRef, referenced object has not "
                    "been read, object index: " + zSTRING(index));
                result = object;
            }
            else if (!object) {
                result->AddRef();
            }
            else if (result != object) {
                zerr->Warning("D: zArchiver(::ReadObject): found an objectPtr in file, "
                    "tried to write thisto an 'objectUseThis'");
                result = object;
            }
        }
        SkipChunk(1);
        return result;
    }

    int skip = 0;
    if (!result) {
        result = CreateObject(record.className);
    }
    else {
        zCClassDef* objectClass = GetClassDefByString(record.className);
        if (result->_GetClassDef() != objectClass) {
            zerr->Warning("D: zArchiver(::ReadObject): skipping object-chunk, class from "
                "file does not match class from 'objectUseThis'");
            CheckObjectListSize(record.objectIndex + 1);
            objectList.parray[record.objectIndex] = result;
            result->AddRef();
            skip = 1;
        }
    }

    if (!result || skip) {
        SkipChunk(1);
        return result;
    }

    CheckObjectListSize(record.objectIndex + 1);
    objectList.parray[record.objectIndex] = result;
    result->AddRef();
    result->Unarchive(*this);
    ReadChunkEnd();
    return result;
}

zCObject* zCArchiverBinSafe::ReadObject(zCObject* object)
{
    return ReadObject("", object);
}

void zCArchiverBinSafe::SkipChunk(int onlyCurrent)
{
    if (chunkStack.numInArray <= 0)
        return;

    int depth = 1;
    int linesRead = 0;
    zSTRING line;

    while (depth > 0) {
        line.Clear();
        RestoreStringEOL(line);
        const char* text = line.ToChar();
        if (text[0] == '[') {
            if (text[1] == ']')
                --depth;
            else
                ++depth;
        }
        ++linesRead;
    }

    if (linesRead > 1)
        warnEntrysNotRead = 1;
    if (onlyCurrent)
        --chunkStack.numInArray;
}

int zCArchiverBinSafe::ReadEntryASCII(const char* name, zSTRING& value)
{
    const unsigned long expectedIndex = ResolveAndPosEntry(name);
    if (expectedIndex == 0xFFFFFFFF) {
        zerr->Warning("OH: zCArchiverBinSafe::SkipChunk - expected entry not found: "
            + zSTRING(name));
        return 0;
    }

    unsigned char discarded[12];
    while (!EndOfArchive()) {
        unsigned char type;
        RestoreBuffer(&type, 1);

        if (type == zARC2_ID_HASH) {
            unsigned long index;
            RestoreBuffer(&index, 4);
            if (index == expectedIndex)
                return 1;
            continue;
        }

        unsigned short size;
        switch (type) {
        case zARC2_ID_STRING:
            RestoreBuffer(&size, 2);
            RestoreBuffer(m_readTextBuffer, size);
            m_readTextBuffer[size] = 0;
            break;
        case zARC2_ID_INTEGER:
        case zARC2_ID_FLOAT:
        case zARC2_ID_INT:
        case zARC2_ID_COLOR:
        case zARC2_ID_ENUM:
            RestoreBuffer(discarded, 4);
            break;
        case zARC2_ID_BYTE:
            RestoreBuffer(discarded, 1);
            break;
        case zARC2_ID_WORD:
            RestoreBuffer(discarded, 2);
            break;
        case zARC2_ID_VEC3:
            RestoreBuffer(discarded, 12);
            break;
        case zARC2_ID_RAW:
        case zARC2_ID_RAWFLOAT:
            RestoreBuffer(&size, 2);
            RestoreBuffer(m_readTextBuffer, size);
            break;
        default:
            break;
        }
    }
    return 0;
}

float zCArchiverBinSafe::ReadFloat(const char* name)
{
    float value = 0.0f;
    ReadFloat(name, value);
    return value;
}

zVEC3 zCArchiverBinSafe::ReadVec3(const char* name)
{
    zVEC3 value;
    ReadVec3(name, value);
    return value;
}

int zCArchiverBinSafe::ReadInt(const char* name)
{
    int value = 0;
    ReadInt(name, value);
    return value;
}

unsigned char zCArchiverBinSafe::ReadByte(const char* name)
{
    unsigned char value = 0;
    ReadByte(name, value);
    return value;
}

unsigned short zCArchiverBinSafe::ReadWord(const char* name)
{
    unsigned short value = 0;
    ReadWord(name, value);
    return value;
}

int zCArchiverBinSafe::ReadBool(const char* name)
{
    int value = 0;
    ReadBool(name, value);
    return value;
}

zSTRING zCArchiverBinSafe::ReadString(const char* name)
{
    zSTRING value;
    ReadString(name, value);
    return value;
}

zCOLOR zCArchiverBinSafe::ReadColor(const char* name)
{
    zCOLOR value(0);
    ReadColor(name, value);
    return value;
}

int zCArchiverBinSafe::ReadEnum(const char* name)
{
    int value = 0;
    ReadEnum(name, value);
    return value;
}

void zCArchiverBinSafe::ReadRaw(const char* name, void* data, unsigned long size)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_RAW, data, size);
}

void zCArchiverBinSafe::ReadRawFloat(const char* name, void* data, unsigned long size)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_RAWFLOAT, data, size);
}

void zCArchiverBinSafe::ReadFloat(const char* name, float& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_FLOAT, &value, 4);
}

void zCArchiverBinSafe::ReadVec3(const char* name, zVEC3& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_VEC3, &value, 12);
}

void zCArchiverBinSafe::ReadInt(const char* name, int& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_INTEGER, &value, 4);
}

void zCArchiverBinSafe::ReadByte(const char* name, unsigned char& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_BYTE, &value, 1);
}

void zCArchiverBinSafe::ReadWord(const char* name, unsigned short& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_WORD, &value, 2);
}

void zCArchiverBinSafe::ReadBool(const char* name, int& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_INT, &value, 4);
}

void zCArchiverBinSafe::ReadString(const char* name, zSTRING& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_STRING, &value, 0);
}

void zCArchiverBinSafe::ReadColor(const char* name, zCOLOR& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_COLOR, &value, 4);
}

void zCArchiverBinSafe::ReadEnum(const char* name, int& value)
{
    if (ReadEntryASCII(name, tmpResultValue))
        ReadType(zARC2_ID_ENUM, &value, 4);
}

zCObject* zCArchiverBinSafe::GetParentObject()
{
    for (int i = chunkStack.numInArray - 2; i >= 0; --i) {
        const zTChunkRecord& record = chunkStack.parray[i];
        if (!record.className.IsEmpty() && record.className.ToChar()[0] != '%')
            return objectList.parray[record.objectIndex];
    }
    return NULL;
}

zCArchiverBinSafe::~zCArchiverBinSafe()
{
    ClearChunkList();
    Close();
    file = NULL;
    cbuffer = NULL;
}

void zCArchiverBinSafe::DebugMessage(const zSTRING& message)
{
    if (debugMessagesOn && !(flags & zARC_FLAG_NO_SPY_MESSAGES))
        zerr->Message(message);
}

void zCArchiverBinSafe::WriteHeaderNumObj()
{
    if (accessMode != 2)
        return;
    if (medium == 0 && !file)
        return;

    unsigned long position = StoreGetPos();
    StoreSeek(posNumObjects);
    int count = objectList.numInArray;
    StoreBuffer(&count, 4);
    StoreSeek(position);
}

void zCArchiverBinSafe::Close()
{
    if (accessMode == 0)
        return;

    zCArchiverChunkMap* map = static_cast<zCArchiverChunkMap*>(stringHashMap);

    if (accessMode == 2) {
        unsigned long position = StoreGetPos();
        StoreSeek(posChunkList);
        StoreBuffer(&position, 4);
        StoreSeek(position);
        StoreBuffer(&map->count, 4);

        if (map->table) {
            for (unsigned long i = 0; i < map->tableSize; ++i) {
                for (zCArchiverChunkEntry* entry = map->table[i]; entry; entry = entry->next) {
                    unsigned short nameSize = static_cast<unsigned short>(
                        std::strlen(entry->key->string));
                    unsigned short chunkIndex = static_cast<unsigned short>(entry->value);
                    StoreBuffer(&nameSize, 2);
                    StoreBuffer(&chunkIndex, 2);
                    StoreBuffer(&entry->hash, 4);
                    StoreBuffer(entry->key->string, nameSize);
                }
            }
        }

        WriteHeaderNumObj();

        if (map->table) {
            for (unsigned long i = 0; i < map->tableSize; ++i) {
                zCArchiverChunkEntry* entry = map->table[i];
                while (entry) {
                    zCArchiverChunkEntry* next = entry->next;
                    delete entry;
                    entry = next;
                }
            }
            delete[] map->table;
            map->table = NULL;
        }
        map->count = 0;
    }

    if (owningMedium) {
        DebugMessage("D: ARC: .. Closing (" + zSTRING(objectList.numInArray) + " objects)");
        if (medium != 0) {
            DeleteBuffer();
        }
        else if (file) {
            file->Close();
            delete file;
            file = NULL;
        }
    }

    for (int i = objectList.numInArray - 1; i >= 0; --i) {
        if (objectList.parray[i]) {
            objectList.parray[i]->Release();
            objectList.parray[i] = NULL;
        }
    }
    objectList.numInArray = 0;
    chunkStack.numInArray = 0;
    file = NULL;
    cbuffer = NULL;
    accessMode = 0;
}

GECString::~GECString()
{
    Free();
}

void GECString::Copy(const char* value)
{
    Free();
    string = new char[std::strlen(value) + 1];
    std::strcpy(string, value);
}

zCObject* zCArchiver::ReadObjectAccount(const char* fileName, int line, zCObject* object)
{
    zCObject* result = ReadObject(object);
    zCAccount::Begin();
    zCAccount::Add(result, fileName, line, 1);
    return result;
}

zCObject* zCArchiver::ReadObjectAccount(const char* fileName, int line,
    const char* comment, zCObject* object)
{
    return ReadObjectAccount(fileName, line, object);
}
