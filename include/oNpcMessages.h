#ifndef ONPCMESSAGES_H
#define ONPCMESSAGES_H

#include "zVob.h"

class oCNpcMessage : public zCEventMessage {
public:

    oCNpcMessage()
        : highPriority(0), deleted(0), inUse(0)
    {
    }
    virtual ~oCNpcMessage() {}
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int IsOverlay();
    virtual int IsHighPriority();
    virtual int IsJob();
    virtual int GetIgnoreCutsceneMode();
    virtual void Delete();
    virtual int IsDeleteable();
    virtual int IsDeleted();
    virtual void SetInUse(int value);
    virtual int IsInUse();
    virtual void SetHighPriority(int value);

    zSTRING targetVobName;
    int highPriority : 1;
    int deleted : 1;
    int inUse : 1;

    static zCClassDef* classDef;
};

class oCMsgMovement : public oCNpcMessage {
public:

    oCMsgMovement();
    oCMsgMovement(int type, const zSTRING& name);
    oCMsgMovement(int type, zCVob* vob);
    oCMsgMovement(int type, const zVEC3& position);
    oCMsgMovement(int type, float angle);
    oCMsgMovement(int type, int mode);
    virtual ~oCMsgMovement() {}
    int         IsInUse() const { return inUse; }
    void        SetInUse(int value) { inUse = value; }
    static void* operator new(unsigned int size);
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    void Init();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int type);
    virtual zSTRING MD_GetVobRefName();
    virtual void MD_SetVobRefName(const zSTRING& name);
    virtual void MD_SetVobParam(zCVob* vob);
    virtual void Delete();
    virtual zCEventMessage::zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();

    zCVob*  targetVob;
    zVEC3   targetPos;
    zSTRING targetName;
    int     targetMode;
    int     ani;
    int     inUse;
    float   timer;
    float   angle;
    void*   route;

    static zCClassDef* classDef;
};

class oCMsgAttack : public oCNpcMessage {
public:

    oCMsgAttack();
    oCMsgAttack(int type, int combo, int hitAni);
    oCMsgAttack(int type, zCVob* target, float startFrame);
    virtual ~oCMsgAttack();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsNetRelevant();
    virtual int IsOverlay();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int type);
    int IsInUse() const { return inUse; }
    void SetInUse(int value) { inUse = value; }

    zCVob* attackTarget;
    zCVob* target;
    int attackSubType;
    int attackAni;
    float hitChance;
    int duringRun;
    int inUse;
    int combo;
    int hitAni;
    float startFrame;
    int enableNextHit;
    int reachedTarget;

    static zCClassDef* classDef;
};

class oCMsgMagic : public oCNpcMessage {
public:

    oCMsgMagic();
    oCMsgMagic(int type);
    oCMsgMagic(int subType, int spellID, int remove);
    virtual ~oCMsgMagic();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsJob();
    virtual int IsHighPriority();
    virtual int IsNetRelevant();
    virtual void Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int type);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    int spellID;
    zCVob* target;
    zVEC3 targetPos;
    int what;
    int removeSymbol;
    int level;
    int manaInvested;
    int energyLeft;
    int activeSpell;

    static zCClassDef* classDef;
};

class oCNpc;

class oCMsgState : public oCNpcMessage {
public:

    oCMsgState();
    oCMsgState(int type, float timer);
    oCMsgState(int type, int function);
    oCMsgState(int type, int function, int endOldState, const zSTRING& wp);
    virtual ~oCMsgState();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsOverlay();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int type);
    virtual zCEventMessage::zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();

    zSTRING wp;
    int function;
    int instance;
    int minutes;
    oCNpc* other;
    oCNpc* victim;
    int endOldState;
    float timer;

    static zCClassDef* classDef;
};

class oCMsgConversation : public oCNpcMessage {
public:

    oCMsgConversation();
    oCMsgConversation(int type);
    oCMsgConversation(int type, const zSTRING& name);
    oCMsgConversation(int type, const zSTRING& text, const zSTRING& name);
    oCMsgConversation(int type, zCVob* target);
    oCMsgConversation(int type, int ani);
    oCMsgConversation(int type, int number, int no, float timer);
    virtual ~oCMsgConversation();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int MD_GetNumOfSubTypes();
    virtual void Delete();
    virtual int IsOverlay();
    virtual zSTRING MD_GetSubTypeString(int type);
    virtual zSTRING MD_GetVobRefName();
    virtual void MD_SetVobRefName(const zSTRING& name);
    virtual void MD_SetVobParam(zCVob* vob);
    virtual zCEventMessage::zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();
    int IsInUse() const { return inUse; }
    void SetInUse(int value) { inUse = value; }

    zSTRING text;
    zSTRING name;
    zCVob* target;
    zVEC3 targetPos;
    int ani;
    int handle;
    float timer;
    int number;
    int f_no;
    int f_yes;
    int inUse;

    static zCClassDef* classDef;
};

class oCItem;

class oCMsgUseItem : public oCNpcMessage {
public:

    oCMsgUseItem();
    oCMsgUseItem(int type, zCVob* vob);
    virtual ~oCMsgUseItem();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsNetRelevant();
    virtual void Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    int IsInUse() const { return inUse; }
    void SetInUse(int value) { inUse = value; }

    oCItem* item;
    zCVob* vob;
    int ani;
    int state;
    int inUse;

    static zCClassDef* classDef;
};

class oCMsgManipulate : public oCNpcMessage {
public:

    oCMsgManipulate();
    oCMsgManipulate(int type);
    oCMsgManipulate(int type, zCVob* vob);
    oCMsgManipulate(int type, int state);
    oCMsgManipulate(int type, const zSTRING& name, int state);
    oCMsgManipulate(int type, zCVob* vob, int state);
    oCMsgManipulate(int type, const zSTRING& name, const zSTRING& slot);
    virtual ~oCMsgManipulate();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsNetRelevant();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int type);
    virtual zCEventMessage::zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();
    virtual zSTRING MD_GetVobRefName();
    virtual void MD_SetVobRefName(const zSTRING& name);
    virtual void MD_SetVobParam(zCVob* vob);
    int IsInUse() const { return inUse; }
    void SetInUse(int value) { inUse = value; }

    zCVob* targetVob;
    zSTRING name;
    zSTRING slot;
    float aniCombY;
    int targetState;
    int flag;
    int state;
    int inUse;

    static zCClassDef* classDef;
};

class oCMsgWeapon : public oCNpcMessage {
public:

    oCMsgWeapon();
    oCMsgWeapon(int type, int mode, int run);
    virtual ~oCMsgWeapon();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int type);
    virtual zCEventMessage::zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();
    int IsInUse() const { return inUse; }
    void SetInUse(int value) { inUse = value; }

    int targetMode;
    int duringRun;
    int initDone;
    int firstTime;
    int useFist;
    int showMagicCircle;
    int ani;
    int inUse;

    static zCClassDef* classDef;
    static void operator delete(void* memory);
};

class zCEvMsgCutscene : public oCNpcMessage {
public:

    zSTRING GetCutsceneName();

    zSTRING csName;
};

#endif
