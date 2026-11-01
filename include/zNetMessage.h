#ifndef ZNETMESSAGE_H
#define ZNETMESSAGE_H

#include "zQueue.h"
#include "zNetTypes.h"
#include "zBuffer.h"

class zCPlayerInfo;
class zCPlayerGroup;
class zFILE;
class zCBuffer;

class zCNetMessage {
public:

    zCNetMessage(zTNetPacket* packet);
    zCNetMessage(zTNetMessageHeader& header);
    zCNetMessage(const unsigned long& id, unsigned long size);
    virtual ~zCNetMessage();
    void CopyFrom(zCNetMessage* other);
    void Init();
    void GetTarget(zCPlayerInfo*& player, zCPlayerGroup*& group);
    int IsStream();
    int IsFileMessage();
    unsigned long GetSize();
    void SetTimestamp(unsigned char timestamp);
    unsigned char* GetPtr();
    unsigned char GetSenderID();
    void SetStream(int value);
    void Add(void* data, unsigned long size);
    void AddCursorPtr(unsigned long size);
    void ResetCursorPtr();
    unsigned char GetHeaderSize();
    zTNetMessageHeader* GetHeader();
    unsigned long GetDataSize();
    unsigned char* GetCursorPtr();
    zFILE* GetFile(zFILE*& remoteFile);
    void SetFileMessage(zFILE* file, zFILE* remoteFile);
    void Send();
    void SetTarget(zCPlayerInfo* player);
    void SetTarget(zCPlayerGroup* group);
    void Get(void* data, const unsigned long& size);
    zCBuffer* GetBuffer();
    unsigned long GetID();
    unsigned char GetTimestamp();
    void Add(const zSTRING& text);
    void Get(zSTRING& text);
    int IsMultiMessage();
    void LogMessage(zFILE* logFile, const zSTRING& prefix);
    void SetLog(int value);
    unsigned char* GetDataPtr();
    void SetMode(zTBufferMode newMode);
    unsigned long GetRestSize();
    unsigned char GetFlags();
    bool HasError() const;

    zCBuffer* buffer;
    int isStream;
    zFILE* file;
    zFILE* remote_file;
    zCPlayerInfo* targetPlayer;
    zCPlayerGroup* targetGroup;
};

class zCNetMessageQueue {
public:

    zCNetMessageQueue();
    ~zCNetMessageQueue();

    zQUEUE<zCNetMessage*> queue;
};

#endif
