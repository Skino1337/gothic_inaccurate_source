#ifndef OMOB_H
#define OMOB_H

#include "zVob.h"
#include "oMobMsg.h"
#include "oDocumentManager.h"

class oCNpc;
class oCItem;
class oCItemContainer;

class oCMOB : public zCVob {
public:

    oCMOB();
    virtual ~oCMOB();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    void SetMoveable(int value);
    int  IsMoveable();
    void SetOwner(const zSTRING& ownerName, const zSTRING& guildName);
    void SetOwner(int ownerInstance, int guild);
    int  IsOwner(oCNpc* npc);
    void Hit(int damage);
    void InsertInIgnoreList(zCVob* vob);
    void RemoveFromIgnoreList(zCVob* vob);

    virtual zCClassDef*   _GetClassDef() const;
    virtual void          OnMessage(zCEventMessage* message, zCVob* sender);
    virtual int           CanThisCollideWith(zCVob* vob);
    virtual void          SetVisual(zCVisual* visual);
    virtual int           IsOwnedByGuild(int guild);
    virtual int           IsOwnedByNpc(int npcInstance);
    virtual void          GetSoundMaterial_AM(zCSoundManager::zTSndManMedium& medium,
        oTSndMaterial& material, int damageType);
    virtual void          SetSoundMaterial(oTSndMaterial material);
    virtual oTSndMaterial GetSoundMaterial();
    virtual void          SetName(const zSTRING& value);
    virtual zSTRING       GetName();
    virtual zCModel*      GetModel();
    virtual zSTRING       GetScemeName();
    virtual void          Destroy();

    zSTRING       name;
    int           hitp : 12;
    int           damage : 12;
    int           isDestroyed : 1;
    int           moveable : 1;
    int           takeable : 1;
    int           focusOverride : 1;
    oTSndMaterial sndMat : 3;
    zSTRING       visualDestroyed;
    zSTRING       ownerStr;
    zSTRING       ownerGuildStr;
    int           owner;
    int           ownerGuild;
    int           focusNameIndex;
    zCList<zCVob> ignoreVobList;

    static zCClassDef* classDef;
};

class oCWorld;

struct TMobOptPos {
    ~TMobOptPos() {}

    zMAT4   trafo;
    int     distance;
    oCNpc*  npc;
    zSTRING nodeName;
};

class oCMobInter : public oCMOB {
public:

    enum TMobInterDirection {
        MOBINTER_DIRECTION_NONE,
        MOBINTER_DIRECTION_UP,
        MOBINTER_DIRECTION_DOWN
    };

    oCMobInter();
    virtual ~oCMobInter();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    static int       SetAllMobsToState(oCWorld* world, const zSTRING& scemeName, int state);
    static void      TraverseMobs(zCTree<zCVob>* node, const zSTRING& scemeName, int state,
        int* count);
    static void      TriggerAllMobsToTmpState(zCWorld* world);
    static void      TraverseTriggerMobs(zCTree<zCVob>* node);

    void SetTempState(int state);
    int  IsTempStateChanged();
    void SetStateToTempState();
    void SetMobBodyState(oCNpc* npc);
    int  HasUseWithItem(oCNpc* npc);
    void ScanIdealPositions();
    int  GetFreePosition(oCNpc* npc, zVEC3& position);
    void SetHeading(oCNpc* npc);
    void SetIdealPosition(oCNpc* npc);
    void SendStateChange(int from, int to);
    void SendEndInteraction(oCNpc* npc, int state, int playAni);
    void StartTransitionAniNpc(oCNpc* npc, const zSTRING& aniName);
    int  IsMultiMob();
    int  IsAvailable(oCNpc* npc);
    void MarkAsUsed(float time, zCVob* vob);

    virtual zCClassDef*         _GetClassDef() const;
    virtual void                InterruptInteraction(oCNpc* npc);
    virtual int                 CanInteractWith(oCNpc* npc);
    virtual int                 AI_UseMobToState(oCNpc* npc, int state);
    virtual void                Archive(zCArchiver& archiver);
    virtual void                Unarchive(zCArchiver& archiver);
    virtual void                Interact(oCNpc* npc, int apply, int up, int down,
        int keyState, int keyToggle);
    virtual void                GetTransitionNames(int from, int to, zSTRING& mobAni,
        zSTRING& npcAni);
    virtual int                 IsInState(oCNpc* npc, int state);
    virtual int                 CanChangeState(oCNpc* npc, int from, int to);
    virtual void                CheckStateChange(oCNpc* npc);
    virtual void                OnBeginStateChange(oCNpc* npc, int from, int to);
    virtual void                OnEndStateChange(oCNpc* npc, int from, int to);
    virtual void                EndInteraction(oCNpc* npc, int endMode);
    virtual int                 IsInteractingWith(oCNpc* npc);
    virtual void                Reset();
    virtual void                OnTick();
    virtual void                OnTrigger(zCVob* other, zCVob* instigator);
    virtual void                OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void                SetVisual(zCVisual* visual);
    virtual void                OnDamage(zCVob* other, zCVob* instigator, float damage,
        int damageType, const zVEC3& hitLocation);
    virtual void                OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void                SetUseWithItem(const zSTRING& itemName);
    virtual int                 GetUseWithItem();
    virtual zSTRING             GetScemeName();
    virtual const zSTRING*      GetTriggerTarget(int index) const;
    virtual int                 GetState();
    virtual int                 GetStateNum();
    virtual TMobInterDirection  GetDirection();
    virtual void                SetDirection(TMobInterDirection direction);
    virtual int                 IsOccupied();
    virtual int                 IsIn(int state);
    virtual void                StopInteraction(oCNpc* npc);
    virtual void                StartInteraction(oCNpc* npc);
    virtual void                SetMobBodyState(int state);
    virtual void                StartStateChange(oCNpc* npc, int from, int to);
    virtual void                CallOnStateFunc(oCNpc* npc, int state);
    virtual void                SendCallOnStateFunc(oCNpc* npc, int state);
    virtual TMobOptPos*         SearchFreePosition(oCNpc* npc, float maxDistance);

    zCList<TMobOptPos> optimalPosList;
    zSTRING            triggerTarget;
    zSTRING            useWithItem;
    zSTRING            sceme;
    zSTRING            conditionFunc;
    zSTRING            onStateFuncName;
    int                state;
    int                state_num;
    int                state_target;
    int                rewind;
    int                mobStateAni;
    int                npcStateAni;
    int                npcsMax : 8;
    int                npcsNeeded : 8;
    int                npcsCurrent : 8;
    int                tmpState : 8;
    int                tmpStateChanged;
    TMobInterDirection Direction;
    int                onInterruptReturnToSlotPos : 1;
    zVEC3              startPos;
    float              aniCombHeight;
    zCVob*             inUseVob;
    float              timerEnd;

    static zCClassDef* classDef;
};

class oCMobBed : public oCMobInter {
public:

    oCMobBed();
    virtual ~oCMobBed();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        StartInteraction(oCNpc* npc);
    virtual void        OnBeginStateChange(oCNpc* npc, int from, int to);
    virtual void        OnEndStateChange(oCNpc* npc, int from, int to);
    virtual zSTRING     GetScemeName();
    virtual TMobOptPos* SearchFreePosition(oCNpc* npc, float maxDistance);

    zSTRING addName;

    static zCClassDef* classDef;
};

class oCMobSwitch : public oCMobInter {
public:

    oCMobSwitch();
    virtual ~oCMobSwitch();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);

    static zCClassDef* classDef;
};

class oCMobLockable : public oCMobInter {
public:

    oCMobLockable();
    virtual ~oCMobLockable();

    int CanOpen(oCNpc* npc);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void        Interact(oCNpc* npc, int apply, int up, int down, int keyState,
        int keyToggle);
    virtual int         CanInteractWith(oCNpc* npc);
    virtual int         CanChangeState(oCNpc* npc, int from, int to);
    virtual void        OnEndStateChange(oCNpc* npc, int from, int to);
    virtual void        SetLocked(int value);
    virtual void        SetKeyInstance(const zSTRING& value);
    virtual void        SetPickLockStr(const zSTRING& value);
    virtual void        Open(oCNpc* npc) = 0;
    virtual void        Close(oCNpc* npc) = 0;
    virtual void        Lock(oCNpc* npc);
    virtual void        Unlock(oCNpc* npc, int playAni);
    virtual int         PickLock(oCNpc* npc, char direction);
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);

    int     locked : 1;
    int     autoOpen : 1;
    int     pickLockNr : 30;
    zSTRING keyInstance;
    zSTRING pickLockStr;

    static zCClassDef* classDef;
};

class oCMobContainer : public oCMobLockable {
public:

    oCMobContainer();
    virtual ~oCMobContainer();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void        Destroy();
    virtual void        Reset();
    virtual int         IsIn(int instance);
    virtual void        Open(oCNpc* npc);
    virtual void        Close(oCNpc* npc);
    virtual void        Insert(oCItem* item);
    virtual void        Remove(oCItem* item);
    virtual oCItem*     Remove(oCItem* item, int amount);
    virtual void        CreateContents(const zSTRING& contents);
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);

    zSTRING           contains;
    oCItemContainer*  items;
    zCListSort<oCItem> containList;

    static zCClassDef* classDef;
};

class oCMobWheel : public oCMobInter {
public:

    oCMobWheel();
    virtual ~oCMobWheel();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        OnTrigger(zCVob* other, zCVob* instigator);
    virtual void        OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void        Interact(oCNpc* npc, int apply, int up, int down, int keyState,
        int keyToggle);

    static zCClassDef* classDef;
};

class oCMobLadder : public oCMobInter {
public:

    oCMobLadder();
    virtual ~oCMobLadder();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual int         DoFocusCheckBBox();
    virtual void        Interact(oCNpc* npc, int apply, int up, int down, int keyState,
        int keyToggle);
    virtual void        EndInteraction(oCNpc* npc, int endMode);
    virtual int         CanInteractWith(oCNpc* npc);
    virtual void        StartInteraction(oCNpc* npc);
    virtual int         CanChangeState(oCNpc* npc, int from, int to);
    virtual TMobOptPos* SearchFreePosition(oCNpc* npc, float maxDistance);

    int Interacting;
    int PrevAction;

    static zCClassDef* classDef;
};

class oCItem;

class oCMobDoor : public oCMobLockable {
public:

    oCMobDoor();
    virtual ~oCMobDoor();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual zSTRING     GetScemeName();
    virtual TMobOptPos* SearchFreePosition(oCNpc* npc, float maxDistance);
    virtual void        Open(oCNpc* npc);
    virtual void        Close(oCNpc* npc);

    zSTRING addName;

    static zCClassDef* classDef;
};

class oCMobFire : public oCMobInter {
public:

    oCMobFire();
    virtual ~oCMobFire();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    void DeleteEffects();

    virtual zCClassDef* _GetClassDef() const;
    virtual void        OnTrigger(zCVob* other, zCVob* instigator);
    virtual void        OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void        OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
        const zVEC3& hitLocation);
    virtual void        OnEndStateChange(oCNpc* npc, int from, int to);
    virtual void        PreSave();
    virtual void        PostSave();
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);

    zSTRING fireSlot;
    zSTRING fireVobtreeName;
    zCVob*  fireVobtree;

    static zCClassDef* classDef;
};

class oCMobItemSlot : public oCMobInter {
public:

    oCMobItemSlot();
    virtual ~oCMobItemSlot();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual int         GetUseWithItem();
    virtual int         CanInteractWith(oCNpc* npc);
    virtual int         IsIn(int instance);
    virtual oCItem*     GetInsertedItem();
    virtual int         PlaceItem(oCItem* item);
    virtual oCItem*     RemoveItem();
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);

    oCItem* insertedItem;
    int     removeable;

    static zCClassDef* classDef;
};

class oCDummyVobGenerator : public zCVob {
public:

    oCDummyVobGenerator();
    virtual ~oCDummyVobGenerator();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        OnTrigger(zCVob* other, zCVob* instigator);

    static zCClassDef* classDef;
};

#endif
