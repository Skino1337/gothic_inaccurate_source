#ifndef OAISHOOT_H
#define OAISHOOT_H

#include "oCollisionObjectMiscChar.h"
#include "zSoundMan.h"

class zCCollisionReport;

class oCAISound : public zCObject {
public:

    oCAISound();
    virtual ~oCAISound();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void RemoveSlideSound();
    void CheckSlideSound(zCVob* vob);
    int GetSoundMaterial(zCVob* vob, zCSoundManager::zTSndManMedium& medium);
    void StartCollisionSound(zCVob* vob, const zCCollisionReport& report);

    int slideSoundHandle;
    int slideSoundOn;

    static zCClassDef* classDef;
};

class oCAIVobMove : public oCAISound {
public:

    oCAIVobMove();
    virtual ~oCAIVobMove();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int CanThisCollideWith(zCVob* vob);
    virtual void Init(zCVob* vob, zCVob* owner, zVEC3& position, float speedXY,
        float speedZ, zMAT4* trajectory);
    virtual void DoAI(zCVob* vob, int& state);
    virtual void ReportCollisionToAI(const zCCollisionReport& report);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void AddIgnoreCDVob(zCVob* vob);
    void ClearIgnoreCDVob();

    zCVob* vob;
    zCVob* owner;
    zCArray<zCVob*> ignoreVobList;

    static zCClassDef* classDef;
};

class oCAIVobMoveTorch : public oCAIVobMove {
public:

    oCAIVobMoveTorch();
    virtual ~oCAIVobMoveTorch();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Init(zCVob* vob, zCVob* owner, zVEC3& position, float speedXY,
        float speedZ, zMAT4* trajectory);
    virtual void DoAI(zCVob* vob, int& state);
    int CheckWater();
    void BurnedOut();

    float timer;

    static zCClassDef* classDef;
};

class zCPolyStrip;

class oCAIArrowBase : public oCAISound {
public:

    oCAIArrowBase();
    virtual ~oCAIArrowBase();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int HasAIDetectedCollision();
    virtual void AICollisionResponseSelfDetected(const zVEC3& normal, int& state);
    virtual int GetIsProjectile();
    void AddIgnoreCDVob(zCVob* vob);
    void ClearIgnoreCDVob();
    void CreateTrail(zCVob* vob);
    virtual void DoAI(zCVob* vob, int& state);
    virtual void ReportCollisionToAI(const zCCollisionReport& report);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCArray<zCVob*> ignoreVobList;
    int collisionOccured;
    float timeLeft;
    zCVob* vob;
    int startDustFX;
    zCVob* trailVob;
    zCPolyStrip* trailStrip;
    int trailActive;
    float trailTime;
    int hasHit;

    static zCClassDef* classDef;
};

class oCAIArrow : public oCAIArrowBase {
public:

    oCAIArrow();
    virtual ~oCAIArrow();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void SetTarget(zCVob* target);
    void ClearUsedVobs();
    void SetupAIVob(zCVob* vob, zCVob* owner, zCVob* target);
    virtual void DoAI(zCVob* vob, int& state);
    virtual int CanThisCollideWith(zCVob* vob);
    virtual void ReportCollisionToAI(const zCCollisionReport& report);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    oCItem* arrow;
    oCNpc* owner;
    int removeVob;
    zCVob* targetNPC;

    static zCClassDef* classDef;
};

class oCAIDrop : public oCAISound {
public:

    oCAIDrop();
    virtual ~oCAIDrop();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int HasAIDetectedCollision();
    virtual int CanThisCollideWith(zCVob* vob);
    void AddIgnoreCDVob(zCVob* vob);
    void ClearIgnoreCDVob();
    void SetupAIVob(zCVob* vob, zCVob* owner);
    virtual void DoAI(zCVob* vob, int& state);
    virtual void ReportCollisionToAI(const zCCollisionReport& report);
    void SetVelocity(float speed, float angle);
    void SetStartPosition(zVEC3& position);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCArray<zCVob*> ignoreVobList;
    zCVob* vob;
    zCVob* owner;
    int collisionOccured;
    float timer;
    float count;

    static zCClassDef* classDef;
};

#endif
