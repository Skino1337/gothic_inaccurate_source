#ifndef ODIALOG_H
#define ODIALOG_H

#include "oBarrier.h"

class oCNpc;

class zCAIBase : public zCObject {
public:

    virtual ~zCAIBase();
    virtual zCClassDef* _GetClassDef() const;
    virtual void DoAI(zCVob* vob, int& deleteAI);
    virtual int CanThisCollideWith(zCVob* vob);
    virtual void ReportCollisionToAI(const zCCollisionReport& report);
    virtual int HasAIDetectedCollision();
    virtual void AICollisionResponseSelfDetected(const zVEC3& normal, int& collisionDetected);
    virtual void HostVobRemovedFromWorld(zCVob* vob, zCWorld* world);
    virtual void HostVobAddedToWorld(zCVob* vob, zCWorld* world);
    virtual int GetIsProjectile();

    static zCClassDef* classDef;
    static void operator delete(void* memory);
};

class oCAskBox : public zCAIBase {
public:

    oCAskBox();
    virtual ~oCAskBox();
    void Init(oCNpc* firstNpc, oCNpc* secondNpc);
    void Delete();
    virtual void DoAI(zCVob* vob, int& deleteAI);
    void SetNeutral();
    void SetYes();
    void SetNo();
    void SetPosition();

    int fadeout;
    int answer;
    int alpha;
    zCVob* askvob;
    oCNpc* npc1;
    oCNpc* npc2;
    zCDecal* thumb[3];
};

#endif
