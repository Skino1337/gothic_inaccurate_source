#ifndef ZSOUND_H
#define ZSOUND_H

#include "zResource.h"

const int zSND_FREQ_DEFAULT = -1;
const float zSND_PAN_DEFAULT = -2.0f;
const float zSND_PAN_LEFT = -1.0f;
const float zSND_PAN_CENTER = 0.0f;
const float zSND_PAN_RIGHT = 1.0f;
const float zSND_PAN_SURROUND = 100.0f;
const int zSND_SLOT_NONE = 0;
const int zSND_SLOT_MAX = 8;
const int zSND_CHANNEL_FRAME_RANDOM = -1;

class zCSoundFX;
class zCVob;
class zCParser;

class zCSoundSystem {
public:

    enum zTLoopType {
        zSND_LOOPING_DEFAULT,
        zSND_LOOPING_ENABLED,
        zSND_LOOPING_DISABLED
    };

    enum zTSpeakerType {
        zSPEAKER_TYPE_2_SPEAKER,
        zSPEAKER_TYPE_HEADPHONE,
        zSPEAKER_TYPE_SURROUND,
        zSPEAKER_TYPE_4_SPEAKER,
        zSPEAKER_TYPE_51_SPEAKER,
        zSPEAKER_TYPE_71_SPEAKER,
        zMaxSpeakerType
    };

    struct zTSound3DParams {
        void  SetDefaults();

        float obstruction;
        float volume;
        float radius;
        zTLoopType loopType;
        float coneAngleDeg;
        float reverbLevel;
        int isAmbient3D;
        float pitchOffset;
    };

    int             GetNumReverbPresets();
    const zSTRING*  GetReverbPresetName(int index);
    virtual ~zCSoundSystem();
    virtual zCSoundFX* LoadSoundFX(const zSTRING& name);
    virtual zCSoundFX* LoadSoundFXScript(const zSTRING& name);
    virtual zCParser* GetSFXParser();
    virtual float GetPlayingTimeMSEC(const zSTRING& name);
    virtual void SetSound3DDefaultRadius(float radius);
    virtual float GetSound3DDefaultRadius() const;
    virtual void SetMasterVolume(float volume);
    virtual float GetMasterVolume();
    virtual int PlaySound(zCSoundFX* sound, int slot);
    virtual int PlaySound(zCSoundFX* sound, int slot, int volume, float pitch, float reverb);
    virtual int PlaySound3D(zCSoundFX* sound, zCVob* vob, int slot, zTSound3DParams* params);
    virtual int PlaySound3D(const zSTRING& name, zCVob* vob, int slot, zTSound3DParams* params);
    virtual void StopSound(const int& handle);
    virtual void StopAllSounds();
    virtual int GetSound3DProps(const int& handle, zTSound3DParams& params);
    virtual int UpdateSound3D(const int& handle, zTSound3DParams* params);
    virtual void GetSoundProps(const int& handle, int& volume, float& pitch, float& reverb);
    virtual void UpdateSoundProps(const int& handle, int volume, float pitch, float reverb);
    virtual int IsSoundActive(const int& handle);
    virtual void DoSoundUpdate();
    virtual void SetListener(zCVob* vob);
    virtual void SetGlobalReverbPreset(int preset, float weight);
    virtual void SetReverbEnabled(int enabled);
    virtual int GetReverbEnabled();
    virtual int GetNumProvider();
    virtual const zSTRING* GetProviderName(int index);
    virtual int SetProvider(int provider);
    virtual void SetSpeakerType(zTSpeakerType speaker);
    virtual zTSpeakerType GetSpeakerType();
    virtual void SetGlobalOcclusion(float occlusion);
    virtual float GetCPULoad();

    float defaultRadius;
};

extern zCSoundSystem* zsound;

class zCSoundFX : public zCResource {
public:

    zCSoundFX();
    virtual ~zCSoundFX();
    virtual zCClassDef* _GetClassDef() const;
    static void operator delete(void* memory);
    virtual void GetCacheConfig(unsigned long& cacheOutTime, unsigned long& cacheSize);
    virtual int GetNumChannels() const;
    virtual int GetNumChannelFrames(int channel) const;
    virtual void GetChannelFrame(int channel, int frame) const;
    virtual void SetChannelFrame(int channel, int frame);
    virtual float GetPlayingTimeMSEC() const;
    virtual float GetPan() const;
    virtual void SetPan(float pan);
    virtual float GetVolume() const;
    virtual void SetVolume(float volume);
    virtual int GetFrequency() const;
    virtual void SetFrequency(int frequency);
    virtual void SetLooping(int looping);
    virtual int IsLooping() const;
    virtual void SetIsFixed(int fixed);
    virtual int GetIsFixed() const;

    static zCClassDef* classDef;
};

class zCSoundFXDummy : public zCSoundFX {
public:
    virtual ~zCSoundFXDummy();
};

class zCSoundSystemDummy : public zCSoundSystem {
public:
    void DoSoundUpdate();
    zCSoundFX* LoadSoundFX(const zSTRING& name);
    int PlaySound(zCSoundFX* sound, int slot);
    int PlaySound(zCSoundFX* sound, int slot, int volume, float pitch, float reverb);
    int PlaySound3D(zCSoundFX* sound, zCVob* vob, int slot, zTSound3DParams* params);
    int PlaySound3D(const zSTRING& name, zCVob* vob, int slot, zTSound3DParams* params);
    int GetSound3DProps(const int& handle, zTSound3DParams& params);
    int UpdateSound3D(const int& handle, zTSound3DParams* params);
    void SetGlobalOcclusion(float occlusion);
};

#endif
