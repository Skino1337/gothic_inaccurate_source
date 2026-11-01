#ifndef OGAMEINFO_H
#define OGAMEINFO_H

#include "zGameInfo.h"

class oCGameInfo : public zCGameInfo {
public:

    oCGameInfo();
    virtual ~oCGameInfo();
    virtual void Init();
    virtual void Pack(zCBuffer& buffer, unsigned char flags);
    virtual void Unpack(zCBuffer& buffer);
};

#endif
