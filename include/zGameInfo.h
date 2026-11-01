#ifndef ZGAMEINFO_H
#define ZGAMEINFO_H

#include "zString.h"
#include "zBuffer.h"

class zCPlayerInfo;
struct zTNetAddress;

class zCGameInfo {
public:

    zCGameInfo() {}
    virtual ~zCGameInfo() {}
    void PackToBuffer(zCBuffer& buffer, unsigned char flags);
    int GetNumPlayers();
    static zCGameInfo* CreateFromBuffer(zCBuffer& buffer);
    virtual void Init();
    virtual void Reset();
    virtual void SetName(const zSTRING& value);
    virtual zSTRING GetName() const;
    virtual int AddPlayer(zCPlayerInfo* player);
    virtual int RemPlayer(zCPlayerInfo* player);
    virtual zCPlayerInfo* GetPlayerByID(int id);
    virtual zCPlayerInfo* GetPlayerByVobID(unsigned long id);
    virtual zCPlayerInfo* GetPlayerByNetAddress(zTNetAddress& address);
    virtual void Pack(zCBuffer& buffer, unsigned char flags);
    virtual void Unpack(zCBuffer& buffer);

    zSTRING name;
};

#endif
