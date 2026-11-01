#include "include/zMusic_Dm.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"

extern zCOption* zgameoptions;
extern zCParser* parserMusic;

zCMusicTheme_DM::~zCMusicTheme_DM()
{
    if (seg)
        ((IUnknown*)seg)->Release();
    seg = NULL;
}

int zCMusicTheme_DM::IsActive() const
{
    return 0;
}

zCMusicSys_DirectMusic::zCMusicSys_DirectMusic()
{
    zoptions->ChangeDir(8);

    if (!parserMusic)
        parserMusic = new zCParser(100);

    zSTRING musicFile;
    if (zgameoptions)
        musicFile = zgameoptions->ReadString("FILES", "Music", "");
    if (musicFile.IsEmpty())
        musicFile = "System\\Music";

    parserMusic->Parse(musicFile + ".src");
    parserMusic->CreatePCode();
    prefs.volume = 0.0f;
    prefs.bitResolution = 0;
    prefs.globalReverbEnabled = 0;
    prefs.sampleRate = 0;
    prefs.numChannels = 0;
    prefs.reverbBufferSize = 0;
    prefs.dScriptEnd = 0;
    actTheme = NULL;
    defTheme = NULL;
}

zCMusicTheme* zCMusicSys_DirectMusic::GetActiveTheme()
{
    return actTheme;
}

void zCMusicSys_DirectMusic::StopJingle(const int& handle)
{
}

int zCMusicSys_DirectMusic::IsJingleActive(const int& handle)
{
    return 0;
}

float zCMusicSys_DirectMusic::GetVolume() const
{
    return prefs.volume;
}

zCMusicSys_DirectMusic::~zCMusicSys_DirectMusic()
{
    delete parserMusic;
    parserMusic = NULL;
    actTheme = NULL;
    defTheme = NULL;
}

void zCMusicSys_DirectMusic::PlayThemeByScript(const zSTRING& theme, int variation, int* defaultVariation)
{
    if (s_musicSystemDisabled)
        return;

    if (defaultVariation)
        *defaultVariation = 1;

    zSTRING upperName = theme;
    upperName.Upper();

    if (actTheme && actTheme->name == upperName)
        return;

    zCMusicTheme* newTheme = LoadThemeByScript(upperName);

    if (!newTheme) {
        if (defaultVariation)
            *defaultVariation = 0;
        return;
    }

    float volume = zMUS_THEME_VOL_DEFAULT;
    zTMus_TransType transition = zMUS_TR_DEFAULT;
    zTMus_TransSubType subType = zMUS_TRSUB_DEFAULT;

    if (variation == 1) {
        transition = zMUS_TR_END;
        subType = zMUS_TRSUB_MEASURE;
    }
    else if (variation == 2) {
        transition = zMUS_TR_BREAK;
        subType = zMUS_TRSUB_IMMEDIATE;
    }

    PlayTheme(newTheme, volume, transition, subType);
}

zCMusicTheme* zCMusicSys_DirectMusic::LoadTheme(const zSTRING& name)
{
    return NULL;
}

zCMusicTheme* zCMusicSys_DirectMusic::LoadThemeByScript(const zSTRING& name)
{
    if (s_musicSystemDisabled)
        return NULL;

    zSTRING upperName = name;
    upperName.Upper();

    if (actTheme && actTheme->name == upperName)
        return actTheme;
    if (!parserMusic->GetSymbol(upperName))
        return NULL;

    zCMusicTheme_DM* theme = new zCMusicTheme_DM();
    parserMusic->CreateInstance(upperName, theme);
    theme->name = upperName;
    theme->seg = NULL;
    return theme;
}

void zCMusicSys_DirectMusic::PlayTheme(zCMusicTheme* theme, const float& volume,
    const zTMus_TransType& transition, const zTMus_TransSubType& subType)
{
    if (s_musicSystemDisabled || !theme)
        return;

    if (!actTheme || !(actTheme->name == theme->name)) {
        zTMus_TransType actualTransition = transition == zMUS_TR_DEFAULT ? theme->trType : transition;
        zTMus_TransSubType actualSubType = subType == zMUS_TRSUB_DEFAULT ? theme->trSubType : subType;
        actTheme = theme;
    }
}

int zCMusicSys_DirectMusic::PlayJingleByScript(const zSTRING& name)
{
    if (s_musicSystemDisabled || !actTheme)
        return 0;

    zCMusicJingle* jingle = LoadJingleByScript(name);
    float volume = zMUS_JINGLE_VOL_DEFAULT;
    zTMus_TransSubType subType = zMUS_TRSUB_DEFAULT;
    return PlayJingle(jingle, volume, subType);
}

void zCMusicSys_DirectMusic::StopJingleByScript(const zSTRING& name)
{
}

zCMusicJingle* zCMusicSys_DirectMusic::LoadJingleByScript(const zSTRING& name)
{
    if (s_musicSystemDisabled || !parserMusic->GetSymbol(name))
        return NULL;

    zCMusicJingle* jingle = new zCMusicJingle();
    parserMusic->CreateInstance(name, jingle);
    jingle->name = name;
    jingle->name.Upper();
    return jingle;
}

zCMusicJingle* zCMusicSys_DirectMusic::LoadJingle(const zSTRING& name)
{
    return NULL;
}

int zCMusicSys_DirectMusic::PlayJingle(const zCMusicJingle* jingle, const float& volume,
    const zTMus_TransSubType& subType)
{
    return 0;
}

void zCMusicSys_DirectMusic::DoMusicUpdate()
{
}

void zCMusicSys_DirectMusic::Stop()
{
    actTheme = NULL;
}

void zCMusicSys_DirectMusic::Mute()
{
    static int muted;
    muted = !muted;

    if (!muted && actTheme) {
        float volume = zMUS_THEME_VOL_DEFAULT;
        zTMus_TransType transition = zMUS_TR_DEFAULT;
        zTMus_TransSubType subType = zMUS_TRSUB_MEASURE;
        PlayTheme(actTheme, volume, transition, subType);
    }
}

void zCMusicSys_DirectMusic::SetVolume(float value)
{
    prefs.volume = value;
}

int zCMusicSys_DirectMusic::IsAvailable(const zSTRING& theme)
{
    return parserMusic->GetSymbol(theme) != NULL;
}
