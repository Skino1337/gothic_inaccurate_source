#include "include/zNet.h"
#include "include/zNetMessage.h"
#include "include/oGame.h"
#include "include/oObjFactory.h"
#include "include/zNetHost.h"
#include "include/zObject.h"
#include "include/zFile.h"

void* netListenPacket(zTNetAddress* address);
void* netListenStream(zTNetAddress* address, int (*callback)(void*));
int netClose(void* handle);
int netSend(zTNetAddress* address, void* data, int size, void* handle);
char* netGetLastErrorString();

static void* handle_stream_listen;
static void* handle_dgram_listen;
static int (*connectCallback)(void*);
static unsigned char netTimestamps[255];
static unsigned char netReceivedTimestamps[255];
static int netStatisticsState = -1;
static zSTRING netDefaultBroadcast = "broadcast";
static zSTRING netDefaultLocalhost = "127.0.0.1";

zCNetMessageQueue::zCNetMessageQueue()
{
}

zCNetMessageQueue::~zCNetMessageQueue()
{
    zQUEUE<zCNetMessage*>::zTNode* node = queue.first;

    while (node != queue.last)
    {
        zQUEUE<zCNetMessage*>::zTNode* next = node->next;
        delete node;
        node = next;
    }

    delete node;
    queue.first = NULL;
    queue.last = NULL;
}

zCNet::zCNet(unsigned short protocol)
    : session(new zCNetSession(protocol)), enabled(false), error(0), sendCount(0), recvCount(0)
{
    if (!session->valid)
        zerr->Report(zERR_TYPE_FAULT, 0,
            "B: (zCNet::construct) Session could not be initialized", 0, 0, 111,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    NetReset();
    statistics.startTime = 0;
}

zCNet::~zCNet()
{
    NetReset();
    delete session;
    session = NULL;
}

int zCNet::NetReset()
{
    if (!session)
    {
        zerr->Report(zERR_TYPE_FAULT, 0,
            "B: (zCNet::NetReset) \"session\" is not defined.", 0, 0, 141,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
        return 0;
    }

    session->Reset();
    int (*callback)(void*) = connectCallback;
    connectCallback = NULL;

    if (handle_stream_listen)
    {
        netClose(handle_stream_listen);
        handle_stream_listen = NULL;
        if (callback)
        {
            handle_stream_listen = netListenStream(&session->listenStreamAddress, callback);
            if (handle_stream_listen)
                netClose(handle_stream_listen);
        }
    }

    if (handle_dgram_listen)
        netClose(handle_dgram_listen);

    handle_stream_listen = NULL;
    handle_dgram_listen = NULL;

    if (session->valid)
        handle_dgram_listen = netListenPacket(&session->listenPacketAddress);

    for (int i = 0; i < 255; ++i)
    {
        netTimestamps[i] = 0;
        netReceivedTimestamps[i] = 0;
    }

    return 0;
}

int zCNet::NetReset(int id)
{
    if (id >= 0 && id < 255)
    {
        netTimestamps[id] = 0;
        netReceivedTimestamps[id] = 0;
    }

    return 0;
}

int zCNet::StreamListen(int (*callback)(void*))
{
    connectCallback = session ? callback : NULL;

    if (handle_stream_listen)
    {
        netClose(handle_stream_listen);
        handle_stream_listen = NULL;
    }

    if (connectCallback)
        handle_stream_listen = netListenStream(&session->listenStreamAddress, connectCallback);

    return 0;
}

int zCNet::Send(zCNetMessage*& message)
{
    if (message->IsStream())
        StreamWriteMessage(message);
    else
        DGramWriteMessage(message);

    if (message->IsFileMessage())
        StreamWriteFile(message);

    return 0;
}

unsigned long zCNet::DGramWriteMessage(zCNetMessage*& message)
{
    zCPlayerInfo* player;
    zCPlayerGroup* group;
    message->GetTarget(player, group);
    unsigned long result = 0;

    if (player)
        result = DGramWriteToPlayer(message, player);

    if (group)
    {
        for (zCList<zCPlayerInfo>* entry = group->playerList.next; entry; entry = entry->next)
        {
            result = message->GetSize();
            if (!entry->data->IsActive())
                DGramWriteToPlayer(message, entry->data);
        }
    }

    return result;
}

unsigned long zCNet::DGramWriteToPlayer(zCNetMessage* message, zCPlayerInfo* player)
{
    unsigned char id = player->GetID();
    message->SetTimestamp(netTimestamps[id]);

    if (netReceivedTimestamps[id] > 0)
        ++netTimestamps[id];

    if (netTimestamps[id] > 100)
        netTimestamps[id] = 0;

    zCNetHost* netHost = player->netHost;

    if (netSend(&netHost->hostPacketAddr, message->GetPtr(), message->GetSize(), 0))
        statistics.AddPacketSend(message->GetSize());
    else
        zerr->Report(zERR_TYPE_WARN, 0,
            zSTRING("B: (zCNet::DGramWriteToPlayer) Could not send message: ") +
                netGetLastErrorString(), 0, 0, 258,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);

    return message->GetSize();
}

char* netAddrToString(zTNetAddress* address, int includePort);
zTNetPacket* netRecv(void* handle);
unsigned long netRead(void* handle, void* data, unsigned long size);
void sysEvent();

zSTRING zCNetHost::GetReadableAddr()
{
    return zSTRING(netAddrToString(&hostStreamAddr, 0));
}

unsigned long zCNet::DGramWriteToGroup(zCNetMessage* message, zCPlayerGroup* group)
{
    unsigned long result = 0;

    for (zCList<zCPlayerInfo>* entry = group->playerList.next; entry; entry = entry->next)
    {
        result = message->GetSize();
        if (!entry->data->IsActive())
            DGramWriteToPlayer(message, entry->data);
    }

    return result;
}

int zCNet::DGramRead(zCNetMessage*& message)
{
    delete message;
    message = NULL;
    zTNetPacket* packet = netRecv(handle_dgram_listen);

    if (packet)
    {
        message = new zCNetMessage(packet);
        if (statistics.seconds != 0.0)
        {
            statistics.sent.packets += packet->size;
            statistics.recv.bytes += 1.0;
        }
        CheckTimestamp(message, message->GetSenderID());
    }

    return 0;
}

unsigned long zCNet::StreamRead(zCNetMessage*& message, zCPlayerInfo* player)
{
    message = NULL;
    zTNetMessageHeader header;
    unsigned long read = StreamReadFromPlayer(&header, 11, player, zNET_READ_BLOCK);

    if (read != 11)
    {
        if (read != 0)
            zerr->Report(zERR_TYPE_WARN, 0,
                "B: (zCNet::StreamRead) expected a header, but read more or less data", 0, 0,
                321, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
        return 0;
    }

    message = new zCNetMessage(header);
    message->SetStream(1);
    unsigned long remaining = header.datasize;
    static unsigned char data[20480];

    while (remaining > 0)
    {
        sysEvent();
        unsigned long size = remaining > 20480 ? 20480 : remaining;
        unsigned long received = netRead(player->netHost->stream_handle, data, size);
        message->Add(data, received);
        remaining -= received;
    }

    message->ResetCursorPtr();
    return message->GetSize();
}

unsigned long zCNet::StreamReadFromPlayer(void* data, unsigned long size, zCPlayerInfo* player,
    zTNetStreamReadMode mode)
{
    void* handle = player->netHost->stream_handle;

    if (mode == zNET_READ_STREAM)
        return netRead(handle, data, size);

    unsigned long remaining = size;
    unsigned char* cursor = (unsigned char*)data;

    while (remaining > 0)
    {
        unsigned long received = netRead(handle, cursor, remaining);
        remaining -= received;
        cursor += received;
    }

    return size - remaining;
}

unsigned long netWrite(void* handle, const void* data, unsigned long size, int flush);

unsigned long zCNet::StreamWriteMessage(zCNetMessage*& message)
{
    zCPlayerInfo* player;
    zCPlayerGroup* group;
    message->GetTarget(player, group);
    unsigned long sent = 0;
    unsigned long headerSize = message->GetHeaderSize();
    zTNetMessageHeader* header = message->GetHeader();

    if (player)
    {
        StreamWriteToPlayer(header, headerSize, player);
        sent = headerSize;
    }

    if (group)
    {
        StreamWriteToGroup(header, headerSize, group);
        sent = headerSize;
    }

    message->ResetCursorPtr();
    unsigned long remaining = message->GetDataSize();
    unsigned long dataSize = remaining;

    while (remaining > 0)
    {
        unsigned long size = remaining > 20480 ? 20480 : remaining;
        void* data = message->GetCursorPtr();

        if (player)
        {
            StreamWriteToPlayer(data, size, player);
            sent = size;
        }

        if (group)
        {
            StreamWriteToGroup(data, size, group);
            sent = size;
        }

        sysEvent();
        message->AddCursorPtr(sent);
        remaining -= sent;
    }

    return dataSize - remaining;
}

unsigned long zCNet::StreamWriteToPlayer(void* data, unsigned long size, zCPlayerInfo* player)
{
    netWrite(player->netHost->stream_handle, data, size, 1);
    return size;
}

unsigned long zCNet::StreamWriteToGroup(void* data, unsigned long size, zCPlayerGroup* group)
{
    for (zCList<zCPlayerInfo>* entry = group->playerList.next; entry; entry = entry->next)
    {
        if (!entry->data->IsActive())
            StreamWriteToPlayer(data, size, entry->data);
    }

    return size;
}

unsigned long zCNet::StreamWriteFile(zCNetMessage*& message)
{
    zCPlayerInfo* player;
    zCPlayerGroup* group;
    message->GetTarget(player, group);
    zFILE* remoteFile;
    zFILE* file = message->GetFile(remoteFile);
    file->Open(0);
    unsigned long remaining = file->Size();
    unsigned long fileSize = remaining;
    zSTRING path = file->GetFilename();
    char fileHeader[264] = { 0 };
    *(unsigned long*)fileHeader = fileSize;
    const char* fileName = path.ToChar();
    int i = 0;

    do
    {
        fileHeader[i + 4] = fileName[i];
        ++i;
    } while (fileName[i - 1]);

    if (player)
        StreamWriteToPlayer(fileHeader, sizeof(fileHeader), player);
    if (group)
        StreamWriteToGroup(fileHeader, sizeof(fileHeader), group);

    unsigned long checksum;
    zCChecksum::BeginChecksum(&checksum);
    static unsigned char data[20480];

    while (remaining > 0)
    {
        sysEvent();
        unsigned long received = file->Read(data, 20480);
        zCChecksum::CalcBufferChecksum(&checksum, data, received);

        if (received > 0)
        {
            if (player)
                StreamWriteToPlayer(data, received, player);
            if (group)
                StreamWriteToGroup(data, received, group);
        }

        remaining -= received;
        if (received != 20480)
            break;
    }

    checksum = zCChecksum::EndChecksum(&checksum);

    if (player)
        StreamWriteToPlayer(&checksum, sizeof(checksum), player);
    if (group)
        StreamWriteToGroup(&checksum, sizeof(checksum), group);

    file->Close();
    return fileSize - remaining;
}

unsigned long zCNet::StreamReadFile(zCNetMessage* message, zCPlayerInfo* player)
{
    char fileHeader[264];
    StreamReadFromPlayer(fileHeader, sizeof(fileHeader), player, zNET_READ_BLOCK);
    unsigned long remaining = *(unsigned long*)fileHeader;
    zoptions->ChangeDir(3);
    zFILE* remoteFile = new zFILE_FILE(zSTRING(fileHeader + 4));
    zFILE* tempFile = new zFILE_FILE("temp.dat");
    message->SetFileMessage(remoteFile, tempFile);
    unsigned long checksum;
    zCChecksum::BeginChecksum(&checksum);
    tempFile->Create();
    static unsigned char data[20480];

    while (remaining > 0)
    {
        sysEvent();
        unsigned long size = remaining > 20480 ? 20480 : remaining;
        unsigned long received = netRead(player->netHost->stream_handle, data, size);

        if (received > 0)
        {
            zCChecksum::CalcBufferChecksum(&checksum, data, received);
            tempFile->Write(data, received);
            remaining -= received;
        }

        if (received == 0)
            break;
    }

    tempFile->Close();
    unsigned long receivedChecksum;
    StreamReadFromPlayer(&receivedChecksum, sizeof(receivedChecksum), player, zNET_READ_BLOCK);
    return 0;
}

zCNetMessageQueue recvQueue[4];
zCNetMessageQueue sendQueue[4];
zCNetMessage* msgPtr;
int lockQueues;

int zCNet::MsgGetFromStream(zCPlayerInfo* player, zCNetMessage*& message)
{
    message = NULL;

    if (!player->netHost)
    {
        zerr->Report(zERR_TYPE_FATAL, 0,
            "B: (zCNet::MsgGetFromStream) Player has not network-host-data", -1, 0, 614,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
        return 0;
    }

    int result = StreamRead(message, player);
    if (message->IsFileMessage())
        result += StreamReadFile(message, player);
    return result;
}

int zCNet::MsgPut(zCNetMessage* message)
{
    if (!enabled)
        return 0;

    if (!message)
    {
        zerr->Report(zERR_TYPE_WARN, 0,
            "B: (zCNet::MsgPut) There is no message to \"put\"", 0, 0, 640,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
        return 0;
    }

    while (lockQueues)
    {
    }

    Send(message);
    sendCount = 0;
    delete message;
    return sendCount;
}

int zCNet::MsgGet(zCNetMessage*& message, unsigned char priority)
{
    message = NULL;

    if (enabled)
    {
        while (lockQueues)
        {
        }

        message = recvQueue[priority].queue.Pop();
        if (message)
            --recvCount;
    }

    return 0;
}

int zCNet::MsgSendFromQueue()
{
    if (sendCount)
    {
        sendCount = 0;
        msgPtr = sendQueue[0].queue.Pop();

        if (msgPtr)
        {
            sendCount = Send(msgPtr);
            if (sendCount)
                return 6006;
            --sendCount;
            delete msgPtr;
            msgPtr = NULL;
            return 0;
        }
    }

    return 6004;
}

int zCNet::MsgRecvToQueue()
{
    msgPtr = NULL;
    sendCount = DGramRead(msgPtr);
    if (sendCount)
        return 6005;
    if (!msgPtr)
        return 6004;

    recvQueue[0].queue.Push(msgPtr);
    ++recvCount;
    return sendCount;
}

int zCNet::HandleReceiving()
{
    int result;

    do
    {
        result = MsgRecvToQueue();
    } while (!result);

    return result;
}

int zCNet::HandleSending()
{
    int result;

    do
    {
        result = MsgSendFromQueue();
    } while (!result);

    if (result != 6004)
        zerr->Report(zERR_TYPE_FAULT, 0,
            "B: (zCNetHandle) sending messages causes an error", 0, 0, 770,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);

    return result;
}

void zCNet::Handle()
{
    lockQueues = 1;
    int result = HandleReceiving();
    lockQueues = 0;

    if (result != 6004)
        zerr->Report(zERR_TYPE_FAULT, 0,
            "B: (zCNetHandle) receiving message causes an error", 0, 0, 808,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
}

void* netConnect(zTNetAddress* address);
char* netGetLocalHost();
zTNetAddress* netGetHostAddr(char* hostName);

int zCNet::StreamConnect(zCNetHost*& host)
{
    host->stream_handle = netConnect(&host->hostStreamAddr);
    return host->stream_handle ? 0 : 6001;
}

int zCNet::StreamDisconnect(zCNetHost*& host)
{
    netClose(host->stream_handle);
    host->stream_handle = NULL;
    host->valid = 0;
    return 0;
}

void zCNet::LockQueues(int lock)
{
    lockQueues = lock;
}

int zCNet::WaitForStreamMessage(zCPlayerInfo* player, zCNetMessage*& message)
{
    message = NULL;

    do
    {
        sysEvent();
        lockQueues = 1;
        int result = HandleReceiving();
        lockQueues = 0;

        if (result != 6004)
            zerr->Report(zERR_TYPE_FAULT, 0,
                "B: (zCNetHandle) receiving message causes an error", 0, 0, 808,
                "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);

        MsgGetFromStream(player, message);
    } while (!message);

    return 1;
}

zTNetAddress* zCNet::GetLocalAddress()
{
    return netGetHostAddr(netGetLocalHost());
}

void zCNet::EnableNetwork(bool value)
{
    enabled = value;
}

void zCNet::ApplyTimestamp(zCNetMessage* message, unsigned char id)
{
    message->SetTimestamp(netTimestamps[id]);
    if (id > 0)
        ++netTimestamps[id];
    if (netTimestamps[id] > 100)
        netTimestamps[id] = 0;
}

int zCNet::CheckTimestamp(zCNetMessage*, unsigned char)
{
    return 0;
}

#include <time.h>

zCNetStatistics::zCNetStatistics()
    : seconds(0.0), started(0), startTime(0)
{
    sent.bytes = 0.0;
    sent.packets = 0.0;
    recv.bytes = 0.0;
    recv.packets = 0.0;
    lostRecv.bytes = 0.0;
    lostRecv.packets = 0.0;
    lostSent.bytes = 0.0;
    lostSent.packets = 0.0;
}

zCNetStatistics::~zCNetStatistics()
{
}

void zCNetStatistics::AddLostRecv(unsigned int packets)
{
    lostRecv.packets -= packets;
}

void zCNetStatistics::AddLostSend(unsigned int packets)
{
    lostSent.packets += packets;
}

void zCNetStatistics::AddPacketSend(double size)
{
    if (started)
    {
        sent.bytes += size;
        sent.packets += 1.0;
    }
}

void zCNetStatistics::AddPacketRecv(double size)
{
    if (started)
    {
        recv.bytes += size;
        recv.packets += 1.0;
    }
}

void zCNetStatistics::Start()
{
    sent.bytes = 0.0;
    sent.packets = 0.0;
    recv.bytes = 0.0;
    recv.packets = 0.0;
    lostRecv.bytes = 0.0;
    lostRecv.packets = 0.0;
    lostSent.bytes = 0.0;
    lostSent.packets = 0.0;
    seconds = 0.0;
    started = 1;
    startTime = time(NULL);
    zCBuffer::ByteFreq_Reset();
    zerr->Report(zERR_TYPE_INFO, 0, "B: ZNET: Network-Statistics: Started", 3, 0, 972,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
}

void zCNetStatistics::Stop()
{
    seconds = difftime(time(NULL), startTime);
    started = 0;
    zerr->Report(zERR_TYPE_INFO, 0, "B: ZNET: Network-Statistics: Stopped", 3, 0, 981,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
}

void zCNetStatistics::Print()
{
    seconds = difftime(time(NULL), startTime);
    zerr->Report(zERR_TYPE_INFO, 0,
        started ? "B: zNET: Network-Statistics Snapshot" : "B: zNET: Network-Statistics", 1, 1,
        1003, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0, "B: zNET: Time recorded..: "
        + zSTRING((int)(seconds / 60.0)) + ":" + zSTRING((int)seconds % 60).Align(zSTR_RIGHT, 2, '0'),
        1, 1, 1009, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: zNET: Kilobytes .....: " + zSTRING((int)(sent.bytes / 1024.0)) + " sent, " +
            zSTRING((int)(recv.bytes / 1024.0)) + " received.", 1, 1, 1010,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: zNET: Packets........: " + zSTRING((int)sent.packets) + " sent, " +
            zSTRING((int)recv.packets) + " received.", 1, 1, 1011,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: zNET: Bytes/sec......: " + zSTRING((int)(sent.bytes / seconds)) + " sent, " +
            zSTRING((int)(recv.bytes / seconds)) + " received.", 1, 1, 1012,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: zNET: Packets/min....: " + zSTRING((int)(sent.packets * 60.0 / seconds)) + " sent, " +
            zSTRING((int)(recv.packets * 60.0 / seconds)) + " received.", 1, 1, 1013,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: zNET: Bytes/Packet...: " + zSTRING((int)(sent.bytes / sent.packets)) + " sent, " +
            zSTRING((int)(recv.bytes / recv.packets)) + " received.", 1, 1, 1014,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0, "B: zNET: Bytes lost.....: not yet implemented",
        1, 1, 1015, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: zNET: Packets lost...: " + zSTRING((int)lostRecv.packets) + " recv",
        1, 1, 1016, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet.cpp", NULL);
    zCBuffer::ByteFreq_Print();
}

void zCNetStatistics::Reset()
{
    Stop();
    Start();
}

double zCNetStatistics::GetSeconds()
{
    seconds = difftime(time(NULL), startTime);
    return seconds;
}
