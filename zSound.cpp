#include "include/zSound.h"

static int soundSystemState = -1;
static unsigned int soundGlobalState[3];
static zSTRING soundEnvironmentNames[26] = {
    "GENERIC", "PADDEDCELL", "ROOM", "BATHROOM", "LIVINGROOM", "STONEROOM",
    "AUDITORIUM", "CONCERTHALL", "CAVE", "ARENA", "HANGAR", "CARPETEDHALLWAY",
    "HALLWAY", "STONECORRIDOR", "ALLEY", "FOREST", "CITY", "MOUNTAINS", "QUARRY",
    "PLAIN", "PARKINGLOT", "SEWERPIPE", "UNDERWATER", "DRUGGED", "DIZZY", "PSYCHOTIC"
};

zCSoundSystem::~zCSoundSystem()
{
}

zCSoundFX* zCSoundSystem::LoadSoundFX(const zSTRING& name)
{
    return NULL;
}

zCSoundFX* zCSoundSystem::LoadSoundFXScript(const zSTRING& name)
{
    return NULL;
}

zCParser* zCSoundSystem::GetSFXParser()
{
    return NULL;
}

float zCSoundSystem::GetPlayingTimeMSEC(const zSTRING& name)
{
    return 0.0f;
}

void zCSoundSystem::SetSound3DDefaultRadius(float radius)
{
    defaultRadius = radius;
}

float zCSoundSystem::GetSound3DDefaultRadius() const
{
    return defaultRadius;
}

void zCSoundSystem::SetMasterVolume(float volume)
{
}

float zCSoundSystem::GetMasterVolume()
{
    return 1.0f;
}

void zCSoundSystem::StopSound(const int& handle)
{
}

void zCSoundSystem::StopAllSounds()
{
}

void zCSoundSystem::GetSoundProps(const int& handle, int& volume, float& pitch, float& reverb)
{
}

void zCSoundSystem::UpdateSoundProps(const int& handle, int volume, float pitch, float reverb)
{
}

int zCSoundSystem::IsSoundActive(const int& handle)
{
    return 0;
}

void zCSoundSystem::SetListener(zCVob* vob)
{
}

void zCSoundSystem::SetGlobalReverbPreset(int preset, float weight)
{
}

void zCSoundSystem::SetReverbEnabled(int enabled)
{
}

int zCSoundSystem::GetReverbEnabled()
{
    return 1;
}

int zCSoundSystem::GetNumProvider()
{
    return 1;
}

const zSTRING* zCSoundSystem::GetProviderName(int index)
{
    return NULL;
}

int zCSoundSystem::SetProvider(int provider)
{
    return 1;
}

void zCSoundSystem::SetSpeakerType(zTSpeakerType speaker)
{
}

zCSoundSystem::zTSpeakerType zCSoundSystem::GetSpeakerType()
{
    return zSPEAKER_TYPE_2_SPEAKER;
}

float zCSoundSystem::GetCPULoad()
{
    return -1.0f;
}

static zCClassDef zCSoundFXClassDef("zCSoundFX", "zCResource", NULL, 0, 0x54, 5, "");
zCClassDef* zCSoundFX::classDef = &zCSoundFXClassDef;

void zCSoundFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCSoundFX::_GetClassDef() const
{
    return classDef;
}

zCSoundFX::zCSoundFX()
{
}

zCSoundFX::~zCSoundFX()
{
}

int zCSoundFX::GetNumChannels() const
{
    return 0;
}

int zCSoundFX::GetNumChannelFrames(int channel) const
{
    return 0;
}

void zCSoundFX::GetChannelFrame(int channel, int frame) const
{
}

void zCSoundFX::SetChannelFrame(int channel, int frame)
{
}

float zCSoundFX::GetPlayingTimeMSEC() const
{
    return 0.0f;
}

float zCSoundFX::GetPan() const
{
    return 0.0f;
}

void zCSoundFX::SetPan(float pan)
{
}

float zCSoundFX::GetVolume() const
{
    return 0.0f;
}

void zCSoundFX::SetVolume(float volume)
{
}

int zCSoundFX::GetFrequency() const
{
    return 0;
}

void zCSoundFX::SetFrequency(int frequency)
{
}

void zCSoundFX::SetLooping(int looping)
{
}

int zCSoundFX::IsLooping() const
{
    return 0;
}

void zCSoundFX::SetIsFixed(int fixed)
{
}

int zCSoundFX::GetIsFixed() const
{
    return 0;
}
