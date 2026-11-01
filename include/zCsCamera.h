#ifndef ZCSCAMERA_H
#define ZCSCAMERA_H

#include "zVob.h"
#include "zKbSplines.h"

class zCCSCamera;
class zCCSCamera_EventMsg;
class zCCSCamera_EventMsgActivate;

class zCCamTrj_KeyFrame : public zCVob {
public:
    enum zTCamTrj_KFType {
        KF_UNDEF,
        KF_TARGET,
        KF_CAM
    };

    zCCamTrj_KeyFrame();
    virtual ~zCCamTrj_KeyFrame();
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void ThisVobRemovedFromWorld(zCWorld* world);
    void Changed();

    float time;
    float angleRollDeg;
    float camFOVScale;
    int motionType;
    int motionTypeFOV;
    int motionTypeRoll;
    int motionTypeTimeScale;
    int timeIsFixed;
    float tension;
    float bias;
    float continuity;
    float timeScale;
    zMAT4 originalPose;
    zMAT4 actPose;
    int adjusted;
    zCCSCamera* cscam;
    zTCamTrj_KFType type;

    static zCClassDef* classDef;
};

class zCCSCamera : public zCVob {
public:
    zCCSCamera();
    virtual ~zCCSCamera();
    static zCVob* GetPlayerVob();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    zCCamTrj_KeyFrame* GetCamKey(int index);
    zCCamTrj_KeyFrame* GetTargetKey(int index);
    void InsertCamKey(zCCamTrj_KeyFrame* key);
    void InsertCamKeyAtPos(zCCamTrj_KeyFrame* key, int index);
    void RemoveCamKey(zCCamTrj_KeyFrame* key);
    void InsertTargetKey(zCCamTrj_KeyFrame* key);
    void InsertTargetKeyAtPos(zCCamTrj_KeyFrame* key, int index);
    void RemoveTargetKey(zCCamTrj_KeyFrame* key);
    void PreKFProcessing();
    void PostKFProcessing();
    void AutoSetKeyTimes(zCArray<zCCamTrj_KeyFrame*>& keys);
    void Refresh();
    void Draw();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void ThisVobRemovedFromWorld(zCWorld* world);
    virtual void OnMessage(zCEventMessage* message, zCVob* source);
    void Initialize();
    void Changed();
    int EV_Play(zCCSCamera_EventMsg* event);
    int EV_Resume(zCCSCamera_EventMsg* event);
    int EV_Pause(zCCSCamera_EventMsg* event);
    int EV_Stop(zCCSCamera_EventMsg* event);
    int EV_GotoKey(zCCSCamera_EventMsg* event);
    int EV_SetDuration(zCCSCamera_EventMsg* event);
    int EV_SetTime(zCCSCamera_EventMsg* event);
    int EV_Deactivate(zCCSCamera_EventMsgActivate* event);
    int EV_SetCamReference(zCCSCamera_EventMsgActivate* event);
    int EV_SetTargetReference(zCCSCamera_EventMsgActivate* event);
    void DeleteEvMsgOfSubType(int eventSubType);
    virtual void OnTick();
    virtual void OnTrigger(zCVob* other, zCVob* source);
    virtual void OnUntrigger(zCVob* other, zCVob* source);
    void RecalcTrajectories();
    int CheckAndResetAdjustedKeys();
    int CheckAndCorrectKey(int index);
    int CheckAndCorrectPose(zMAT4& pose);
    void Reset();
    void CalcPrevNextActKeyOfTime(const float& time, zCArray<zCCamTrj_KeyFrame*>& keys,
        int& previous, int& next, int& current);
    int IsPointNearWall(const zVEC3& point);
    zVEC3 GetPolyNearestPoint(zCPolygon* polygon, const zVEC3& point) const;
    int AdjustCenterSphere(zVEC3& center, const zVEC3& player, float radius);
    static void SetDrawEnabled(int enabled);

    static int playing;
    static int draw;
    static int evaluateAvgFPS;

    int camKeysFOR;
    int targetKeysFOR;
    int loopMode;
    int splLerpMode;
    int ignoreFORVobRotCam;
    int ignoreFORVobRotTarget;
    int adaptToSurroundings;
    int easeToFirstKey;
    int easeFromLastKey;
    float totalTime;
    zSTRING autoCamFocusVobName;
    int autoCamPlayerMovable;
    int autoCamUntriggerOnLastKey;
    float autoCamUntriggerOnLastKeyDelay;
    zCVob* camKeysRefVob;
    zCVob* targetKeysRefVob;
    zCArray<zCEventMessage*> evMsgProcessList;
    zCArray<zCCamTrj_KeyFrame*> posKeys;
    zCArray<zCCamTrj_KeyFrame*> targetKeys;
    int paused;
    int hasBeenTriggered;
    int hasBeenUntriggered;
    int triggerQueued;
    zCVob* queuedVob;
    int keysPreProcessed;
    int started;
    int gotoTimeMode;
    int initialized;
    int easeToFirstKeyNotPossible;
    int easeFromLastKeyNotPossible;
    int autoCamPlayerMovable_BAK;
    int autoAdjustTime;
    float avgFPS;
    int avgFPSNumFrames;
    float actTimeCam;
    float csTime;
    float unTriggerDelayTime;
    zMAT4 trafoCStoWS;
    float lerpDirSign;
    float timeToReach;
    float timeToReachScale;
    int camFOVSetBy;
    int angleRollDegSetBy;
    int globalTimeScaleSetBy;
    int d_testCS;
    zCKBSpline posSpl;
    zCKBSpline targetSpl;
    zMAT4 initialPoseCamRefVob;
    zMAT4 initialPoseTargetRefVob;
    zVEC3 actTargetFocus;

    static zCClassDef* classDef;
};

class zCCSCamera_EventMsg : public zCEventMessage {
public:

    enum zTCSCam_EvSubType {
        EV_NOTHING,
        EV_PLAY,
        EV_PAUSE,
        EV_RESUME,
        EV_STOP,
        EV_GOTO_KEY,
        EV_SET_DURATION,
        EV_SET_TO_TIME
    };

    zCCSCamera_EventMsg();
    zCCSCamera_EventMsg(zTCSCam_EvSubType eventSubType);
    virtual ~zCCSCamera_EventMsg();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int IsHighPriority();
    virtual int IsJob();
    virtual void Delete();
    virtual int IsDeleted();
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int eventSubType);
    virtual zTTimeBehavior MD_GetTimeBehavior();
    virtual float MD_GetMinTime();

    int key;
    float time;
    int isDeleted;
    int kfType;

    static zCClassDef* classDef;
};

class zCCSCamera_EventMsgActivate : public zCEventMessage {
public:

    enum zTCSCam_ActivateSubType {
        EV_DEACTIVATE,
        EV_SETCAMREFERENCE,
        EV_SETTARGETREFERENCE
    };

    zCCSCamera_EventMsgActivate();
    zCCSCamera_EventMsgActivate(zTCSCam_ActivateSubType eventSubType);
    virtual ~zCCSCamera_EventMsgActivate();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void Delete();
    virtual int IsDeleted();
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int eventSubType);
    virtual zSTRING MD_GetVobRefName();
    virtual void MD_SetVobRefName(const zSTRING& name);
    virtual void MD_SetVobParam(zCVob* vob);

    zSTRING referenceName;
    int isDeleted;
    zCVob* referenceVob;

    static zCClassDef* classDef;
};

#endif
