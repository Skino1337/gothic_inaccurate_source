#ifndef OBINKPLAYER_H
#define OBINKPLAYER_H

#include "zBinkPlayer.h"

class oCBinkPlayer : public zCBinkPlayer {
public:

    oCBinkPlayer();
    virtual ~oCBinkPlayer();
    virtual int OpenVideo(zSTRING fileName);
    virtual int PlayInit(int looping);
    virtual int PlayFrame();
    virtual int PlayDeinit();
    virtual int PlayHandleEvents();

    int extendedKeys;
    int disallowInputHandling;
};

#endif
