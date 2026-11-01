#include "include/zSndActive.h"
#include "include/zMath.h"
#include "include/zSky_Outdoor.h"

zCArraySort<zCActiveSnd*> zCActiveSnd::activeSndList;
unsigned long zCActiveSnd::lastHandle;
int zCActiveSnd::maxChannels = 16;
zCArray<zCActiveSnd*> zCActiveSnd::preAllocedSndList;
zCActiveSnd* zCActiveSnd::nextFreeSnd;
int zCActiveSnd::autoUpdate3D = 1;

zCActiveSnd::zCActiveSnd()
{
    Initialize();
}

zCActiveSnd::~zCActiveSnd()
{
    if (sourceVob)
        sourceVob->refCtr--;
    if (sourceSnd)
        sourceSnd->refCtr--;
    Initialize();
    nextFreeSnd = NULL;
}

void zCActiveSnd::Initialize()
{
    handle = 0;
    sample = NULL;
    sample3D = NULL;
    age = 0;
    loopType = zCSoundSystem::zSND_LOOPING_DEFAULT;
    radius = -1.0f;
    reverbLevel = 0.0f;
    pitchOffset = -999999.0f;
    volWeight = 1.0f;
    obstruction = 0.0f;
    obstructionToGo = 0.0f;
    volumeToGo = 1.0f;
    autoObstructTimer = 0;
    active = 0;
    looping = 0;
    isAmbient = 0;
    is3D = 0;
    allocated = 0;
    vobSlot = 0;
    pan = 64;
    volume = 127;
    rate = 44100;
    muteTime = 0;
    frameCtr = 0;
    lastPos = zVEC3(0.0f, 0.0f, 0.0f);
    lastTime = 0.0f;
    sourceVob = NULL;
    sourceFrm = NULL;
    sourceSnd = NULL;
}

void zCActiveSnd::Release()
{
    if (sourceVob)
        sourceVob->refCtr--;
    if (sourceSnd)
        sourceSnd->refCtr--;
    Initialize();
    nextFreeSnd = this;
}

int zCActiveSnd::GetNextFreeHandle()
{
    return ++lastHandle;
}

zCActiveSnd* zCActiveSnd::AllocNextFreeSnd()
{
    if (nextFreeSnd) {
        zCActiveSnd* sound = nextFreeSnd;
        nextFreeSnd = NULL;
        sound->handle = ++lastHandle;
        sound->active = 1;
        sound->allocated = 1;
        activeSndList.InsertEnd(sound);
        return sound;
    }

    zCActiveSnd* sound = new zCActiveSnd();
    preAllocedSndList.InsertEnd(sound);
    sound->handle = ++lastHandle;
    sound->active = 1;
    sound->allocated = 1;
    activeSndList.InsertEnd(sound);
    return sound;
}

int zCActiveSnd::GetHandleIndex(int handle)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->handle == handle)
            return i;
    }

    return -1;
}

int zCActiveSnd::GetHandleActive(int handle)
{
    if (!handle)
        return 0;

    zCActiveSnd* sound = GetHandleSound(handle);
    return sound && (sound->active || sound->sample || sound->sample3D);
}

int zCActiveSnd::GetSoundActive(zCActiveSnd* sound)
{
    if (!sound)
        return 0;

    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i] == sound)
            return sound->active;
    }

    return 0;
}

int zCActiveSnd::GetFrameActive(zCSndFrame* frame)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->active && activeSndList.array[i]->sourceFrm == frame)
            return 1;
    }

    return 0;
}

zCActiveSnd* zCActiveSnd::GetHandleSound(int handle)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->handle == handle)
            return activeSndList.array[i];
    }

    return NULL;
}

void zCActiveSnd::AddSound(zCActiveSnd* sound)
{
    activeSndList.InsertSort(sound);
}

void zCActiveSnd::RemoveSound(zCActiveSnd* sound)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i] == sound) {
            for (int j = i + 1; j < activeSndList.numInArray; j++)
                activeSndList.array[j - 1] = activeSndList.array[j];
            activeSndList.numInArray--;
            sound->Release();
            return;
        }
    }
}

int zCActiveSnd::GetNumActive()
{
    return activeSndList.numInArray;
}

void zCActiveSnd::RemoveSoundsByFrame(zCSndFrame* frame)
{
    for (int i = activeSndList.numInArray - 1; i >= 0; i--) {
        if (activeSndList.array[i]->sourceFrm == frame)
            RemoveSound(activeSndList.array[i]);
    }
}

void zCActiveSnd::RemoveSoundByVobSlot(const zCVob* vob, int slot)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->sourceVob == vob && activeSndList.array[i]->vobSlot == slot) {
            RemoveSound(activeSndList.array[i]);
            return;
        }
    }
}

void zCActiveSnd::StopSoundsByFrame(zCSndFrame* frame)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->sourceFrm == frame)
            activeSndList.array[i]->active = 0;
    }
}

void zCActiveSnd::ResumeSoundsByFrame(zCSndFrame* frame)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->sourceFrm == frame)
            activeSndList.array[i]->active = 1;
    }
}

void zCActiveSnd::UpdateSoundsByFrame(zCSndFrame* frame)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        zCActiveSnd* sound = activeSndList.array[i];
        if (sound->sourceFrm != frame)
            continue;

        frame->SetDefaultProperties();
        sound->rate = frame->CalcPitchVariance();
        sound->volume = unsigned char(frame->actVolume * sound->volWeight);
        sound->looping = frame->looping;
        sound->active = sound->volume > 0;
    }
}

void zCActiveSnd::NewFrame()
{
    for (int i = activeSndList.numInArray - 1; i >= 0; i--) {
        zCActiveSnd* sound = activeSndList.array[i];
        sound->age++;

        if (!sound->active) {
            sound->muteTime += ztimer.frameTime;
            if (!sound->looping || sound->muteTime > 3000)
                RemoveSound(sound);
        }
        else if (autoUpdate3D && sound->sourceVob)
            zsound->UpdateSound3D(sound->handle, NULL);
    }
}

void zCActiveSnd::UpdateReverb(float weight)
{
    for (int i = 0; i < activeSndList.numInArray; i++) {
        zCActiveSnd* sound = activeSndList.array[i];
        if (sound->active && sound->is3D)
            sound->reverbLevel = weight > 0.0f ? weight : -1.0f;
    }
}

int zCActiveSnd::CalcListenerPan()
{
    return 64;
}

int zCActiveSnd::CalcListenerVolume()
{
    volume = unsigned char(volume * volWeight);
    return volume;
}

int zCActiveSnd::GetNum3DSamplesUsed()
{
    int result = 0;
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (activeSndList.array[i]->is3D && activeSndList.array[i]->sample3D)
            result++;
    }
    return result;
}

int zCActiveSnd::GetNumAmbientSamplesUsed()
{
    int result = 0;
    for (int i = 0; i < activeSndList.numInArray; i++) {
        if (!activeSndList.array[i]->is3D && activeSndList.array[i]->sample)
            result++;
    }
    return result;
}

int zCActiveSnd::RequestChannel()
{
    if (is3D)
        return RequestChannel3D();

    sample = (void*)1;
    active = 1;
    return 1;
}

int zCActiveSnd::RequestChannel3D()
{
    sample3D = (void*)1;
    active = 1;
    return 1;
}

float zCActiveSnd::GetVolume()
{
    if (!is3D)
        return sourceFrm->actVolume / 127.0f;
    if (!sourceVob)
        return 0.0f;
    return sourceFrm->actVolume / 127.0f * (1.0f - obstruction);
}

void zCActiveSnd::DebugInfo()
{
    zerr->Message("listing active sounds");

    for (int i = 0; i < activeSndList.numInArray; i++) {
        zCActiveSnd* sound = activeSndList.array[i];
        zerr->Message(zSTRING(sound->handle) + (sound->is3D ? " is a 3d sound" : " is a 2d sound"));
    }
}

void zCActiveSnd::InitPreAllocedData()
{
    for (int i = 0; i < 50; i++)
        preAllocedSndList.InsertEnd(new zCActiveSnd());
}

void zCActiveSnd::CleanupPreAllocedData()
{
    for (int i = preAllocedSndList.numInArray - 1; i >= 0; i--)
        delete preAllocedSndList.parray[i];
    preAllocedSndList.DeleteList();
    nextFreeSnd = NULL;
}

int zCActiveSnd::HasBeenUpdatedThisFrame()
{
    return 1;
}

zCSndChannel::zCSndChannel()
{
    actFrame = -1;
    refCtr = 1;
    loaded = 0;
}

zCSndChannel::~zCSndChannel()
{
    for (int i = 0; i < frameList.numInArray; i++) {
        zCActiveSnd::RemoveSoundsByFrame(frameList.parray[i]);
        delete frameList.parray[i]->wav;
        delete frameList.parray[i];
    }

    frameList.DeleteList();
}

void zCActiveSnd::AutoCalcObstruction(int immediately)
{
    zCSkyControler* skyControler = zCSkyControler::s_activeSkyControler;
    if (skyControler && !zCObject::CheckInheritance(zCSkyControler_Outdoor::classDef,
            skyControler->_GetClassDef()))
        skyControler = NULL;

    if (skyControler && sourceVob) {
        zCSkyControler_Outdoor* outdoor = static_cast<zCSkyControler_Outdoor*>(skyControler);
        const int locationHint = outdoor->rainFX.camLocationHint;
        const int outside = sourceVob->groundPoly == NULL;
        float newVolume = 1.0f;
        float newObstruction = 0.0f;

        if (locationHint == zCSkyControler::zCAM_INSIDE_SECTOR_CAN_SEE_OUTSIDE && outside) {
            newVolume = 0.7f;
            newObstruction = 0.3f;
        }
        else if (locationHint == zCSkyControler::zCAM_INSIDE_SECTOR_CANT_SEE_OUTSIDE && outside) {
            newVolume = 0.5f;
            newObstruction = 0.5f;
        }
        else if (locationHint == zCSkyControler::zCAM_OUTSIDE_SECTOR && !outside) {
            newVolume = 0.6f;
            newObstruction = 0.5f;
        }

        if (volumeToGo != newVolume || obstructionToGo != newObstruction) {
            volumeToGo = newVolume;
            obstructionToGo = newObstruction;
            autoObstructTimer = 0;
        }
    }

    autoObstructTimer += ztimer.frameTime;
    if (autoObstructTimer > 1000 || immediately)
        autoObstructTimer = 1000;

    const float weight = (zSinApprox(autoObstructTimer * 0.001f * 3.14159274f
        - 1.57079637f) + 1.0f) * 0.5f;
    volWeight += (volumeToGo - volWeight) * weight;
    obstruction += (obstructionToGo - obstruction) * weight;
}
