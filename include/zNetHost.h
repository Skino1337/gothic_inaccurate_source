#ifndef ZNETHOST_H
#define ZNETHOST_H

#include "zString.h"
#include "zNetTypes.h"
#include "zBuffer.h"

class zCNetHost {
public:

    zCNetHost(void* handle);
    zCNetHost(const zSTRING& host);
    zCNetHost(const zTNetAddress& address);
    virtual ~zCNetHost();
    int InitHost(const zSTRING& host);
    void Init();
    int InitHostForBroadcast();
    void InitHostAddr(zTNetAddress* address);
    void Pack(zCBuffer& buffer);
    void Unpack(zCBuffer& buffer);
    void PackToBuffer(zCBuffer& buffer);
    static zCNetHost* CreateFromBuffer(zCBuffer& buffer);
    zSTRING GetReadableAddr();
    zSTRING GetHostname() const { return hostName; }
    int HasAddress(zTNetAddress& address);

    static unsigned char iter;
    static zCNetHost* localhost;

    unsigned char id;
    zSTRING hostName;
    zTNetAddress hostStreamAddr;
    zTNetAddress hostPacketAddr;
    int valid;
    void* stream_handle;
};

#endif
