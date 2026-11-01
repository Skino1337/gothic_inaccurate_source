#ifndef ZVIDEOPLAYER_H
#define ZVIDEOPLAYER_H

#include "zString.h"

class zCVideoPlayer {
public:

    zCVideoPlayer();
    virtual ~zCVideoPlayer();
    zSTRING GetVideoFilename();
    int Play(int looping);
    virtual int OpenVideo(zSTRING fileName);
    virtual int CloseVideo();
    virtual int PlayInit(int looping);
    virtual int PlayFrame();
    virtual int PlayDeinit();
    virtual int Pause();
    virtual int Unpause();
    virtual int Stop();
    virtual int IsPlaying();
    virtual int ToggleSound();
    virtual float SetSoundVolume(float volume);
    virtual int PlayGotoNextFrame();
    virtual int PlayWaitNextFrame();

    zSTRING mVideoFilename;
    int mLooping;
    int mPaused;
    int mPlaying;
    int mSoundOn;
    float mSoundVolume;
    float mScaleValue;
};

#endif
