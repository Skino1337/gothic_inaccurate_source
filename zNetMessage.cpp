#include "include/zNetMessage.h"
#include "include/zNet.h"
#include "include/oObjFactory.h"
#include "include/zFile.h"
#include "include/zObject.h"

extern zCNet* znet;

static int netMessageState = -1;
static zSTRING netMessageBroadcast = "broadcast";
static zSTRING netMessageLocalhost = "127.0.0.1";
static int netMessageRuntimeState = -1;
static zSTRING netMessageDefaultBroadcast = "broadcast";
static zSTRING netMessageDefaultLocalhost = "127.0.0.1";

zCNetMessage::zCNetMessage(zTNetMessageHeader& header)
{
    Init();
    buffer = new zCBuffer(header.datasize + 31);
    buffer->SetMode(READWRITE);
    buffer->Write(&header, 11);
}

zCNetMessage::zCNetMessage(const unsigned long& id, unsigned long size)
{
    Init();
    zTNetMessageHeader header = {};
    header.id = id;
    if (zCPlayerInfo::GetActivePlayer())
        header.senderID = zCPlayerInfo::GetActivePlayer()->GetID();
    header.flags = 2;
    buffer = new zCBuffer(size + 31);
    buffer->SetMode(READWRITE);
    buffer->Write(&header, 11);
}

void zCNetMessage::CopyFrom(zCNetMessage* other)
{
    unsigned char* data = other->buffer->GetBuffer();
    buffer->Write(data + 11, other->buffer->GetSize() - 11);
    buffer->SetMode(READWRITE);
    isStream = other->isStream;
    targetGroup = NULL;
    targetPlayer = NULL;
}

zCNetMessage::zCNetMessage(zTNetPacket* packet)
{
    Init();

    if (!znet || !znet->session)
    {
        zerr->Report(zERR_TYPE_FAULT, 0, "B: (zCNetMessage) No session-data available!", 0, 0,
            134, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetMessage.cpp", NULL);
        return;
    }

    buffer = new zCBuffer(packet->size);
    buffer->SetMode(READWRITE);
    buffer->Write(packet->Data, packet->size);
    buffer->SetPos(11);
}

void zCNetMessage::Init()
{
    buffer = NULL;
    file = NULL;
    remote_file = NULL;
    targetPlayer = NULL;
    targetGroup = NULL;
    isStream = 0;
}

zCNetMessage::~zCNetMessage()
{
    delete buffer;
    buffer = NULL;
    targetPlayer = NULL;
    targetGroup = NULL;

    if (file)
    {
        file->Close();
        delete file;
        file = NULL;
    }

    if (remote_file)
    {
        remote_file->Close();
        delete remote_file;
        remote_file = NULL;
    }
}

void zCNetMessage::Send()
{
    buffer->ByteFreq_Analyse();

    if (buffer->HasError())
    {
        zerr->Report(zERR_TYPE_WARN, 0,
            "B: (zCNetMessage::Send) Message is illegal. Did not sent it!", 0, 0, 183,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetMessage.cpp", NULL);
        return;
    }

    if (file)
        isStream = 1;
    GetHeader()->datasize = buffer->GetSize() - 11;
    zCNetMessage* message = this;
    znet->MsgPut(message);
}

void zCNetMessage::SetTimestamp(unsigned char timestamp)
{
    GetHeader()->timestamp = timestamp;
}

unsigned char zCNetMessage::GetTimestamp()
{
    return GetHeader()->timestamp;
}

void zCNetMessage::SetTarget(zCPlayerInfo* player)
{
    GetHeader()->flags &= ~4;
    targetPlayer = player;
}

void zCNetMessage::SetTarget(zCPlayerGroup* group)
{
    GetHeader()->flags |= 4;
    targetGroup = group;
}

void zCNetMessage::GetTarget(zCPlayerInfo*& player, zCPlayerGroup*& group)
{
    player = targetPlayer;
    group = targetGroup;
}

unsigned long zCNetMessage::Get(void* data, const unsigned long& size)
{
    unsigned long readSize = size;
    if (readSize > buffer->GetRestSize())
    {
        readSize = buffer->GetRestSize();
        zerr->Report(zERR_TYPE_WARN, 0,
            "B: (zCNetMessage::Get) Requested data is bigger than message", 0, 0, 235,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetMessage.cpp", NULL);
    }
    buffer->Read(data, readSize);
    return readSize;
}

void zCNetMessage::Add(void* data, unsigned long size)
{
    if (size > buffer->GetRestSize())
    {
        size = buffer->GetRestSize();
        zerr->Report(zERR_TYPE_WARN, 0, "B: (zCNetMessage::Add) Data is too long for message",
            0, 0, 248, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetMessage.cpp", NULL);
    }
    if (size > 0 && data)
        buffer->Write(data, size);
}

void zCNetMessage::AddCursorPtr(unsigned long size)
{
    buffer->SetPosRel(size);
}

void zCNetMessage::Add(const zSTRING& text)
{
    buffer->WriteString(text);
}

void zCNetMessage::Get(zSTRING& text)
{
    text = buffer->ReadString();
}

int zCNetMessage::IsStream()
{
    return isStream;
}

void zCNetMessage::SetStream(int value)
{
    isStream = value;
}

void zCNetMessage::SetFileMessage(zFILE* newFile, zFILE* newRemoteFile)
{
    if (file)
    {
        file->Close();
        delete file;
        file = NULL;
    }
    delete remote_file;
    remote_file = NULL;

    if (!newFile)
    {
        GetHeader()->flags &= ~1;
        return;
    }

    GetHeader()->flags |= 1;
    file = newFile;
    remote_file = newRemoteFile;
}

zFILE* zCNetMessage::GetFile(zFILE*& remoteFile)
{
    if (GetHeader()->flags & 1)
    {
        remoteFile = remote_file;
        return file;
    }
    remoteFile = NULL;
    return NULL;
}

int zCNetMessage::IsFileMessage()
{
    return (GetHeader()->flags & 1) != 0;
}

int zCNetMessage::IsMultiMessage()
{
    return (GetHeader()->flags & 4) != 0;
}

void zCNetMessage::LogMessage(zFILE* logFile, const zSTRING& prefix)
{
    if (GetHeader()->flags & 2)
    {
        zSTRING text = prefix + (isStream ? "<STREAM> " : "<DGRAM>  ") +
            zSTRING((int)GetID()).Align(zSTR_RIGHT, 6, ' ') + " from " +
            zSTRING((int)GetSenderID()).Align(zSTR_RIGHT, 3, ' ') + " (" +
            zSTRING((int)GetSize()).Align(zSTR_RIGHT, 5, ' ') + " bytes)\n";
        logFile->Write(text);
    }
}

void zCNetMessage::SetLog(int)
{
}

zTNetMessageHeader* zCNetMessage::GetHeader()
{
    return (zTNetMessageHeader*)buffer->GetBuffer();
}

unsigned char zCNetMessage::GetHeaderSize()
{
    return 11;
}

zCBuffer* zCNetMessage::GetBuffer()
{
    return buffer;
}

unsigned char* zCNetMessage::GetCursorPtr()
{
    return buffer->GetCursor();
}

unsigned char* zCNetMessage::GetPtr()
{
    return buffer->GetBuffer();
}

unsigned char* zCNetMessage::GetDataPtr()
{
    return buffer->GetBuffer() + 11;
}

void zCNetMessage::ResetCursorPtr()
{
    buffer->SetPos(11);
}

void zCNetMessage::SetMode(zTBufferMode newMode)
{
    buffer->SetMode(newMode);
}

unsigned long zCNetMessage::GetSize()
{
    return buffer->GetSize();
}

unsigned long zCNetMessage::GetDataSize()
{
    return buffer->GetSize() - 11;
}

unsigned long zCNetMessage::GetRestSize()
{
    return buffer->GetRestSize();
}

unsigned long zCNetMessage::GetID()
{
    return GetHeader()->id;
}

unsigned char zCNetMessage::GetSenderID()
{
    return GetHeader()->senderID;
}

unsigned char zCNetMessage::GetFlags()
{
    return GetHeader()->flags;
}

bool zCNetMessage::HasError() const
{
    return buffer->HasError();
}
