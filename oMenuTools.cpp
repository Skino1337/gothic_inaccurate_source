#include "include/oGame.h"
#include "include/zMenu.h"
#include "include/zMenuItem.h"
#include "include/zSound.h"
#include "include/zMusic.h"
#include "include/oSysInfo.h"

class zCOptionEntry;

class zCZoneMusic {
public:

    static void SetAutochange(int enabled);
};

int VidIsResolutionValid(int width, int height, int bpp)
{
    if ((bpp != 16 && bpp != 32) || width < 800 || height < 600)
        return 0;
    if (zoptions->ReadBool(zOPT_SEC_INTERNAL, "extendedMenu", 0))
        return 1;
    if (width > 1600 || height > 1200)
        return 0;
    const float aspect = (float)width / height;
    return aspect >= 1.2f && aspect <= 1.6f;
}

static void PlayMenuApplySound()
{
    zCSoundFX* sound = zsound->LoadSoundFXScript("MENU_APPLY");
    if (!sound)
        sound = zsound->LoadSoundFXScript("TORCH_ENLIGHT");
    if (sound) {
        zsound->PlaySound(sound, 0);
        sound->Release();
    }
}

int Apply_Options_Video()
{
    zerr->Message("B: MENUE_APPLY_VIDEO");
    const int selectedMode = zoptions->ReadInt(zOPT_SEC_INTERNAL, ZOPT_VID_RES, 0);
    int width = 800;
    int height = 600;
    int bpp = 16;
    int modeIndex = 0;
    for (int i = 0; i < zrenderer->Vid_GetNumModes(); ++i) {
        zTRnd_VidModeInfo info;
        zrenderer->Vid_GetModeInfo(info, i);
        if (!VidIsResolutionValid(info.xdim, info.ydim, info.bpp))
            continue;
        if (modeIndex == selectedMode) {
            width = info.xdim;
            height = info.ydim;
            bpp = info.bpp;
        }
        ++modeIndex;
    }
    zTRnd_VidModeInfo current;
    zrenderer->Vid_GetModeInfo(current, zrenderer->Vid_GetActiveModeNr());
    if (width != current.xdim || height != current.ydim || bpp != current.bpp) {
        zCView::SetMode(width, height, bpp, NULL);
        zCPosition virtualSize(0x2000, 0x2000);
        zCViewDraw::GetScreen()->SetVirtualSize(virtualSize);
    }
    zoptions->WriteInt(zOPT_SEC_VIDEO, "zVidResFullscreenX", width);
    zoptions->WriteInt(zOPT_SEC_VIDEO, "zVidResFullscreenY", height);
    zoptions->WriteInt(zOPT_SEC_VIDEO, "zVidResFullscreenBPP", bpp);
    return 0;
}

int Apply_Options_Performance()
{
    const int quality = zoptions->ReadInt(zOPT_SEC_INTERNAL, "perfQualityIndex", 0);
    oCSystemInfo systemInfo;
    systemInfo.AnalyseNow();
    systemInfo.ScoreNow();
    if (quality == 1)
        systemInfo.ResetSettings(0.0f);
    else if (quality == 2)
        systemInfo.ResetSettings(0.5f);
    else if (quality == 3)
        systemInfo.ResetSettings(1.0f);
    PlayMenuApplySound();
    return 0;
}

int ChangeMusicEnabled(zCOptionEntry&)
{
    const int enabled = zoptions->ReadBool(zOPT_SEC_SOUND, "musicEnabled", 1);
    zmusic->DisableMusicSystem(!enabled);
    zCZoneMusic::SetAutochange(enabled);
    if (enabled && zCMenu::GetActive())
        zCMenu::GetActive()->RestartMusicTheme();
    zerr->Message(enabled ? "B: MUSIC: Turned ON" : "B: MUSIC: Turned OFF");
    float soundVolume = zoptions->ReadReal(zOPT_SEC_SOUND, ZOPT_SND_SFX_VOL, 1.0f);
    if (soundVolume < 0.0f)
        soundVolume = 0.0f;
    else if (soundVolume > 1.0f)
        soundVolume = 1.0f;
    if (zsound)
        zsound->SetMasterVolume(soundVolume);
    float musicVolume = zoptions->ReadReal(zOPT_SEC_SOUND, ZOPT_SND_MUSIC_VOL, 0.8f);
    if (musicVolume < 0.0f)
        musicVolume = 0.0f;
    else if (musicVolume > 1.0f)
        musicVolume = 1.0f;
    if (zmusic)
        zmusic->SetVolume(musicVolume);
    return 1;
}

int Apply_Options_Audio()
{
    zerr->Message("B: MENUE_APPLY_AUDIO");
    PlayMenuApplySound();
    return 0;
}

int Apply_Options_Game()
{
    zerr->Message("B: MENUE_APPLY_GAME");
    PlayMenuApplySound();
    return 0;
}

int Apply_Options_Controls()
{
    zerr->Message("B: MENUE_APPLY_CONTROLS");
    PlayMenuApplySound();
    return 0;
}

int Update_ChoiceBox()
{
    zerr->Message("B: Update ChoiceBox");
    zSTRING itemName;
    zCMenu::GetParser()->GetParameter(itemName);
    zCMenuItem* item = zCMenuItem::GetByName(itemName);
    if (!item) {
        zerr->Warning(zSTRING("C: Update_CB_Resolution: Box ") + itemName + " unknown");
        return 0;
    }
    if (itemName == "MENUITEM_VID_RESOLUTION_CHOICE") {
        zCArray<zSTRING> choices;
        for (int i = 0; i < zrenderer->Vid_GetNumModes(); ++i) {
            zTRnd_VidModeInfo info;
            zrenderer->Vid_GetModeInfo(info, i);
            if (!VidIsResolutionValid(info.xdim, info.ydim, info.bpp))
                continue;
            zSTRING choice;
            choice.Sprintf("%dx%dx%d", info.xdim, info.ydim, info.bpp);
            choices.InsertEnd(choice);
        }
        item->SetText(choices, 0);
    } else if (itemName == "MENUITEM_VID_DEVICE_CHOICE") {
        zCArray<zSTRING> choices;
        for (int i = 0; i < zrenderer->Vid_GetNumDevices(); ++i) {
            zTRnd_DeviceInfo info;
            zrenderer->Vid_GetDeviceInfo(info, i);
            choices.InsertEnd(info.deviceName);
        }
        item->SetText(choices, 0);
    } else if (itemName == "MENUITEM_AUDIO_PROVIDER_CHOICE" && zsound) {
        zCArray<zSTRING> choices;
        const int providerCount = zsound->GetNumProvider();
        for (int i = 0; i < providerCount; ++i) {
            const zSTRING* providerName = zsound->GetProviderName(i);
            if (providerName)
                choices.InsertEnd(*providerName);
        }
        item->SetText(choices, 0);
    }
    return 0;
}

int DefineMenuScriptFunctions()
{
    zCParser* scriptParser = zCMenu::GetParser();
    scriptParser->DefineExternal("Update_ChoiceBox", Update_ChoiceBox, 3, 0);
    scriptParser->DefineExternal("Apply_Options_Performance", Apply_Options_Performance, 0, 0);
    scriptParser->DefineExternal("Apply_Options_Video", Apply_Options_Video, 0, 0);
    scriptParser->DefineExternal("Apply_Options_Audio", Apply_Options_Audio, 0, 0);
    scriptParser->DefineExternal("Apply_Options_Game", Apply_Options_Game, 0, 0);
    scriptParser->DefineExternal("Apply_Options_Controls", Apply_Options_Controls, 0, 0);
    scriptParser->DefineExternal("PlayVideo", PlayVideo, 3, 2, 0);
    return 1;
}
