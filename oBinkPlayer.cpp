#include "include/oBinkPlayer.h"
#include "include/zMoving.h"
#include "include/oGame.h"

extern "C" void __stdcall BinkGoto(void* video, unsigned long frame, unsigned long flags);

static int videoResolutionState = -1;
static zSTRING videoResolution1024 = "1024x768";
static zSTRING videoResolution800 = "800x600";
static zSTRING videoResolution640 = "640x480";

oCBinkPlayer::oCBinkPlayer()
    : extendedKeys(0), disallowInputHandling(0)
{
}

oCBinkPlayer::~oCBinkPlayer()
{
}

int oCBinkPlayer::OpenVideo(zSTRING fileName)
{
    if (zerr->filter_level >= 3)
        zerr->Report(zERR_TYPE_INFO, 0, "B: VP: Start opening video \"" + fileName + "\"",
            3, 1, 0x92, 0, 0);
    fileName.Upper();
    fileName.Delete(".BIK", 0);
    int result = zCBinkPlayer::OpenVideo(fileName + ".BIK");
    if (!result && zrenderer) {
        zTRnd_VidModeInfo modeInfo;
        zrenderer->Vid_GetModeInfo(modeInfo, zrenderer->Vid_GetActiveModeNr());
        const zSTRING resolutions[] = { videoResolution1024, videoResolution800, videoResolution640 };
        const int widths[] = { 1024, 800, 640 };
        const int heights[] = { 768, 600, 480 };
        for (int i = 0; i < 3; ++i) {
            if (zerr->filter_level >= 6)
                zerr->Report(zERR_TYPE_INFO, 0, "B: VP: Checking resolution " + resolutions[i],
                    6, 0, 0x9F, 0, 0);
            if (widths[i] > modeInfo.xdim || heights[i] > modeInfo.ydim)
                continue;
            const zSTRING candidate = fileName + "_" + resolutions[i] + ".BIK";
            if (zerr->filter_level >= 5)
                zerr->Report(zERR_TYPE_INFO, 0, "B: VP: Trying videofile \"" + candidate + "\"",
                    5, 0, 0xA5, 0, 0);
            result = zCBinkPlayer::OpenVideo(candidate);
            if (result)
                break;
        }
    }
    extendedKeys = zoptions->ReadBool(zOPT_SEC_GAME, "extendedVideoKeys", 0);
    return result;
}

int oCBinkPlayer::PlayInit(int looping)
{
    disallowInputHandling = zoptions->ReadBool(zOPT_SEC_GAME, "disallowVideoInput", 0);
    int result = zCBinkPlayer::PlayInit(looping);
    zinput->ProcessInputEvents();
    zinput->ClearKeyBuffer();
    zinput->ResetRepeatKey(1);
    return result;
}

int oCBinkPlayer::PlayDeinit()
{
    return zCBinkPlayer::PlayDeinit();
}

int oCBinkPlayer::PlayFrame()
{
    return zCBinkPlayer::PlayFrame();
}

int oCBinkPlayer::PlayHandleEvents()
{
    if (disallowInputHandling || !mVideoHandle)
        return 0;
    if (zCBinkPlayer::PlayHandleEvents())
        return 1;
    int key = zinput->GetKey(0, 0);
    zinput->ProcessInputEvents();
    int handled = 0;
    if (extendedKeys) {
        if (key == 0x32) {
            ToggleSound();
            handled = 1;
        } else if (key == 0x13) {
            BinkGoto(mVideoHandle, 1, 0);
        } else if (key == 0xc8) {
            float volume = mSoundVolume + 0.05f;
            if (volume > 1.0f)
                volume = 1.0f;
            SetSoundVolume(volume);
            handled = 1;
        } else if (key == 0xcd) {
            unsigned long* videoInfo = (unsigned long*)mVideoHandle;
            const unsigned long frame = videoInfo[3] + 30;
            if (frame < videoInfo[2])
                BinkGoto(mVideoHandle, frame, 1);
        } else if (key == 0xd0) {
            float volume = mSoundVolume - 0.05f;
            if (volume < 0.0f)
                volume = 0.0f;
            SetSoundVolume(volume);
            handled = 1;
        }
    }
    if (key == 1) {
        Stop();
        handled = 1;
    } else if (key == 0x39) {
        if (mPaused)
            Unpause();
        else
            Pause();
        handled = 1;
    }
    return handled;
}
