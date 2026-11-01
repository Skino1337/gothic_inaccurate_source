#include "include/zBinkPlayer.h"
#include "include/zFile.h"
#include "include/zSession.h"
#include "include/oGame.h"
#include "include/zSndMss.h"

extern "C" {
void* __stdcall BinkOpenMiles();
void __stdcall BinkSetSoundSystem(void* driver, void* openMiles);
void __stdcall BinkSetSoundOnOff(void* video, int enabled);
void* __stdcall BinkOpen(const char* fileName, unsigned long flags);
void __stdcall BinkClose(void* video);
void __stdcall BinkGoto(void* video, unsigned long frame, unsigned long flags);
void __stdcall BinkNextFrame(void* video);
int __stdcall BinkWait(void* video);
void __stdcall BinkDoFrame(void* video);
void __stdcall BinkCopyToBuffer(void* video, void* destination, unsigned long pitch,
    unsigned long height, unsigned long x, unsigned long y, unsigned long flags);
void __stdcall BinkPause(void* video, int pause);
void __stdcall BinkSetVolume(void* video, unsigned long track, unsigned long volume);
}

struct zTBinkInfo {
    unsigned long width;
    unsigned long height;
    unsigned long frames;
    unsigned long currentFrame;
};

zCBinkPlayer::zCBinkPlayer()
    : mVideoHandle(NULL), mCopyMode(-1), mXpos(0), mYpos(0), mDoBlit(1),
      mDoHandleEvents(1), mDoFullscreen(1), mBackView(NULL)
{
}

zCBinkPlayer::~zCBinkPlayer()
{
    if (mVideoHandle)
        CloseVideo();
}

int zCBinkPlayer::OpenVideo(zSTRING fileName)
{
    zFILE_FILE file(fileName);
    if (file.GetExt().IsEmpty())
        file.SetExt("bik");
    if (!file.Exists()) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: VP: File \"" + file.GetFile() + "\" does not exist.",
            0, 0, 0x4A, 0, 0);
        return 0;
    }
    if (zerr->filter_level >= 4)
        zerr->Report(zERR_TYPE_INFO, 0, "B: VP: Open Videofile: \"" + file.GetFile() + "\"",
            4, 1, 0x4E, 0, 0);
    zCSndSys_MSS* soundSystem = dynamic_cast<zCSndSys_MSS*>(zsound);
    if (soundSystem) {
        void* driver = soundSystem->GetDigitalDriverHandle();
        if (driver) {
            BinkSetSoundSystem(driver, BinkOpenMiles);
            BinkSetSoundOnOff(mVideoHandle, 1);
            BinkSetVolume(mVideoHandle, 0, 0x10000);
        }
    }
    mScaleValue = 1.0f;
    mVideoHandle = BinkOpen(file.GetFullPath().ToChar(), 0);
    if (!mVideoHandle) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: VP: Could not open videofile \""
            + file.GetFullPath() + "\"", 0, 0, 0x75, 0, 0);
        return 0;
    }
    return zCVideoPlayer::OpenVideo(fileName);
}

int zCBinkPlayer::CloseVideo()
{
    if (!mVideoHandle)
        return 0;
    BinkClose(mVideoHandle);
    mVideoHandle = NULL;
    return zCVideoPlayer::CloseVideo();
}

int zCBinkPlayer::PlayInit(int looping)
{
    if (!mVideoHandle)
        return 0;
    zCVideoPlayer::PlayInit(looping);
    if (looping > 0)
        BinkGoto(mVideoHandle, looping, 0);
    mCopyMode = -1;
    mXpos = -1;
    mYpos = -1;
    zTRndSurfaceDesc surface;
    if (!zrenderer->Vid_Lock(surface))
    {
        CloseVideo();
        return 0;
    }
    zrenderer->Vid_Unlock();
    mCopyMode = GetPixelFormat(surface);
    if (mCopyMode < 0)
    {
        CloseVideo();
        return 0;
    }
    zTBinkInfo* info = (zTBinkInfo*)mVideoHandle;
    if (info->width > surface.pixelWidth || info->height > surface.pixelHeight) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: VP: Video won't play because of low resolution: "
            + zSTRING(info->width) + "x" + zSTRING(info->height), 0, 0, 0xBE, 0, 0);
        CloseVideo();
        return 0;
    }
    float scaleX = (float)surface.pixelWidth / info->width;
    float scaleY = (float)surface.pixelHeight / info->height;
    mScaleValue = scaleX < scaleY ? scaleX : scaleY;
    if (!zoptions->ReadBool(zOPT_SEC_GAME, "scaleVideos", 1))
        mScaleValue = 1.0f;
    mXpos = (short)((surface.pixelWidth - info->width * mScaleValue) * 0.5f);
    mYpos = (short)((surface.pixelHeight - info->height * mScaleValue) * 0.5f);
    if (mXpos < 0 || mYpos < 0) {
        CloseVideo();
        return 0;
    }
    mSourceRect.left = 0;
    mSourceRect.top = 0;
    mSourceRect.right = info->width + mXpos * 2;
    mSourceRect.bottom = info->height + mYpos * 2;
    mTargetRect.left = 0;
    mTargetRect.top = 0;
    mTargetRect.right = surface.pixelWidth;
    mTargetRect.bottom = surface.pixelHeight;
    if (mDoFullscreen)
    {
        if (mBackTexture.IsEmpty())
            mBackTexture = "default.tga";
        delete mBackView;
        mBackView = new zCView(0, 0, 0x2000, 0x2000, 2);
        mBackView->SetColor(GFX_BLACK);
        mBackView->InsertBack(mBackTexture);
        screen->InsertItem(mBackView, 0);
        screen->Render();
        zrenderer->Vid_Blit(1, &mSourceRect, &mTargetRect);
        screen->Render();
        zrenderer->Vid_Blit(1, &mSourceRect, &mTargetRect);
    }
    return 1;
}

int zCBinkPlayer::PlayDeinit()
{
    zCVideoPlayer::PlayDeinit();
    delete mBackView;
    mBackView = NULL;
    return 1;
}

int zCBinkPlayer::PlayGotoNextFrame()
{
    BinkNextFrame(mVideoHandle);
    return 1;
}

int zCBinkPlayer::PlayWaitNextFrame()
{
    while (IsPlaying() && BinkWait(mVideoHandle))
        PlayHandleEvents();
    return 1;
}

int zCBinkPlayer::PlayFrame()
{
    if (!IsPlaying())
        return 0;
    zCVideoPlayer::PlayFrame();
    PlayHandleEvents();
    if (IsPlaying())
    {
        BinkDoFrame(mVideoHandle);
        PlayGotoNextFrame();
        PlayWaitNextFrame();
        zTRndSurfaceDesc surface;
        if (zrenderer->Vid_Lock(surface))
        {
            if (mCopyMode >= 0)
            {
                zTBinkInfo* info = (zTBinkInfo*)mVideoHandle;
                BinkCopyToBuffer(mVideoHandle, surface.pSurface, surface.bytePitch,
                    surface.pixelHeight, mXpos, mYpos, mCopyMode);
            } else {
                zerr->Report(zERR_TYPE_WARN, 0, "B: VP: Unknown pixel-format", 0, 0, 0x142, 0, 0);
            }
            zrenderer->Vid_Unlock();
            if (mDoBlit)
                zrenderer->Vid_Blit(0, &mSourceRect, &mTargetRect);
        }
    }
    return 1;
}

int zCBinkPlayer::Pause()
{
    BinkPause(mVideoHandle, 1);
    return zCVideoPlayer::Pause();
}

int zCBinkPlayer::Unpause()
{
    BinkPause(mVideoHandle, 0);
    return zCVideoPlayer::Unpause();
}

int zCBinkPlayer::PlayHandleEvents()
{
    return !mDoHandleEvents;
}

int zCBinkPlayer::IsPlaying()
{
    if (!mVideoHandle || !mPlaying)
        return 0;
    zTBinkInfo* info = (zTBinkInfo*)mVideoHandle;
    return mLooping || info->frames > info->currentFrame;
}

long zCBinkPlayer::GetPixelFormat(zTRndSurfaceDesc& surface)
{
    long result = -1;
    if (surface.bitRGBCount == 16)
    {
        if (surface.bitGMask & 0x400)
            result = 10;
        else if (!(surface.bitGMask & 0x400))
            result = 9;
    }
    else if (surface.bitRGBCount == 24)
    {
        if (surface.bitRMask & 1)
            result = 2;
        else if (surface.bitBMask & 1)
            result = 1;
    }
    else if (surface.bitRGBCount == 32)
    {
        if (surface.bitRMask & 1)
            result = 4;
        else if (surface.bitBMask & 1)
            result = 3;
    }
    return result;
}

int zCBinkPlayer::ToggleSound()
{
    mSoundOn = !mSoundOn;
    BinkSetVolume(mVideoHandle, 0, mSoundOn ? (unsigned long)(mSoundVolume * 65536.0f) : 0);
    return mSoundOn;
}

float zCBinkPlayer::SetSoundVolume(float volume)
{
    mSoundVolume = volume;
    BinkSetVolume(mVideoHandle, 0, (unsigned long)(volume * 65536.0f));
    return volume;
}

void zCBinkPlayer::SetFullscreen(int fullscreen, zSTRING backTexture)
{
    mDoFullscreen = fullscreen;
    mBackTexture = backTexture;
}
