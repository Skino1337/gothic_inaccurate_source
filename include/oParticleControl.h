#ifndef OPARTICLECONTROL_H
#define OPARTICLECONTROL_H

#include "zParticle.h"
#include "zConsole.h"

class oCParticleControl {
public:

    oCParticleControl();
    oCParticleControl(zCWorld* world);
    ~oCParticleControl();
    zCVob* GetPFXVob();
    void SelectActivePFX();
    void EditActivePFX();
    void SetStartPos(zVEC3& position);
    void StartActivePFX(zCVob* vob, zSTRING& nodeName);
    void StartActivePFX();
    void CreateNewPFX(zSTRING& name);
    void SetPFXName(const zSTRING& name);
    void SetWorld(zCWorld* world);
    void BeginEditActive(void(*changedFunc)(const zSTRING&));
    void DeleteLastPFX();
    void UpdateInternals();
    void EndEditActive();

    zCParticleFX* partFX;
    zCVob* pfxVob;
    unsigned long pfxVobID;
    zSTRING pfxName;
    zVEC3 startpos;
    int IsInEditMode;
    zCWorld* rnd_world;
    zCConsole* part_edit;
};

#endif
