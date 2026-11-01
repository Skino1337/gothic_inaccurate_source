#ifndef ZBINKPLAYER_H
#define ZBINKPLAYER_H

#include "zVideoPlayer.h"
#include "zRenderer.h"

class zCView;

class zCBinkPlayer : public zCVideoPlayer {
public:

    zCBinkPlayer();
    virtual ~zCBinkPlayer();
    long GetPixelFormat(zTRndSurfaceDesc& surface);
    void SetFullscreen(int fullscreen, zSTRING backTexture);
    virtual int OpenVideo(zSTRING fileName);
    virtual int CloseVideo();
    virtual int PlayInit(int looping);
    virtual int PlayFrame();
    virtual int PlayDeinit();
    virtual int Pause();
    virtual int Unpause();
    virtual int IsPlaying();
    virtual int ToggleSound();
    virtual float SetSoundVolume(float volume);
    virtual int PlayGotoNextFrame();
    virtual int PlayWaitNextFrame();
    virtual int PlayHandleEvents();

    void* mVideoHandle;
    long mCopyMode;
    short mXpos;
    short mYpos;
    int mDoBlit;
    int mDoHandleEvents;
    int mDoFullscreen;
    zSTRING mBackTexture;
    zCView* mBackView;
    tagRECT mSourceRect;
    tagRECT mTargetRect;
};

#endif
