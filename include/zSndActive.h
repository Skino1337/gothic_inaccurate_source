#ifndef ZSND_ACTIVE_H
#define ZSND_ACTIVE_H

#include "zSndFxMss.h"
#include "zTools.h"
#include "zModel.h"

class zCActiveSnd {
public:

    zCActiveSnd();
    ~zCActiveSnd();
    void  Release();
    void  Initialize();
    static int GetNextFreeHandle();
    static zCActiveSnd* AllocNextFreeSnd();
    static int GetHandleIndex(int handle);
    static int GetHandleActive(int handle);
    static int GetSoundActive(zCActiveSnd* sound);
    static int GetFrameActive(zCSndFrame* frame);
    static zCActiveSnd* GetHandleSound(int handle);
    static void AddSound(zCActiveSnd* sound);
    static void RemoveSound(zCActiveSnd* sound);
    static void RemoveSoundsByFrame(zCSndFrame* frame);
    static void RemoveSoundByVobSlot(const zCVob* vob, int slot);
    static void StopSoundsByFrame(zCSndFrame* frame);
    static void ResumeSoundsByFrame(zCSndFrame* frame);
    static void UpdateSoundsByFrame(zCSndFrame* frame);
    static void NewFrame();
    static void UpdateReverb(float weight);
    int   CalcListenerPan();
    int   CalcListenerVolume();
    static int GetNum3DSamplesUsed();
    static int GetNumAmbientSamplesUsed();
    int   RequestChannel();
    int   RequestChannel3D();
    float GetVolume();
    static void DebugInfo();
    static void InitPreAllocedData();
    static void CleanupPreAllocedData();
    int   HasBeenUpdatedThisFrame();
    static int GetNumActive();

    int handle;
    void* sample;
    void* sample3D;
    unsigned long age;
    zCSoundSystem::zTLoopType loopType;
    float radius;
    float reverbLevel;
    float pitchOffset;
    float volWeight;
    float obstruction;
    float obstructionToGo;
    float volumeToGo;
    int autoObstructTimer;
    int active;
    int looping;
    int isAmbient;
    int is3D;
    int allocated;
    int vobSlot;
    unsigned char pan;
    unsigned char volume;
    unsigned short rate;
    int muteTime;
    int frameCtr;
    zVEC3 lastPos;
    float lastTime;
    zCVob* sourceVob;
    zCSndFrame* sourceFrm;
    zCSndFX_MSS* sourceSnd;

    static zCArraySort<zCActiveSnd*> activeSndList;
    static unsigned long lastHandle;
    static int maxChannels;
    static zCArray<zCActiveSnd*> preAllocedSndList;
    static zCActiveSnd* nextFreeSnd;
    static int autoUpdate3D;
};

#endif
