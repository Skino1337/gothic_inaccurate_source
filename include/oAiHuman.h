#ifndef OAIHUMAN_H
#define OAIHUMAN_H

#include "oCollisionObjectMiscChar.h"

class oCAIHuman : public oCAniCtrl_Human {
public:

    oCAIHuman();
    virtual ~oCAIHuman();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Init(oCNpc* npc);
    int DoSimpleAI();
    int Pressed(int key);
    int Toggled(int key);
    void SetCamModeMob(oCMobInter* mob);
    void ChangeCamMode(const zSTRING& mode);
    void InitCamModes(int weaponMode);
    void SetCamMode(const zSTRING& mode, int immediate);
    void WeaponChoose();
    int BowMode(int oldControls);
    int GetEnemyThreat();
    void CheckFightCamera();
    void SetSprintMode(int enabled);
    void SetCrawlMode(int enabled);
    int FightMelee();
    int CheckMobInteraction();
    int CheckItemInteraction();
    int StandActions();
    int PC_CheckSpecialStates();
    void PC_Diving();
    void PC_Turn(float angle, int animate);
    int PC_SpecialMove(int enabled);
    int PC_ActionMove(int oldControls);
    int PC_WeaponMove(int enabled);
    int PC_SlowMove(int enabled);
    int PC_Sneak(int enabled);
    void PC_Turnings(int oldControls);
    int PC_Strafe(int enabled);
    void _WalkCycle();
    void ResetObserveSuspectCounter();
    void CreateObserveSuspect(int reset);
    void CreateFootStepSound(int reset);
    void ResetAssessUseMobCounter();
    void CreateAssessUseMob();
    int MoveCamera();
    void CamMessages();
    void CheckFocusVob(int oldControls);
    int CheckActiveSpells();
    int MagicInvestSpell();
    int MagicCheckSpellStates(int spellID);
    int MagicMode();
    void Moving();
    virtual void DoAI(zCVob* vob, int& state);
    virtual void DoHackSpecials();
    void ChangeCamModeBySituation();
    void DoMonsterSpecial();
    void SetShowAI(int enabled);
    int GetShowAI();
    virtual int CanThisCollideWith(zCVob* vob);
    virtual void AddIgnoreCD(zCVob* vob);
    virtual void SubIgnoreCD(zCVob* vob);
    void StartFlyDamage(float damage, zVEC3& direction);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCArray<zCVob*> ignoreVobList;
    void* aiCam;
    int forcejump;
    int lookedAround;
    int sprintActive;
    int crawlActive;
    int showai;
    int startObserveIntruder;
    int dontKnowAniPlayed;
    int spellReleased;
    int spellCastedLastFrame;
    int eyeBlinkActivated;
    int thirdPersonFallback;
    float createFlyDamage;

    static zCClassDef* classDef;
    static zSTRING Cam_Normal;
    static zSTRING Cam_Run;
    static zSTRING Cam_Fight;
};

class oCAICamera {
public:

    oCAICamera();
    virtual ~oCAICamera();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int HasAIDetectedCollision();
    virtual void DoAI(zCVob* vob, int& state);
    zSTRING GetPreviousMode();

    zSTRING oldCamSys;

    static zCClassDef* classDef;
};

class oCAIHuman_Stand {
public:

    oCAIHuman_Stand(zCVob* vob);
    virtual ~oCAIHuman_Stand();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void DoAI(zCVob* vob, int& state);

    static zCClassDef* classDef;
};

#endif
