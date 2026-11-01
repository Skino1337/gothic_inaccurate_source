#ifndef ZNETSESSION_H
#define ZNETSESSION_H

#include "zNetTypes.h"

class zCNetSession {
public:

    zCNetSession(unsigned short protocol);
    virtual ~zCNetSession();
    void Reset();

    unsigned short protocol;
    zTNetAddress listenPacketAddress;
    zTNetAddress listenStreamAddress;
    int valid;
};

#endif
