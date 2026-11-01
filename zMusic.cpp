#include "include/zMusic.h"

static int musicSystemState = -1;
static zSTRING musicSysConfig = "musicSysConfig";
zCMusicSystem* zmusic;

zCMusicTheme* zCMusicSys_Dummy::LoadThemeByScript(const zSTRING&)
{
    return new zCMusicTheme;
}

zCMusicTheme* zCMusicSys_Dummy::LoadTheme(const zSTRING&)
{
    return new zCMusicTheme;
}

zCMusicJingle* zCMusicSys_Dummy::LoadJingleByScript(const zSTRING&)
{
    return new zCMusicJingle;
}

zCMusicJingle* zCMusicSys_Dummy::LoadJingle(const zSTRING&)
{
    return new zCMusicJingle;
}

void zCMusicSys_Dummy::DoMusicUpdate()
{
}

int zCMusicSystem::s_musicSystemDisabled;

void zCMusicSystem::DisableMusicSystem(int disabled)
{
    s_musicSystemDisabled = disabled;

    if (disabled && zmusic)
        zmusic->Stop();
}

zCMusicTheme::zCMusicTheme()
{
}

zCMusicTheme::~zCMusicTheme()
{
}

int zCMusicTheme::IsActive() const
{
    return 0;
}

zCMusicJingle::zCMusicJingle()
{
    refCtr = 1;
}

zCMusicJingle::~zCMusicJingle()
{
}

zCMusicJingle* zCMusicJingle::AddRef()
{
    refCtr++;
    return this;
}

int zCMusicJingle::Release()
{
    delete this;
    return 0;
}

int zCMusicJingle::IsPlaying() const
{
    return 0;
}

void zCMusicJingle::StopPlaying()
{
}

void zCMusicJingle::CacheIn()
{
}

void zCMusicJingle::CacheOut()
{
}

zCMusicSystem::~zCMusicSystem()
{
}

void zCMusicSystem::PlayThemeByScript(const zSTRING& theme, int variation, int* defaultVariation)
{
}

zCMusicTheme* zCMusicSystem::LoadThemeByScript(const zSTRING& name)
{
    return NULL;
}

zCMusicTheme* zCMusicSystem::LoadTheme(const zSTRING& name)
{
    return NULL;
}

void zCMusicSystem::PlayTheme(zCMusicTheme* theme, const float& volume,
    const zTMus_TransType& transition, const zTMus_TransSubType& subType)
{
}

zCMusicTheme* zCMusicSystem::GetActiveTheme()
{
    return NULL;
}

int zCMusicSystem::PlayJingleByScript(const zSTRING& name)
{
    return 0;
}

void zCMusicSystem::StopJingleByScript(const zSTRING& name)
{
}

zCMusicJingle* zCMusicSystem::LoadJingleByScript(const zSTRING& name)
{
    return NULL;
}

zCMusicJingle* zCMusicSystem::LoadJingle(const zSTRING& name)
{
    return NULL;
}

int zCMusicSystem::PlayJingle(const zCMusicJingle* jingle, const float& volume,
    const zTMus_TransSubType& subType)
{
    return 0;
}

void zCMusicSystem::StopJingle(const int& handle)
{
}

int zCMusicSystem::IsJingleActive(const int& handle)
{
    return 0;
}

void zCMusicSystem::AddEventCallBackFunc(const zTMus_EventType& event,
    void(*callback)(const zTMus_EventType&))
{
}

void zCMusicSystem::RemoveEventCallBackFunc(void(*callback)(const zTMus_EventType&))
{
}

void zCMusicSystem::Mute()
{
}

void zCMusicSystem::Stop()
{
}

void zCMusicSystem::SetVolume(float value)
{
    volume = value;
}

float zCMusicSystem::GetVolume() const
{
    return volume;
}

int zCMusicSystem::IsAvailable(const zSTRING& theme)
{
    return 0;
}
