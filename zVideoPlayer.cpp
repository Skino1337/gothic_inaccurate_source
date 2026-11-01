#include "include/zVideoPlayer.h"
#include "include/zObject.h"

zCVideoPlayer::zCVideoPlayer()
    : mLooping(0), mPaused(0), mPlaying(0), mSoundOn(1), mSoundVolume(1.0f), mScaleValue(1.0f)
{
}

zCVideoPlayer::~zCVideoPlayer()
{
}

zSTRING zCVideoPlayer::GetVideoFilename()
{
    return mVideoFilename;
}

int zCVideoPlayer::IsPlaying()
{
    return mPlaying;
}

int zCVideoPlayer::ToggleSound()
{
    mSoundOn = !mSoundOn;
    return mSoundOn;
}

float zCVideoPlayer::SetSoundVolume(float volume)
{
    mSoundVolume = volume;
    return volume;
}

int zCVideoPlayer::OpenVideo(zSTRING fileName)
{
    if (IsPlaying())
    {
        zerr->Message("B: VP: Can't open a video: Another video is still playing!");
        return 0;
    }
    mVideoFilename = fileName;
    mPaused = 0;
    mPlaying = 0;
    return 1;
}

int zCVideoPlayer::CloseVideo()
{
    if (IsPlaying())
    {
        zerr->Message("B: VP: Can't close video: It's still playing!");
        return 0;
    }
    mPaused = 0;
    mPlaying = 0;
    return 1;
}

int zCVideoPlayer::Play(int looping)
{
    PlayInit(looping);
    while (IsPlaying())
        PlayFrame();
    PlayDeinit();
    return 1;
}

int zCVideoPlayer::PlayInit(int)
{
    mPaused = 0;
    mPlaying = 1;
    return 1;
}

int zCVideoPlayer::PlayGotoNextFrame()
{
    return 1;
}

int zCVideoPlayer::PlayWaitNextFrame()
{
    return 1;
}

int zCVideoPlayer::PlayFrame()
{
    return 0;
}

int zCVideoPlayer::PlayDeinit()
{
    mPaused = 0;
    mPlaying = 0;
    return 1;
}

int zCVideoPlayer::Pause()
{
    if (!mPlaying || mPaused)
        return 0;
    mPaused = 1;
    return 1;
}

int zCVideoPlayer::Unpause()
{
    if (!mPlaying)
        return 0;
    mPaused = 0;
    return 1;
}

int zCVideoPlayer::Stop()
{
    if (!mPlaying)
        return 0;
    mPlaying = 0;
    return 1;
}
