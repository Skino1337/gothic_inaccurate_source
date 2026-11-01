#ifndef ZSND_MSS_H
#define ZSND_MSS_H

#include "zSound.h"
#include "zObject.h"

class zCParser;
class zCActiveSnd;
class zCWaveData;
struct h3DPOBJECT;
struct _SAMPLE;

const int MAX_PROVIDER = 20;

class zCDrvProperties_MSS {
public:

    zCDrvProperties_MSS();
    ~zCDrvProperties_MSS();

    float volume;
    int bitResolution;
    int sampleRate;
    int useStereo;
    int numSFXChannels;
    zSTRING use3dProvider;
    unsigned char dScriptEnd;
    unsigned char waveFormat[16];
    long* providers[MAX_PROVIDER];
    zSTRING provNames[MAX_PROVIDER];
    int numProviders;
};

class zCHandleManager {
public:

    zCHandleManager();
    ~zCHandleManager();
    h3DPOBJECT* Alloc3DSample();
    _SAMPLE*      AllocSample();
    void          Free3DSample(h3DPOBJECT* sample);
    void          FreeSample(_SAMPLE* sample);
    void          DisposeAllSamples();

    zCArray<h3DPOBJECT*> free3DSamples;
    zCArray<_SAMPLE*>      freeSamples;
};

class zCWavePool {
public:

    zCWavePool();
    ~zCWavePool();
    static zCWavePool& GetPool()
    {
        static zCWavePool thePool;
        return thePool;
    }

    zCWaveData* CacheIn(zSTRING& fileName);
    void        CacheIn(zCWaveData* wave);
    void        CacheOut(zCWaveData* wave);
    int         ReleaseWave(zCWaveData* wave);
    void        NewFrame();
    void        DebugInfo();
    void        DebugInfo2();

    zCArraySort<zCWaveData*> waveList;
    zCWaveData*              searchWave;
    zCArray<zCWaveData*>     activeWaveList;
};

class zCSndDebugCallback {
public:
    void DoWorldPerFrameCallback(zCWorld* world, zCCamera* camera);
};

class zCSndSys_MSS : public zCSoundSystem {
public:

    zCSndSys_MSS();
    virtual ~zCSndSys_MSS();
    void  InitializeMSS();
    void* GetDigitalDriverHandle();
    void  SetNumAudioChannels(int channels);
    virtual float GetMasterVolume();
    int   GetNumSoundsPlaying();
    void  RemoveAllActiveSounds();
    void  DisposeAllSampleHandles();
    virtual void SetListener(zCVob* vob);
    virtual void SetMasterVolume(float volume);
    void  MuteSFX();
    static int EvalFunc(const zSTRING& command, zSTRING& output);
    static void FrmCon_ParamChanged(const zSTRING& params);
    static void SfxCon_ParamChanged(const zSTRING& params);
    static void CloseProvider();
    virtual zCSoundFX* LoadSoundFX(const zSTRING& name);
    virtual zCSoundFX* LoadSoundFXScript(const zSTRING& name);
    zCSoundFX* LoadMulti(const zSTRING& name, int count);
    zCSoundFX* LoadSingle(const zSTRING& name);
    virtual zCParser* GetSFXParser();
    virtual int PlaySound(zCSoundFX* sound, int slot);
    virtual int PlaySound(zCSoundFX* sound, int slot, int volume, float pitch, float reverb);
    virtual int PlaySound3D(zCSoundFX* sound, zCVob* vob, int slot, zTSound3DParams* params);
    virtual int PlaySound3D(const zSTRING& name, zCVob* vob, int slot, zTSound3DParams* params);
    int   PlaySound3DAmbient(zCSoundFX* sound, zCVob* vob, int slot, zTSound3DParams* params);
    virtual void StopSound(const int& handle);
    virtual void StopAllSounds();
    virtual int UpdateSound3D(const int& handle, zTSound3DParams* params);
    void  UpdateSoundPropsAmbient(zCActiveSnd* sound, zTSound3DParams* params);
    virtual void GetSoundProps(const int& handle, int& volume, float& pitch, float& reverb);
    virtual int GetSound3DProps(const int& handle, zTSound3DParams& params);
    float GetActiveVolume(const int& handle);
    virtual void UpdateSoundProps(const int& handle, int volume, float pitch, float reverb);
    virtual void DoSoundUpdate();
    virtual int IsSoundActive(const int& handle);
    virtual float GetPlayingTimeMSEC(const zSTRING& name);
    int   GetUsedSoundMem();
    virtual void SetGlobalReverbPreset(int preset, float weight);
    virtual void SetReverbEnabled(int enabled);
    virtual int GetNumProvider();
    virtual const zSTRING* GetProviderName(int index);
    virtual int SetProvider(int provider);
    int   Start3DProvider(int provider);
    virtual void SetGlobalOcclusion(float occlusion);
    virtual float GetCPULoad();
    virtual void SetSpeakerType(zCSoundSystem::zTSpeakerType speaker);
    virtual int GetReverbEnabled();
    virtual zCSoundSystem::zTSpeakerType GetSpeakerType();

    zCSoundSystem::zTSpeakerType speakerType;
    int globalReverbPreset;
    int m_bReverbEnabled;

    static zCDrvProperties_MSS prefs;
};

#endif
