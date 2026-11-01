#include "include/zSndMss.h"
#include "include/zSndFxMss.h"
#include "include/zSndActive.h"
#include "include/zWaveData.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include <stdlib.h>

extern zCOption* zgameoptions;
extern zCParser* parserSoundFX;
zSTRING SndMaterialName[6] = { "WOOD", "STONE", "METAL", "LEATHER", "CLAY", "GLAS" };
static int soundPreferencesState = -1;
static zSTRING soundEmptyString1;
static zSTRING soundEmptyString2;
static zSTRING soundPercentSign = "%";
static zSTRING soundInternalName = "Internal";
static zSTRING soundUnderscore = "_";
static zSTRING soundDefaultWave = "DEFAULT.WAV";
static zSTRING soundNoWave = "NOWAVE";
static zSTRING soundProviderMilesFast2D = "Miles Fast 2D Positional Audio";
static zSTRING soundProviderDolbySurround = "Dolby Surround";
static zSTRING soundProviderD3DSoftware = "DirectSound3D Software Emulation";
static zSTRING soundProviderD3DHardware = "DirectSound3D Hardware Support";
static zSTRING soundProviderD3D7PanVolume = "DirectSound3D 7+ Software - Pan and Volume";
static zSTRING soundProviderD3D7LightHRTF = "DirectSound3D 7+ Software - Light HRTF";
static zSTRING soundProviderD3D7FullHRTF = "DirectSound3D 7+ Software - Full HRTF";
static zSTRING soundProviderEAX = "Creative Labs EAX (TM)";
static zSTRING soundProviderEAX2 = "Creative Labs EAX 2 (TM)";
static zSTRING soundProviderEAX3 = "Creative Labs EAX 3 (TM)";
static zSTRING soundProviderA3D = "Aureal A3D Interactive (TM)";
static zSTRING soundProviderA3D2 = "Aureal A3D 2.0 (TM)";
static zSTRING soundProviderRSX = "RAD Game Tools RSX 3D Audio";

struct zTSndProviderInfo {
    zSTRING internalName;
    zSTRING displayName;
    zSTRING libraryName;
    zSTRING description;
};

static const zTSndProviderInfo soundProviderInfo[13] = {
    { "MILES", "Miles Fast", "MssSoft.m3d", "Miles Fast 2D Positional Audio" },
    { "DS3D_HW", "DirectSound3D HW", "MssDS3D.m3d", "DirectSound3D Hardware Support" },
    { "EAX2", "Creative EAX 2", "MssEAX.m3d", "Creative Labs EAX 2 (TM)" },
    { "DOLBY", "Dolby Surround", "MssSoft.m3d", "Dolby Surround" },
    { "RSX", "RSX 3D Audio", "MssRSX.m3d", "RAD Game Tools RSX 3D Audio" },
    { "DS3D_SW", "DirectSound3D SW", "MssDS3D.m3d", "DirectSound3D Software Emulation" },
    { "DX7_PV", "DS3D7 SW - Pan/Volume", "MssDX7.m3d", "DirectSound3D 7+ Software - Pan and Volume" },
    { "DX7_HRTF", "DS3D7 SW - light HRTF", "MssDX7.m3d", "DirectSound3D 7+ Software - Light HRTF" },
    { "DX7_FULL", "DS3D7 SW - full HRTF", "MssDX7.m3d", "DirectSound3D 7+ Software - Full HRTF" },
    { "EAX", "Creative EAX", "MssEAX.m3d", "Creative Labs EAX (TM)" },
    { "EAX3", "Creative EAX 3", "MssEAX.m3d", "Creative Labs EAX 3 (TM)" },
    { "A3D", "Aureal A3D", "MssA3D.m3d", "Aureal A3D Interactive (TM)" },
    { "A3D2", "Aureal A3D 2.0", "MssA3D.m3d", "Aureal A3D 2.0 (TM)" }
};
static zSTRING soundDefaultProviderMiles = "MILES";
static zSTRING sndSysConfig = "sndSysConfig";
static zCHandleManager soundHandleManager;
static zSTRING soundDefaultEmptyString;
static zCSndDebugCallback sndDebugCallback;

zCDrvProperties_MSS zCSndSys_MSS::prefs;

zCDrvProperties_MSS::zCDrvProperties_MSS()
{
}

zCDrvProperties_MSS::~zCDrvProperties_MSS()
{
}

zCHandleManager::zCHandleManager()
{
    free3DSamples.parray = NULL;
    free3DSamples.numAlloc = 0;
    free3DSamples.numInArray = 0;
    freeSamples.parray = NULL;
    freeSamples.numAlloc = 0;
    freeSamples.numInArray = 0;
}

zCHandleManager::~zCHandleManager()
{
    DisposeAllSamples();
}

// EXE: 0x004FC300
h3DPOBJECT* zCHandleManager::Alloc3DSample()
{
    if (free3DSamples.numInArray > 0)
        return free3DSamples.parray[--free3DSamples.numInArray];
    return reinterpret_cast<h3DPOBJECT*>(new unsigned char);
}

// EXE: 0x004FC330
_SAMPLE* zCHandleManager::AllocSample()
{
    if (freeSamples.numInArray > 0)
        return freeSamples.parray[--freeSamples.numInArray];
    return reinterpret_cast<_SAMPLE*>(new unsigned char);
}

// EXE: 0x004FC370
void zCHandleManager::Free3DSample(h3DPOBJECT* sample)
{
    if (sample)
        free3DSamples.InsertEnd(sample);
}

// EXE: 0x004FC440
void zCHandleManager::FreeSample(_SAMPLE* sample)
{
    if (sample)
        freeSamples.InsertEnd(sample);
}

// EXE: 0x004FC520
void zCHandleManager::DisposeAllSamples()
{
    for (int i = 0; i < free3DSamples.numInArray; i++)
        delete reinterpret_cast<unsigned char*>(free3DSamples.parray[i]);
    free3DSamples.DeleteList();

    for (int i = 0; i < freeSamples.numInArray; i++)
        delete reinterpret_cast<unsigned char*>(freeSamples.parray[i]);
    freeSamples.DeleteList();
}

zCWavePool::zCWavePool()
{
    searchWave = new zCWaveData();
}

zCWavePool::~zCWavePool()
{
    if (searchWave)
        searchWave->Release();
    searchWave = NULL;
    activeWaveList.DeleteList();
    waveList.DeleteList();
}

zCWaveData* zCWavePool::CacheIn(zSTRING& fileName)
{
    searchWave->fileName = fileName;
    zCWaveData* wave = NULL;

    for (int i = 0; i < waveList.numInArray; i++) {
        if (waveList.parray[i]->fileName == fileName) {
            wave = waveList.parray[i];
            wave->refCtr++;
            break;
        }
    }

    if (!wave) {
        wave = new zCWaveData(fileName);
        waveList.InsertEnd(wave);
    }

    wave->CacheIn();
    return wave;
}

void zCWavePool::CacheIn(zCWaveData* wave)
{
    if (wave)
        wave->CacheIn();
}

void zCWavePool::CacheOut(zCWaveData* wave)
{
    if (wave)
        wave->CacheOut();
}

int zCWavePool::ReleaseWave(zCWaveData* wave)
{
    if (!wave)
        return -1;

    for (int i = 0; i < waveList.numInArray; i++) {
        if (waveList.parray[i] == wave) {
            waveList.RemoveIndex(i);
            break;
        }
    }

    return wave->Release();
}

void zCWavePool::NewFrame()
{
    activeWaveList.DeleteList();

    for (int i = 0; i < zCActiveSnd::activeSndList.numInArray; i++) {
        zCSndFrame* frame = zCActiveSnd::activeSndList.parray[i]->sourceFrm;
        if (!frame || !frame->wav || !frame->wav->memAddr)
            continue;

        int found = 0;
        for (int j = 0; j < activeWaveList.numInArray; j++) {
            if (activeWaveList.parray[j] == frame->wav) {
                found = 1;
                break;
            }
        }

        if (!found)
            activeWaveList.InsertEnd(frame->wav);
    }
}

void zCWavePool::DebugInfo()
{
    zerr->Message("C: Actual loaded waves:");
    int totalSize = 0;

    for (int i = 0; i < waveList.numInArray; i++) {
        zCWaveData* wave = waveList.parray[i];
        zerr->Message(wave->fileName + (wave->memAddr ? " (cached in)" : " (cached out)"));
        if (wave->memAddr) {
            if (!wave->size)
                wave->CacheIn();
            totalSize += wave->size;
        }
    }

    zerr->Message("C: total memory used by all wave files : " + zSTRING(totalSize));
}

void zCWavePool::DebugInfo2()
{
    int num3D = 0;
    int numAmbient = 0;

    for (int i = 0; i < zCActiveSnd::activeSndList.numInArray; i++) {
        zCActiveSnd* sound = zCActiveSnd::activeSndList.parray[i];
        if (!sound->sourceFrm || (!sound->sample && !sound->sample3D))
            continue;

        const int volume = sound->is3D
            ? int(sound->GetVolume() * sound->volWeight * 127.0f)
            : sound->volume;
        if (sound->is3D)
            num3D++;
        else
            numAmbient++;

        zerr->Message(sound->sourceFrm->fileName + (sound->is3D ? " (3D), vol: " : " (AMBIENT), vol: ")
            + zSTRING(volume));
    }

    zerr->Message("num 3d: " + zSTRING(num3D));
    zerr->Message("num ambient: " + zSTRING(numAmbient));
}

void zCSndDebugCallback::DoWorldPerFrameCallback(zCWorld* world, zCCamera* camera)
{
    zCWavePool::GetPool().DebugInfo2();
}

zCSndSys_MSS::zCSndSys_MSS()
{
    speakerType = zCSoundSystem::zSPEAKER_TYPE_2_SPEAKER;
    globalReverbPreset = 0;
    m_bReverbEnabled = 0;
    defaultRadius = 3500.0f;
    prefs.sampleRate = 22000;
    prefs.numSFXChannels = 16;
    prefs.useStereo = 1;
    prefs.bitResolution = 16;
    prefs.volume = 1.0f;
    zoptions->ChangeDir(8);

    if (!parserSoundFX)
        parserSoundFX = new zCParser(100);

    zSTRING soundFile;
    if (zgameoptions)
        soundFile = zgameoptions->ReadString("FILES", "SoundEffects", "");
    if (soundFile.IsEmpty())
        soundFile = "System\\Sfx";

    parserSoundFX->Parse(soundFile + ".src");
    parserSoundFX->CreatePCode();
    InitializeMSS();
    SetSpeakerType(speakerType);
    SetReverbEnabled(m_bReverbEnabled);
}

int zCSndSys_MSS::GetReverbEnabled()
{
    return m_bReverbEnabled;
}

zCSoundSystem::zTSpeakerType zCSndSys_MSS::GetSpeakerType()
{
    return speakerType;
}

static void* digitalDriver;
static zCVob* listenerVob;
static float mutedVolume;

zCSndSys_MSS::~zCSndSys_MSS()
{
    DisposeAllSampleHandles();
    delete parserSoundFX;
    parserSoundFX = NULL;
    listenerVob = NULL;
    digitalDriver = NULL;
}

void zCSndSys_MSS::InitializeMSS()
{
    prefs.numProviders = GetNumProvider();
    prefs.use3dProvider = zoptions->ReadString("SOUND", "use3dProvider", "MILES");
    prefs.numSFXChannels = zoptions->ReadInt("SOUND", "numSFXChannels", prefs.numSFXChannels);
    prefs.sampleRate = zoptions->ReadInt("SOUND", "sampleRate", prefs.sampleRate);
    prefs.useStereo = zoptions->ReadInt("SOUND", "useStereo", prefs.useStereo);
}

void* zCSndSys_MSS::GetDigitalDriverHandle()
{
    return digitalDriver;
}

void zCSndSys_MSS::SetNumAudioChannels(int channels)
{
    zCActiveSnd::maxChannels = channels;
    prefs.numSFXChannels = channels;
}

float zCSndSys_MSS::GetMasterVolume()
{
    return prefs.volume / 127.0f;
}

int zCSndSys_MSS::GetNumSoundsPlaying()
{
    int result = 0;
    for (int i = 0; i < zCActiveSnd::activeSndList.numInArray; i++) {
        if (zCActiveSnd::activeSndList.array[i]->active)
            result++;
    }
    return result;
}

void zCSndSys_MSS::RemoveAllActiveSounds()
{
    for (int i = 0; i < zCActiveSnd::activeSndList.numInArray; i++) {
        zCActiveSnd* sound = zCActiveSnd::activeSndList.array[i];
        if (sound->active)
            zCActiveSnd::RemoveSound(sound);
    }
}

void zCSndSys_MSS::DisposeAllSampleHandles()
{
    for (int i = 0; i < zCActiveSnd::activeSndList.numInArray; i++) {
        zCActiveSnd* sound = zCActiveSnd::activeSndList.array[i];
        sound->sample = NULL;
        sound->sample3D = NULL;
        sound->active = 0;
    }
}

void zCSndSys_MSS::SetListener(zCVob* vob)
{
    if (listenerVob)
        listenerVob->refCtr--;
    listenerVob = vob;
    if (listenerVob)
        listenerVob->refCtr++;
}

void zCSndSys_MSS::SetMasterVolume(float volume)
{
    prefs.volume = volume != -1.0f ? volume : 1.0f;
}

void zCSndSys_MSS::MuteSFX()
{
    float previous = mutedVolume;
    mutedVolume = prefs.volume;
    prefs.volume = previous;
}

int zCSndSys_MSS::EvalFunc(const zSTRING& command, zSTRING& output)
{
    zSTRING action = command.PickWord(1, " ", "");
    action.Upper();

    if (action == "SOUNDSTAT") {
        output = "sound channels: " + zSTRING(prefs.numSFXChannels);
        zCWavePool::GetPool().DebugInfo();
        zCWavePool::GetPool().DebugInfo2();
        return 1;
    }

    zSTRING option = command.PickWord(2, " ", "");
    option.Upper();

    if (action == "TOGGLE" && option == "SOUNDUPDATE") {
        static int noSoundUpdate;
        noSoundUpdate = !noSoundUpdate;
        output = noSoundUpdate ? "no sound update on" : "no sound update off";
        return 1;
    }

    if (action == "TOGGLE" && option == "SOUNDINFO") {
        static int soundInfo;
        soundInfo = !soundInfo;
        output = soundInfo ? "sound info on" : "sound info off";
        return 1;
    }

    if (action == "ZTOGGLE" && option == "SHOWSOUNDS") {
        static int showSounds;
        showSounds = !showSounds;
        output = showSounds ? "show sounds on" : "show sounds off";
        return 1;
    }

    return 0;
}

void zCSndSys_MSS::FrmCon_ParamChanged(const zSTRING& params)
{
    zCSndFrame defaults;
    defaults.fileName.Clear();
    defaults.pitchOffset = 0.0f;
    defaults.pitchVariance = 0.0f;
    defaults.defVolume = 127;
    defaults.looping = 0;
    defaults.loopStartOffset = 0;
    defaults.loopEndOffset = -1;
    defaults.reverbLevel = 0.5f;

    for (int i = 0; i < zCSoundFX::classDef->objectList.numInArray; i++) {
        zCSndFX_MSS* sound = dynamic_cast<zCSndFX_MSS*>(zCSoundFX::classDef->objectList.parray[i]);
        if (!sound)
            continue;

        for (int channel = 0; channel < sound->channelList.numInArray; channel++) {
            zCArray<zCSndFrame*>& frames = sound->channelList.parray[channel]->frameList;

            for (int frame = 0; frame < frames.numInArray; frame++) {
                if (!defaults.fileName.IsEmpty())
                    frames.parray[frame]->fileName = defaults.fileName;
                frames.parray[frame]->pitchOffset = defaults.pitchOffset;
                frames.parray[frame]->pitchVariance = defaults.pitchVariance;
                frames.parray[frame]->defVolume = defaults.defVolume;
                frames.parray[frame]->looping = defaults.looping;
                frames.parray[frame]->loopStartOffset = defaults.loopStartOffset;
                frames.parray[frame]->loopEndOffset = defaults.loopEndOffset;
                frames.parray[frame]->reverbLevel = defaults.reverbLevel;
            }
        }
    }
}

void zCSndSys_MSS::SfxCon_ParamChanged(const zSTRING& params)
{
}

void zCSndSys_MSS::CloseProvider()
{
    if (digitalDriver) {
        digitalDriver = NULL;
        prefs.use3dProvider.Clear();
    }
}

static zCArray<zCSndFX_MSS*> mssSoundCache;

static zCSoundFX* FindMssSound(const zSTRING& name)
{
    for (int i = 0; i < mssSoundCache.numInArray; i++) {
        if (mssSoundCache.parray[i]->GetObjectName() == name) {
            mssSoundCache.parray[i]->refCtr++;
            return mssSoundCache.parray[i];
        }
    }

    return NULL;
}

static zCSndFX_MSS* CreateMssSound(const zSTRING& name, int frameCount)
{
    zCSndFX_MSS* sound = new zCSndFX_MSS();
    sound->SetObjectName(name);
    zCSndChannel* channel = new zCSndChannel();
    channel->actFrame = -1;
    channel->refCtr = 1;
    channel->loaded = 0;

    for (int i = 0; i < frameCount; i++) {
        zCSndFrame* frame = new zCSndFrame();
        frame->fileName = frameCount == 1 ? name : name + zSTRING(i);
        frame->instance = frame->fileName;
        frame->pitchOffset = 0.0f;
        frame->pitchVariance = 0.0f;
        frame->defVolume = 127;
        frame->looping = 0;
        frame->loopStartOffset = 0;
        frame->loopEndOffset = -1;
        frame->reverbLevel = 0.5f;
        frame->actVolume = 0;
        frame->actRate = 0;
        frame->actPan = 0;
        frame->wav = NULL;

        if (parserSoundFX->GetSymbol(frame->instance))
            parserSoundFX->CreateInstance(frame->instance, frame);
        channel->frameList.InsertEnd(frame);
    }

    sound->channelList.InsertEnd(channel);
    mssSoundCache.InsertEnd(sound);
    return sound;
}

zCSoundFX* zCSndSys_MSS::LoadSoundFX(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();

    if (upperName.Search(".MP3", 0) >= 0)
        return NULL;
    if (upperName.Search(".WAV", 0) < 0)
        return LoadSoundFXScript(upperName);
    return LoadSingle(upperName);
}

zCSoundFX* zCSndSys_MSS::LoadSoundFXScript(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();

    if (upperName.IsEmpty() || !parserSoundFX->GetSymbol(upperName))
        return NULL;

    zoptions->ChangeDir(9);
    char last = upperName[upperName.Length() - 1];

    if (last >= '1' && last <= '9') {
        int count = last - '0';
        return LoadMulti(upperName.Copied(0, upperName.Length() - 1), count);
    }

    return LoadSingle(upperName);
}

zCSoundFX* zCSndSys_MSS::LoadMulti(const zSTRING& name, int count)
{
    zCSoundFX* sound = FindMssSound(name);
    if (sound)
        return sound;
    return CreateMssSound(name, count);
}

zCSoundFX* zCSndSys_MSS::LoadSingle(const zSTRING& name)
{
    zCSoundFX* sound = FindMssSound(name);
    if (sound)
        return sound;
    return CreateMssSound(name, 1);
}

zCParser* zCSndSys_MSS::GetSFXParser()
{
    return parserSoundFX;
}

static int StartMssSound(zCSoundFX* sound, int slot, zCVob* vob,
    zCSoundSystem::zTSound3DParams* params, int ambient)
{
    zCSndFX_MSS* mssSound = dynamic_cast<zCSndFX_MSS*>(sound);
    if (!mssSound || !mssSound->channelList.numInArray)
        return 0;

    zCSndChannel* channel = mssSound->channelList.parray[mssSound->actChannel];
    if (channel->frameList.numInArray <= 1)
        mssSound->actChannelFrame = 0;
    else if (mssSound->actChannelFrame == zSND_CHANNEL_FRAME_RANDOM)
        mssSound->actChannelFrame = rand() % channel->frameList.numInArray;

    zCActiveSnd* active = zCActiveSnd::AllocNextFreeSnd();
    active->sourceSnd = mssSound;
    active->sourceFrm = channel->frameList.parray[mssSound->actChannelFrame];
    active->sourceVob = vob;
    active->vobSlot = slot;
    active->is3D = vob != NULL;
    active->isAmbient = ambient;
    active->loopType = params ? params->loopType : zCSoundSystem::zSND_LOOPING_DEFAULT;
    active->looping = active->sourceFrm->looping;
    active->reverbLevel = params ? params->reverbLevel : active->sourceFrm->reverbLevel;
    active->obstruction = params ? params->obstruction : 0.0f;
    active->pitchOffset = params ? params->pitchOffset : active->sourceFrm->pitchOffset;
    active->radius = params ? params->radius : 3500.0f;
    active->volWeight = params && params->volume != -1.0f ? params->volume : 1.0f;
    active->volume = active->sourceFrm->defVolume;
    active->pan = 64;
    active->rate = active->sourceFrm->actRate ? active->sourceFrm->actRate : 44100;
    mssSound->startPlayTime = ztimer.totalTimeFloat;
    mssSound->refCtr++;
    return active->handle;
}

int zCSndSys_MSS::PlaySound(zCSoundFX* sound, int slot)
{
    if (!sound)
        return 0;
    return StartMssSound(sound, slot, NULL, NULL, 0);
}

int zCSndSys_MSS::PlaySound(zCSoundFX* sound, int slot, int volume, float pitch, float reverb)
{
    if (!sound)
        return 0;

    zCSoundSystem::zTSound3DParams params;
    params.obstruction = 0.0f;
    params.volume = volume == -1 ? -1.0f : float(volume) / 127.0f;
    params.radius = defaultRadius;
    params.loopType = zSND_LOOPING_DEFAULT;
    params.coneAngleDeg = 0.0f;
    params.reverbLevel = reverb;
    params.isAmbient3D = 0;
    params.pitchOffset = pitch;
    return StartMssSound(sound, slot, NULL, &params, 0);
}

int zCSndSys_MSS::PlaySound3D(const zSTRING& name, zCVob* vob, int slot, zTSound3DParams* params)
{
    if (name.IsEmpty())
        return 0;

    zCSoundFX* sound = LoadSoundFX(name);
    if (!sound)
        return 0;

    sound->startPlayTime = ztimer.totalTimeFloat;
    int result = PlaySound3D(sound, vob, slot, params);
    sound->refCtr--;
    return result;
}

int zCSndSys_MSS::PlaySound3D(zCSoundFX* sound, zCVob* vob, int slot, zTSound3DParams* params)
{
    if (!sound)
        return 0;

    if (params && params->isAmbient3D)
        return PlaySound3DAmbient(sound, vob, slot, params);
    return StartMssSound(sound, slot, vob, params, 0);
}

int zCSndSys_MSS::PlaySound3DAmbient(zCSoundFX* sound, zCVob* vob, int slot, zTSound3DParams* params)
{
    if (!sound)
        return 0;
    return StartMssSound(sound, slot, vob, params, 1);
}

void zCSndSys_MSS::StopSound(const int& handle)
{
    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);
    if (sound)
        zCActiveSnd::RemoveSound(sound);
}

void zCSndSys_MSS::StopAllSounds()
{
    for (int i = zCActiveSnd::activeSndList.numInArray - 1; i >= 0; i--) {
        zCActiveSnd* active = zCActiveSnd::activeSndList.array[i];
        if (!active->sourceSnd || !active->sourceSnd->IsLooping())
            zCActiveSnd::RemoveSound(active);
    }
}

int zCSndSys_MSS::UpdateSound3D(const int& handle, zTSound3DParams* params)
{
    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);
    if (!sound)
        return 0;

    UpdateSoundPropsAmbient(sound, params);
    sound->frameCtr++;
    return 1;
}

void zCSndSys_MSS::UpdateSoundPropsAmbient(zCActiveSnd* sound, zTSound3DParams* params)
{
    if (params) {
        sound->radius = params->radius != -1.0f ? params->radius : defaultRadius;
        sound->volWeight = params->volume != -1.0f ? params->volume : 1.0f;
        if (sound->volWeight > 1.0f)
            sound->volWeight = 1.0f;
        sound->loopType = params->loopType;
        sound->obstruction = params->obstruction * 0.7f;
        sound->reverbLevel = GetReverbEnabled() ? params->reverbLevel : 0.0f;
    }
    else
        sound->obstruction = 0.0f;

    sound->volume = unsigned char(sound->sourceFrm->actVolume * sound->volWeight);
    sound->active = sound->volume > 0;
}

void zCSndSys_MSS::GetSoundProps(const int& handle, int& volume, float& pitch, float& reverb)
{
    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);

    if (!sound) {
        volume = 0;
        pitch = -1.0f;
        reverb = zSND_PAN_DEFAULT;
        return;
    }

    volume = sound->volume != sound->sourceFrm->defVolume ? sound->volume : -1;
    pitch = sound->pan != 64 ? sound->pan / 127.0f * 2.0f - 1.0f : zSND_PAN_DEFAULT;
    reverb = sound->rate != 44100 ? float(sound->rate) : -1.0f;
}

int zCSndSys_MSS::GetSound3DProps(const int& handle, zTSound3DParams& params)
{
    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);

    if (!sound) {
        params.obstruction = 0.0f;
        params.volume = 1.0f;
        params.radius = -1.0f;
        params.loopType = zSND_LOOPING_DEFAULT;
        params.coneAngleDeg = 0.0f;
        params.reverbLevel = 1.0f;
        params.isAmbient3D = 0;
        params.pitchOffset = -999999.0f;
        return 0;
    }

    params.obstruction = sound->obstruction / 0.7f;
    params.volume = sound->volWeight;
    params.radius = sound->radius;
    params.loopType = sound->loopType;
    params.coneAngleDeg = 0.0f;
    params.reverbLevel = GetReverbEnabled() ? sound->reverbLevel : 0.0f;
    params.isAmbient3D = sound->isAmbient;
    params.pitchOffset = sound->pitchOffset;
    return 1;
}

float zCSndSys_MSS::GetActiveVolume(const int& handle)
{
    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);
    if (!sound)
        return -1.0f;
    return sound->volume / 127.0f * (1.0f - sound->obstruction) * sound->volWeight;
}

void zCSndSys_MSS::UpdateSoundProps(const int& handle, int volume, float pitch, float reverb)
{
    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);
    if (!sound)
        return;

    sound->volume = volume == -1 ? sound->sourceFrm->defVolume : volume;
    sound->pan = pitch == zSND_PAN_DEFAULT ? 64 : unsigned char((pitch + 1.0f) * 63.5f);
    sound->rate = reverb == -1.0f ? 44100 : (unsigned short)reverb;
    sound->active = sound->volume > 0;
}

void zCSndSys_MSS::DoSoundUpdate()
{
    zCWavePool::GetPool().NewFrame();
    zCActiveSnd::NewFrame();
}

int zCSndSys_MSS::IsSoundActive(const int& handle)
{
    if (!handle)
        return 0;

    zCActiveSnd* sound = zCActiveSnd::GetHandleSound(handle);
    return sound && (sound->active || sound->sample || sound->sample3D);
}

float zCSndSys_MSS::GetPlayingTimeMSEC(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();

    for (int i = 0; i < mssSoundCache.numInArray; i++) {
        if (mssSoundCache.parray[i]->GetObjectName() == upperName)
            return (ztimer.totalTimeFloat - mssSoundCache.parray[i]->startPlayTime) * 1000.0f;
    }

    return 0.0f;
}

int zCSndSys_MSS::GetUsedSoundMem()
{
    static int usedSoundMem;
    return usedSoundMem;
}

void zCSndSys_MSS::SetGlobalReverbPreset(int preset, float weight)
{
    if (!m_bReverbEnabled || globalReverbPreset == preset)
        return;

    globalReverbPreset = preset;
}

void zCSndSys_MSS::SetReverbEnabled(int enabled)
{
    if (m_bReverbEnabled && !enabled)
        SetGlobalReverbPreset(enabled, enabled);
    m_bReverbEnabled = enabled;
}

int zCSndSys_MSS::GetNumProvider()
{
    return zoptions->ReadInt("SOUND", "extendedProviders", 0) ? 13 : 3;
}

const zSTRING* zCSndSys_MSS::GetProviderName(int index)
{
    return &soundProviderInfo[index].displayName;
}

int zCSndSys_MSS::SetProvider(int provider)
{
    return 1;
}

int zCSndSys_MSS::Start3DProvider(int provider)
{
    prefs.numProviders = GetNumProvider();
    prefs.use3dProvider = soundProviderInfo[provider].description;
    return 1;
}

static float globalOcclusion;

void zCSndSys_MSS::SetGlobalOcclusion(float occlusion)
{
    globalOcclusion = occlusion;
}

float zCSndSys_MSS::GetCPULoad()
{
    return 0.0f;
}

void zCSndSys_MSS::SetSpeakerType(zCSoundSystem::zTSpeakerType speaker)
{
    if (speaker <= zSPEAKER_TYPE_71_SPEAKER)
        speakerType = speaker;
}
