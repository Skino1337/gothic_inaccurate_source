#ifndef ZNET_H
#define ZNET_H

#include "zNetSession.h"

class zCNetMessage;
class zCPlayerInfo;
class zCPlayerGroup;

struct zTNetStatistics {
    double bytes;
    double packets;
};

class zCNetStatistics {
public:

    zCNetStatistics();
    virtual ~zCNetStatistics();
    void AddLostRecv(unsigned int packets);
    void AddLostSend(unsigned int packets);
    void AddPacketSend(double size);
    void AddPacketRecv(double size);
    void Start();
    void Stop();
    void Print();
    void Reset();
    double GetSeconds();

    zTNetStatistics sent;
    zTNetStatistics recv;
    zTNetStatistics lostRecv;
    zTNetStatistics lostSent;
    double seconds;
    int started;
    long startTime;
};

class zCNet {
public:

    zCNet(unsigned short protocol);
    virtual ~zCNet();
    int NetReset();
    int NetReset(int id);
    int StreamListen(int (*callback)(void*));
    int Send(zCNetMessage*& message);
    unsigned long DGramWriteMessage(zCNetMessage*& message);
    unsigned long DGramWriteToPlayer(zCNetMessage* message, zCPlayerInfo* player);
    unsigned long DGramWriteToGroup(zCNetMessage* message, zCPlayerGroup* group);
    int DGramRead(zCNetMessage*& message);
    unsigned long StreamRead(zCNetMessage*& message, zCPlayerInfo* player);
    unsigned long StreamReadFromPlayer(void* data, unsigned long size, zCPlayerInfo* player,
        zTNetStreamReadMode mode);
    int CheckTimestamp(zCNetMessage* message, unsigned char senderID);
    unsigned long StreamWriteMessage(zCNetMessage*& message);
    unsigned long StreamWriteToPlayer(void* data, unsigned long size, zCPlayerInfo* player);
    unsigned long StreamWriteToGroup(void* data, unsigned long size, zCPlayerGroup* group);
    unsigned long StreamWriteFile(zCNetMessage*& message);
    unsigned long StreamReadFile(zCNetMessage* message, zCPlayerInfo* player);
    int MsgGetFromStream(zCPlayerInfo* player, zCNetMessage*& message);
    int MsgPut(zCNetMessage* message);
    int MsgGet(zCNetMessage*& message, unsigned char priority);
    int MsgSendFromQueue();
    int MsgRecvToQueue();
    int HandleReceiving();
    int HandleSending();
    void Handle();
    int StreamConnect(zCNetHost*& host);
    int StreamDisconnect(zCNetHost*& host);
    void LockQueues(int lock);
    int WaitForStreamMessage(zCPlayerInfo* player, zCNetMessage*& message);
    zTNetAddress* GetLocalAddress();
    void EnableNetwork(bool value);
    void ApplyTimestamp(zCNetMessage* message, unsigned char id);

    zCNetSession* session;
    bool enabled;
    zCNetStatistics statistics;
    int error;
    int sendCount;
    int recvCount;
};

#endif
