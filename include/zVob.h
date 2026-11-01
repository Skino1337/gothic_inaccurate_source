#ifndef ZVOB_H
#define ZVOB_H

#include "zModel.h"
#include "zPhysics.h"

enum oTSndMaterial {
    SND_MAT_WOOD,
    SND_MAT_STONE,
    SND_MAT_METAL,
    SND_MAT_LEATHER,
    SND_MAT_CLAY,
    SND_MAT_GLAS,
    oTSNDMAT_DEFAULT = SND_MAT_WOOD
};

class zCEventMessage : public zCObject {
public:

    enum zTTimeBehavior {
        TBZero,
        TBFix,
        TBUnknown,
        TBAssembled
    };

    zCEventMessage();
    zCEventMessage(unsigned short subType);
    virtual ~zCEventMessage();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    static void      AnalyzeMessageID(unsigned long id, unsigned short& classID,
        unsigned short& subType);
    static zCEventMessage* CreateFromID(unsigned long id);
    static zCEventMessage* CreateFromBuffer(zCBuffer& buffer, zCEventManager* manager);
    static void      TestClassIntegrity();

    unsigned long GetMessageID() const;
    unsigned short GetSubType() const;
    void          PackToBuffer(zCBuffer& buffer, zCEventManager* manager);
    virtual void  Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void  Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual zCClassDef* _GetClassDef() const;
    virtual void  Archive(zCArchiver& archiver);
    virtual void  Unarchive(zCArchiver& archiver);
    virtual int   IsOverlay();
    virtual int   IsNetRelevant();
    virtual int   IsHighPriority();
    virtual int   IsJob();
    virtual int   GetIgnoreCutsceneMode();
    virtual void  Delete();
    virtual int   IsDeleteable();
    virtual int   IsDeleted();
    virtual void  SetCutsceneMode(int mode);
    virtual int   GetCutsceneMode();
    virtual int   MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int subType);
    virtual zSTRING MD_GetVobRefName();
    virtual void  MD_SetVobRefName(const zSTRING& name);
    virtual void  MD_SetVobParam(zCVob* vob);
    virtual zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();

    unsigned short subType;
    int            inCutscene;

    static zCClassDef* classDef;
};

class zCEventCore : public zCEventMessage {
public:

    zCEventCore();
    virtual ~zCEventCore();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int      IsNetRelevant();
    virtual int      MD_GetNumOfSubTypes();
    void             Clear();
    virtual void     Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void     Unpack(zCBuffer& buffer, zCEventManager* manager);
    zSTRING          MD_GetSubTypeString(int subType);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zCVob* otherVob;
    zCVob* vobInstigator;
    float  damage;
    int    damageType;
    zCVob* inflictorVob;
    zVEC3  hitLocation;

    static zCClassDef* classDef;
};

class zCEventCommon : public zCEventMessage {
public:

    enum zTEventCommonSubType {
        zEVENT_ENABLE,
        zEVENT_DISABLE,
        zEVENT_TOGGLE_ENABLED,
        zEVENT_RESET,
        zEVENT_MISC_NUM_SUBTYPES
    };

    zCEventCommon() {}
    virtual ~zCEventCommon();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual int      IsNetRelevant();
    virtual int      MD_GetNumOfSubTypes();
    virtual zSTRING  MD_GetSubTypeString(int subType);

    static zCClassDef* classDef;
};

class zCEventMover : public zCEventMessage {
public:

    enum zTEventMoverSubType {
        zEVENT_GOTO_KEY_FIXED_DIRECTLY,
        zEVENT_GOTO_KEY_FIXED_ORDER,
        zEVENT_GOTO_KEY_NEXT,
        zEVENT_GOTO_KEY_PREV,
        zEVENT_MISC_NUM_SUBTYPES
    };

    zCEventMover() {}
    virtual ~zCEventMover();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual int      IsNetRelevant();
    virtual int      MD_GetNumOfSubTypes();
    virtual zSTRING  MD_GetSubTypeString(int subType);

    int gotoFixedKeyframe;

    static zCClassDef* classDef;
};

class zCEventScreenFX : public zCEventMessage {
public:

    enum zTEventScreenFXSubType {
        zEVENT_BLEND_FADEIN,
        zEVENT_BLEND_FADEOUT,
        zEVENT_CINEMA_FADEIN,
        zEVENT_CINEMA_FADEOUT,
        zEVENT_FOV_MORPH,
        zEVENT_SCREENFX_COUNT
    };

    zCEventScreenFX();
    zCEventScreenFX(zTEventScreenFXSubType subType, float duration, zCOLOR color, float fovDeg);
    virtual ~zCEventScreenFX();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             Clear();
    virtual int      MD_GetNumOfSubTypes();
    virtual zSTRING  MD_GetSubTypeString(int subType);
    virtual int      MD_GetTimeBehavior();
    virtual float    MD_GetMinTime();
    virtual void     Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void     Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    float  duration;
    zCOLOR color;
    float  fovDeg;

    static zCClassDef* classDef;
};

class zCLensFlareFX : public zCObject {
public:

    enum zCLensFlareFXEnum1 {
        FM_GLOW_ONLY,
        FM_MIXED,
        FM_NO_GLOW
    };

    class zTVobLightLensFlare {
    public:

        enum zTFlareType {
            FT_CORONA,
            FT_GLOW,
            FT_FLARE
        };

        zTVobLightLensFlare();

        zCMaterial* lensFlareMaterial;
        zTFlareType flareType;
        float size;
        float alpha;
        float rangeMin;
        float rangeMax;
        float posScale;
        float fadeScale;
    };

    zCLensFlareFX() : flareMix(FM_MIXED), fadeScale(0.0f) {}
    virtual ~zCLensFlareFX();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    static void LoadLensFlareScript();
    static void ReleaseLensFlareScript();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void RenderLensFlares(zCVob* vob);
    void RenderLensFlares(zVEC2& screenPosition);
    void SetAlpha(int alpha);

    zCArray<zTVobLightLensFlare> lensFlareList;
    zCLensFlareFXEnum1 flareMix;
    float fadeScale;

    static zCClassDef classDef;
    static zCMesh* s_lensFlareMesh;
    static zCMesh* s_coronaMesh;
    static zCMesh* s_glowMesh;
};

class zCVobLightData {
public:

    zCVobLightData();
    ~zCVobLightData();

    int            SetLensFlareFXByName(const zSTRING& name);
    const zSTRING& GetLensFlareFXName() const;
    void           Archive(zCArchiver& archiver);
    void           Unarchive(zCArchiver& archiver);
    void           SetRange(float newRange, int resetBackup);

    zCArray<float> rangeAniScaleList;
    zCArray<zCOLOR> colorAniList;
    int            lensFlareFXNo;
    zCLensFlareFX* lensFlareFX;
    zCOLOR         lightColor;
    float          range;
    float          rangeInv;
    float          rangeBackup;
    float          rangeAniActFrame;
    float          rangeAniFPS;
    float          colorAniActFrame;
    float          colorAniFPS;
    float          spotConeAngleDeg;
    unsigned char  isStatic : 1;
    unsigned char  rangeAniSmooth : 1;
    unsigned char  rangeAniLoop : 1;
    unsigned char  colorAniSmooth : 1;
    unsigned char  colorAniLoop : 1;
    unsigned char  isTurnedOn : 1;
    unsigned char  lightQuality : 4;
    unsigned char  lightType : 4;
    unsigned char  m_bCanMove : 1;
};

template <class K, class V>
class zTRayTurboValMap {
public:

    struct zSNode {
        K       key;
        V       value;
        zSNode* next;
    };

    void InitHashTable(unsigned long size)
    {
        hashTable.DeleteList();
        hashTable.AllocAbs(size);
        for (unsigned long i = 0; i < size; ++i)
            hashTable.InsertEnd(static_cast<zSNode*>(0));
    }

    zCArray<zSNode*> hashTable;
};

class zCVobLightPreset : public zCObject {
public:

    zCVobLightPreset();
    virtual ~zCVobLightPreset();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zCVobLightData lightData;
    zSTRING        presetName;

    static zCClassDef* classDef;
};

class zCVobLight : public zCVob {
public:

    zCVobLight();
    virtual ~zCVobLight();

    static zCObject* _CreateNewInstance();
    static void      CleanupVobLight();
    static zCOLOR    SumLightsAtPositionWS(const zCArray<zCVobLight*>& lights,
        const zVEC3& position, int staticOnly);
    static void      LoadLightPresets();
    static void      SaveLightPresets();
    zCClassDef*      _GetClassDef() const;
    void             DoAnimation();
    void             SetRange(float range, int resetBackup);
    void             AddThisToPresetList(const zSTRING& presetName) const;
    int              SetByPreset(const zSTRING& presetName);
    int              SetByPresetInUse();
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     EndMovement(int flags);
    virtual void     ThisVobAddedToWorld(zCWorld* world);

    zCVobLightData                 lightData;
    zTRayTurboValMap<zCPolygon*, int> affectedPolyMap;
    zSTRING                        lightPresetInUse;

    static zCArray<zCVobLightPreset*> lightPresetList;
    static int                        renderLightVisuals;
    static zCClassDef*                classDef;
};

class zCCollObjectBoxPassThrough {
public:

    static zCCollisionObjectClass s_oCollObjClass;
};

class zCTriggerBase : public zCVob {
public:

    zCTriggerBase();
    virtual ~zCTriggerBase();

    zCClassDef*       _GetClassDef() const;
    virtual void      Archive(zCArchiver& archiver);
    virtual void      Unarchive(zCArchiver& archiver);
    virtual void      OnTrigger(zCVob* other, zCVob* instigator);
    virtual void      OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void      OnTouch(zCVob* other);
    virtual void      OnUntouch(zCVob* other);
    virtual const zSTRING* GetTriggerTarget(int index) const;

    zSTRING triggerTarget;

    static zCClassDef* classDef;
};

class zCTriggerUntouch : public zCTriggerBase {
public:

    zCTriggerUntouch() {}
    virtual ~zCTriggerUntouch();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);

    static zCClassDef* classDef;
};

class zCMessageFilter : public zCTriggerBase {
public:

    enum zTMessageType {
        MT_TRIGGER,
        MT_UNTRIGGER,
        MT_ENABLE,
        MT_DISABLE,
        MT_TOGGLE_ENABLED,
        MT_RESET
    };

    zCMessageFilter();
    virtual ~zCMessageFilter();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             ProcessMessage(zTMessageType message, zCVob* other);
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    unsigned char sendWhenTriggered : 4;
    unsigned char sendWhenUntriggered : 4;

    static zCClassDef* classDef;
};

class zCCodeMaster : public zCTriggerBase {
public:

    zCCodeMaster();
    virtual ~zCCodeMaster();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             Init();
    void             Reset();
    void             FireTriggerSuccess(zCVob* other);
    void             FireTriggerFailure(zCVob* other);
    int              GetSlaveNr(zCVob* other);
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnMessage(zCEventMessage* message, zCVob* sender);
    virtual const zSTRING* GetTriggerTarget(int index) const;
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);

    zSTRING slaveVobNameList[6];
    zSTRING triggerTargetFailure;
    unsigned char orderRelevant : 1;
    unsigned char untriggerCancels : 1;
    unsigned char firstFalseIsFailure : 1;
    zCVob*  slaveTriggeredList[6];
    int     numSlavesTriggered;

    static zCClassDef* classDef;
};

class zCMoverControler : public zCTriggerBase {
public:

    zCMoverControler()
        : moverMessage(0), gotoFixedKeyframe(0) {}
    virtual ~zCMoverControler();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntouch(zCVob* other);
    virtual void     OnTouch(zCVob* other);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    int moverMessage;
    int gotoFixedKeyframe;

    static zCClassDef* classDef;
};

class zCTriggerWorldStart : public zCTriggerBase {
public:

    zCTriggerWorldStart()
        : fireOnlyFirstTime(0), hasFired(0) {}
    virtual ~zCTriggerWorldStart();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual void     PostLoad();
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    char fireOnlyFirstTime;
    char hasFired;

    static zCClassDef* classDef;
};

class zCTrigger : public zCTriggerBase {
public:

    zCTrigger();
    virtual ~zCTrigger();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    zSTRING          GetTriggerTarget() const;
    void             ClearStateInternals();
    virtual int      CanBeActivatedNow(zCVob* other);
    virtual void     TriggerTarget(zCVob* other);
    virtual void     UntriggerTarget(zCVob* other);
    void             TriggerTargetPost();
    void             ActivateTrigger(zCVob* other);
    void             DeactivateTrigger(zCVob* other);
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);
    virtual void     OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
        const zVEC3& hitLocation);
    virtual void     OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void     OnTimer();
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    unsigned char reactToOnTrigger : 1;
    unsigned char reactToOnTouch : 1;
    unsigned char reactToOnDamage : 1;
    unsigned char respondToObject : 1;
    unsigned char respondToPC : 1;
    unsigned char respondToNPC : 1;
    unsigned char triggerFlags;
    zSTRING  respondToVobName;
    short    numCanBeActivated;
    float    retriggerWaitSec;
    float    damageThreshold;
    float    fireDelaySec;
    float    nextTimeTriggerable;
    zCVob*   savedOtherVob;
    short    countCanBeActivated;

    static zCClassDef* classDef;
};

class zCTriggerList : public zCTrigger {
public:

    enum zTListProcess {
        LP_ALL,
        LP_NEXT_ONE,
        LP_RAND_ONE
    };

    zCTriggerList();
    virtual ~zCTriggerList();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             Init();
    virtual const zSTRING* GetTriggerTarget(int index) const;
    void             TriggerActTarget(zCVob* other);
    int              ProcessActTarget(zCVob* other);
    void             FinishActivation();
    virtual void     OnTimer();
    void             DoTriggering(zCVob* other);
    virtual void     TriggerTarget(zCVob* other);
    virtual void     UntriggerTarget(zCVob* other);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zSTRING      triggerTargetList[6];
    float        fireDelayList[6];
    zTListProcess listProcess;
    int          actTarget;
    int          sendOnTrigger;

    static zCClassDef* classDef;
};

class zCCollObjectUndef {
public:

    static zCCollisionObjectClass s_oCollObjClass;
};

class zCCollObjectPoint {
public:

    static zCCollisionObjectClass s_oCollObjClass;
};

class zCCollObjectComplex {
public:

    static zCCollisionObjectClass s_oCollObjClass;
};

class zCCollObjectLevelPolys : public zCCollisionObject {
public:

    zCCollObjectLevelPolys()
        : m_pvClientData(0) {}

    zCArray<zCPolygon*> m_aoPolyList;

    static zCCollisionObjectClass s_oCollObjClass;
};

class zCMover : public zCTrigger {
public:

    enum zTMoverState {
        MOVER_STATE_OPEN,
        MOVER_STATE_OPENING,
        MOVER_STATE_CLOSED,
        MOVER_STATE_CLOSING
    };

    enum zTMoverAniType {
        MA_KEYFRAME,
        MA_MODEL_ANI,
        MA_WAYPOINT
    };

    enum zTMoverBehavior {
        MB_2STATE_TOGGLE,
        MB_2STATE_TRIGGER_CONTROL,
        MB_2STATE_OPEN_TIMED,
        MB_NSTATE_LOOP,
        MB_NSTATE_SINGLE_KEYS
    };

    enum zTTouchBehavior {
        TB_TOGGLE,
        TB_WAIT
    };

    enum zTPosLerpType {
        PL_LINEAR,
        PL_CURVE
    };

    enum zTSpeedType {
        ST_CONST,
        ST_SLOW_START_END,
        ST_SLOW_START,
        ST_SLOW_END,
        ST_SEG_SLOW_START_END,
        ST_SEG_SLOW_START,
        ST_SEG_SLOW_END
    };

    struct zTMov_Keyframe {
        zVEC3  pos;
        zCQuat quat;
    };

    zCMover();
    virtual ~zCMover();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             ClearStateInternals();
    void             UpdateInternals();
    static zTMov_Keyframe GetKeyframe(zCVob* vob);
    void             AdvanceKeyframe_KF();
    void             SetToKeyframe_KF(float keyframe);
    void             InterpolateKeyframes_KF();
    void             AdvanceMover();
    void             SetToKeyframe(float keyframe, float direction);
    void             InvertMovement();
    virtual void     SetVisual(zCVisual* visual);
    void             StartMovingSound();
    void             MoveToKeyframe(int keyframe);
    void             DoOpen();
    void             FinishedOpening();
    void             DoClose();
    void             FinishedClosing();
    void             Unlock(zCVob* other);
    void             Lock(zCVob* other);
    int              IsKeyToThisMover(zCVob* other);
    void             TriggerMover(zCVob* other);
    virtual void     OnTick();
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
        const zVEC3& hitLocation);
    virtual void     OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void     PostLoad();
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);
    virtual int      CanThisCollideWith(zCVob* other);

    zCArray<zTMov_Keyframe> keyframeList;
    zVEC3           actKeyPosDelta;
    float           actKeyframeF;
    int             actKeyframe;
    int             nextKeyframe;
    float           moveSpeedUnit;
    float           advanceDir;
    zTMoverState    moverState;
    int             numTriggerEvents;
    float           stayOpenTimeDest;
    zCModel*        model;
    int             soundMovingHandle;
    zCSoundFX*      sfxMoving;
    float           moveSpeed;
    float           stayOpenTimeSec;
    float           touchBlockerDamage;
    char            moverLocked;
    char            autoLinkEnabled;
    char            autoRotate;
    zTMoverAniType  moverAniType;
    zTMoverBehavior moverBehavior;
    zTTouchBehavior touchBehavior;
    zTPosLerpType   posLerpType;
    zTSpeedType     speedType;
    zSTRING         soundOpenStart;
    zSTRING         soundOpenEnd;
    zSTRING         soundMoving;
    zSTRING         soundCloseStart;
    zSTRING         soundCloseEnd;
    zSTRING         soundLock;
    zSTRING         soundUnlock;
    zSTRING         soundUseLocked;

    static zCClassDef* classDef;
};

class zCSoundFX;

class zCTriggerTeleport : public zCTrigger {
public:

    zCTriggerTeleport();
    virtual ~zCTriggerTeleport();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             DoTeleport(zCVob* other);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zSTRING    teleportSoundName;
    zCSoundFX* teleportSound;

    static zCClassDef* classDef;
};

class zCVobWaypoint : public zCVob {
public:

    zCVobWaypoint();
    virtual ~zCVobWaypoint();
    virtual zCClassDef* _GetClassDef() const;
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);

    static zCClassDef* classDef;
};

class zCVobStartpoint : public zCVob {
public:

    zCVobStartpoint() { type = 6; }
    virtual ~zCVobStartpoint();
    virtual zCClassDef* _GetClassDef() const;
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);

    static zCClassDef* classDef;
};

class zCEffect : public zCVob {
public:

    zCEffect() {}
    virtual ~zCEffect();
    virtual zCClassDef* _GetClassDef() const;

    static zCClassDef* classDef;
};

class zCEarthquake : public zCEffect {
public:

    zCEarthquake();
    virtual ~zCEarthquake();

    static zCObject* _CreateNewInstance();
    zCClassDef*      _GetClassDef() const;
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    float radiusSquare;
    float timeMSec;
    zVEC3 amplitude;

    static zCClassDef* classDef;
};

class zCPFXControler : public zCEffect {
public:

    zCPFXControler()
        : killVobWhenDone(1), pfxStartOn(1) {}
    virtual ~zCPFXControler();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    zCParticleFX*    GetPFX();
    virtual void     PostLoad();
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zSTRING pfxName;
    unsigned char killVobWhenDone : 1;
    unsigned char pfxStartOn : 1;

    static zCClassDef* classDef;
};

class zCTouchDamage : public zCEffect {
public:

    enum zTDamageCollType {
        CT_NONE,
        CT_BOX,
        CT_POINT
    };

    zCTouchDamage();
    virtual ~zCTouchDamage();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             ProcessToucher(zCVob* other);
    int              TestCollType(zCVob* other);
    void             FireDamageMessage(zCVob* other);
    void             SetVobProperties();
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);
    virtual void     OnTimer();
    virtual void     SetVisual(zCVisual* visual);
    virtual const char* GetDamageTypeArcEnum(unsigned long index);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    float           damage;
    int             damageType;
    float           damageRepeatDelaySec;
    float           damageVolDownScale;
    zTDamageCollType damageCollType;

    static zCClassDef* classDef;
};

class zCVisualAnimate : public zCVisual {
public:

    virtual ~zCVisualAnimate();
    zCClassDef* _GetClassDef() const;

    virtual void StartAni();
    virtual void StopAni();
    virtual int  IsAniActive();

    static zCClassDef* classDef;
};

class zCTouchAnimate : public zCEffect {
public:

    zCTouchAnimate();
    virtual ~zCTouchAnimate();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    int              IsTouching(zCVob* other);
    void             StartAni();
    void             StopAni();
    int              IsAniActive();
    virtual zSTRING  GetSoundName();
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);
    virtual void     OnTick();
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);
    virtual void     SetVisual(zCVisual* visual);
    void             SetVobProperties();

    zSTRING    touchSoundName;
    zCSoundFX* touchSound;

    static zCClassDef* classDef;
};

class zCTouchAnimateSound : public zCTouchAnimate {
public:

    zCTouchAnimateSound() {}
    virtual ~zCTouchAnimateSound();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual zSTRING  GetSoundName();
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zSTRING touchSoundName;

    static zCClassDef* classDef;
};

class zCVobAnimate : public zCEffect {
public:

    zCVobAnimate();
    virtual ~zCVobAnimate();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             StartAni();
    void             StopAni();
    virtual void     SetVisual(zCVisual* visual);
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    char startOn;
    char isRunning;

    static zCClassDef* classDef;
};

class zCVobLensFlare : public zCEffect {
public:

    zCVobLensFlare();
    virtual ~zCVobLensFlare();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual int      Render(zTRenderContext& context);
    void             SetLensFlareFXByName(const zSTRING& name);
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zCLensFlareFX* lensFlareFX;

    static zCClassDef* classDef;
};

class zCVobScreenFX : public zCEffect {
public:

    struct zTScreenFXSet {
        zTScreenFXSet() {}
        ~zTScreenFXSet();

        float weight;
        float velo;
        zCOLOR color;
        float fovDeg;
        int visible;
        zSTRING texName;
        float texAniFPS;
        zTRnd_AlphaBlendFunc blendFunc;
    };

    zCVobScreenFX();
    virtual ~zCVobScreenFX();

    static zCObject* _CreateNewInstance();
    zCClassDef*      _GetClassDef() const;
    void             StartEffect(float duration, zCOLOR color, int endState, zTScreenFXSet& effect);
    zCOLOR           CalcEffectColor(zTScreenFXSet& effect);
    virtual void     OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void     OnTick();
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);

    zTScreenFXSet blend;
    zTScreenFXSet cinema;
    zTScreenFXSet fovMorph;
    zVEC2 fovSaved;
    zVEC2 fovSavedFirst;

    static zCClassDef* classDef;
};

class zCVobStair : public zCVob {
public:

    zCVobStair() {}
    virtual ~zCVobStair();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;

    static zCClassDef* classDef;
};

class zCVobLevelCompo : public zCVob {
public:

    zCVobLevelCompo();
    virtual ~zCVobLevelCompo();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    int              HasIdentityTrafo() const;
    virtual void     Archive(zCArchiver& archiver);
    virtual void     Unarchive(zCArchiver& archiver);
    virtual void     SetVisual(zCVisual* visual);
    virtual void     SetVisual(const zSTRING& fileName);

    static zCClassDef* classDef;
    static int unarcIgnoreVisuals;
};

class zCEventMusicControler : public zCEventMessage {
public:

    virtual ~zCEventMusicControler();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    int MD_GetNumOfSubTypes();
    zSTRING MD_GetSubTypeString(int subType);
    int MD_GetTimeBehavior();
    float MD_GetMinTime();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zSTRING sgt;

    static zCClassDef* classDef;
};

class zCMusicControler : public zCVob {
public:

    zCMusicControler();
    virtual ~zCMusicControler();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void ThisVobRemovedFromWorld(zCWorld* world);

    static zCClassDef* classDef;
};

#endif
