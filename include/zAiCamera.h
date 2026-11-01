#ifndef ZAICAMERA_H
#define ZAICAMERA_H

#include "oDialog.h"

class zCAICamera;
struct zSPathSearchOptions {
    int searchFlags;
    int poseFailReason;
    int minLerpMode;
    float minScore;
    float bestScore;
    float startAzi;
    float startElev;
    float startRange;
    float endAzi;
    float endElev;
    float endRange;
    float quality;
};

struct zSPathSearchState_Internal;
class zCSphereCoordsCache;

class zCPathSearch {
public:
    zCPathSearch();
    ~zCPathSearch();
    static zSPathSearchResult* GetReport();
    static zCPathSearch* GetSearch();
    int SphereSearch(const zSEvasionSearchDesc& desc);
    int SwitchSearch(const zSEvasionSearchDesc& desc);
    int ShoulderSearch(const zSEvasionSearchDesc& desc);
    int PanicSearch(const zSEvasionSearchDesc& desc);
    void CalcStateScores(zSPathSearchState& state);
    int GetBestLerpMode(zSPathSearchState& state);
    int GetBestLerpMode(const zVEC3& start, const zVEC3& end);
    zVEC3 DoSlerpRotQuat(const zCQuat& first, const zCQuat& second, const float& time,
        const float& range);
    void CamVobChanged();
    void TargetChanged();
    void SetAICamera(zCAICamera* camera);
    int AdjustCenterSphere(zVEC3& center, const zVEC3& player, float radius);
    int FindBestEvasion(zSPathSearchOptions& options);
    int CanEvadeTo(zSPathSearchState& state);
    int FindEvasionSequence(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_PlayerInvisible_Moved(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_PlayerInvisible_Moved_BW(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_PlayerInvisible_Moved_FW(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_IdealCone(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_PlayerInvisible_Standing(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_PlayerInvisible_Rotated(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_PlayerTargetInvisible(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_CamHitWall(zCArray<zSEvasionSearchDesc>& sequence);
    int FES_NoEvasionFound(zCArray<zSEvasionSearchDesc>& sequence);
    int EvasionPossible();
    int IsPointValid(const zVEC3& point, int flags, int& reason);
    int IsPointNearWall(const zVEC3& point);
    int LineOfSight(const zVEC3& start, const zVEC3& end);
    int CorrectPosForNearClip(zVEC3& point);
    zVEC3& GetPolyNearestPoint(zCPolygon* polygon, const zVEC3& point) const;
    int collisionEnabled;
    zCAICamera* camai;
    zCVob* camVob;
    zCVob* target;
    zCWorld* world;
    int ignoreSearchFlags;
    zSPathSearchOptions searchOptions;
    zCMovementTracker* moveTracker;
    zSPathSearchState_Internal bestSearchState;
    zSPathSearchState_Internal lastSearchState;
    zSPathSearchResult evasionSearchReport;
    zCSphereCoordsCache searchCache;
};
class zCMovementTracker {
public:
    zCMovementTracker();
    ~zCMovementTracker();
    static zCMovementTracker* GetTracker();
    void CamVobChanged();
    void TargetChanged();
    void ReceiveMsg(int message);
    void InterpolateTo(const zSPathSearchResult* result);
    void SetRotateEnabled(int enabled);
    float GetAzimuth();
    float GetElevation();
    float GetAzimuth(const zVEC3& point);
    float GetElevation(const zVEC3& point);
    float GetRange(const zVEC3& point);
    float GetRange();
    void SetAzimuth(const float& value);
    void SetElevation(const float& value);
    void SetRange(const float& value);
    zVEC3 ApplyAziToPoint(const float& value, const zVEC3& point);
    zVEC3 ApplyElevToPoint(const float& value, const zVEC3& point);
    zVEC3 ApplyRangeToPoint(const float& value, const zVEC3& point);
    zVEC3 GetRangePos(const float& value);
    zVEC3 GetIdealRangePos();
    float GetDistToIdealRange();
    int IsIdealOrientation();
    int IsIdealRange();
    void UpdateCamPos(const zVEC3& point);
    void UpdatePlayerPos(const zVEC3& point);
    void SetCamPos(const zVEC3& point);
    void CalcPlayerPose();
    void CalcPlayerMovements();
    void SetPlayerPos(const zVEC3& point);
    void ClearMessages();
    void AutoSetMessages();
    void UpdateMessages();
    void Update();
    void CheckKeys();
    int CheckSpecialMoves();
    zCQuat GetLocalRot(const float& azi, const float& elev);
    zMAT4 GetLocalRotEnd();
    void RecalcRingPositions();
    enum zTWayPoint {
        zWAYPOINT_PLAYER,
        zWAYPOINT_CAM,
        zWAYPOINT_HEAD,
        zWAYPOINT_HIP_LEFT,
        zWAYPOINT_HIP_RIGHT
    };
    zVEC3& GetLastValidWayPoint(const zTWayPoint& wayPoint);
    void UpdateWayPoint(const zTWayPoint& wayPoint);
    void ResetWayPoints(const zVEC3& point, const zTWayPoint& wayPoint);
    void CalcPlayerSpline();
    zMAT4& GetFirstPersonCamMat();
    zMAT4& GetPanicCamMat();
    zMAT4& GetShoulderCamMat();
    zVEC3& CalcAziElevRangeApprox(const float& azi, const float& elev, const float& range);
    void SaveFrameStates();
    void UpdateSampledPlayerPos(const zVEC3& point);
    void SetSampledPlayerPos(const zVEC3& point);
    zVEC3& GetPlayerFuturePos(float seconds);

    unsigned long movement;
    zCArray<zCPositionKey*> targetKFList;
    zVEC3 playerPos;
    zVEC3 oldPlayerPos;
    zVEC3 camPos;
    zVEC3 playerHead;
    zVEC3 playerHipLeft;
    zVEC3 playerHipRight;
    zVEC3 playerPosRing;
    zVEC3 POI_Player;
    zVEC3 oldCamPos;
    zVEC3 oldPlayerPosMovement;
    zVEC3 camPosRing;
    zVEC3 sampledPlayerPos;
    zVEC3 sampledPlayerPosRing;
    zVEC3 oldSampledPlayerPosRing;
    zVEC3 oldSampledPlayerPos;
    zVEC3 playerPosMovementRing;
    zVEC3 playerPosMovementLocal;
    zVEC3 playerPosMovementLocalCam;
    zVEC3 playerPosMovement;
    zVEC3 playerFuturePos;
    float playerFuturePosDT;
    float inertiaTrans;
    float inertiaHead;
    float inertiaRotTargetFOR;
    float inertiaTargetTrans;
    float inertiaTargetRot;
    float oldDistToPlayer;
    float actAzi;
    float actElev;
    float distToPlayer;
    float playerVeloRange;
    float oldAzi;
    float oldElev;
    int standing;
    int rotate;
    int numPreSamplesPlayerPos;
    int numSamplesCam;
    int numSamplesPlayer;
    float timeSamples;
    float frameTime;
    float standTimer;
    int bMouseUsed;
    zMAT4 trafoTStoWS;
    zMAT4 trafoWStoTS;
    zMAT4 trafoWStoCS;
    zMAT4 trafoCStoWS;
    zMAT4 oldTrafoWStoTS;
    zMAT4 oldTrafoWStoCS;
    zMAT4 oldTrafoRotOffsetWStoCS;
    zMAT4 lastValidCamTrafo;
    zMAT4 initialPoseRotFreeze;
    zCQuat actRot;
    zVEC3 wayPointHead[3];
    zVEC3 wayPointHipLeft[3];
    zVEC3 wayPointHipRight[3];
    zVEC3 wayPointCam[3];
    int actWayPointHead;
    int actWayPointHipLeft;
    int actWayPointHipRight;
    int actWayPointCam;
    float wayTimeHead;
    float wayTimeHipLeft;
    float wayTimeHipRight;
    float wayTimeCam;
    zCVob* target;
    zCVob* camVob;
    zCAICamera* camai;
    zCPathSearch* pathSearch;
};
struct zSPathSearchResult {
    int foundEvasion;
    zVEC3 p1;
    zVEC3 p2;
    zCQuat r1;
    zCQuat r2;
    int bestLerpMode;
    float veloTrans;
    float veloRot;
    zCArray<zCPositionKey*> resPath;
};

struct zSEvasionSearchDesc {
    struct zSEvasionSearchOptions {
        zSEvasionSearchOptions& operator=(const zSEvasionSearchOptions& other)
        {
            scSign = other.scSign;
            scQuality = other.scQuality;
            scStart = other.scStart;
            scEnd = other.scEnd;
            scSequence = other.scSequence;
            scName = other.scName;
            return *this;
        }

        zVEC3 scSign;
        zVEC3 scQuality;
        zVEC3 scStart;
        zVEC3 scEnd;
        zSTRING scSequence;
        zSTRING scName;
    };

    zSEvasionSearchDesc& operator=(const zSEvasionSearchDesc& other)
    {
        searchFunc = other.searchFunc;
        searchName = other.searchName;
        sphereSearchList = other.sphereSearchList;
        return *this;
    }

    int(* searchFunc)(const zSEvasionSearchDesc&);
    zSTRING searchName;
    zCArray<zSEvasionSearchOptions> sphereSearchList;
};
class zCCSCamera;

class zCAICamera : public zCAIBase {
public:

    zCAICamera();
    virtual ~zCAICamera();
    virtual zCClassDef* _GetClassDef() const;
    static zCAICamera* GetCurrent();
    static zCAICamera* Create();
    static void GlobalStartUp();
    static void GlobalCleanUp();
    static void GlobalDialogCamCleanUp();
    static void operator delete(void* memory);
    void StartUp();
    void CleanUp();
    float GetBestRange();
    float GetMinRange();
    float GetMaxRange();
    void SetMode(zSTRING& mode, zCArray<zCVob*> targets);
    zSTRING GetMode();
    int IsModeActive(zSTRING& mode);
    int IsModeAvailable(const zSTRING& mode);
    void ReceiveMsg(int message);
    void SetVob(zCVob* vob);
    void SetTarget(zCVob* vob);
    void SetTargetList(zCArray<zCVob*> targets);
    void ClearTargetList();
    void AddTarget(zCVob* vob);
    void SubTarget(zCVob* vob);
    int Console_EvalFunc(const zSTRING& input, zSTRING& message);
    void ParameterChanged();
    void CalcFirstPosition();
    int SetByScript(zSTRING& mode);
    zSTRING SetByScriptLine(zSTRING& line);
    void CreateInstance(zSTRING& mode);
    void AI_LookingCam();
    void AI_FirstPerson();
    void AI_Normal();
    zVEC3& GetCamSysFirstPos();
    void SetHintTargetInPortalRoom(int enabled);
    void CheckKeys();
    void SetRotateEnabled(int enabled);
    static zCObject* _CreateNewInstance();
    void StartDialogCam(int take);
    void StartupDialogCam();
    void InitDialogCam();
    void SetDialogCamDuration(float duration);
    void ReleaseLastDialogCam();
    zCCSCamera* PickPreset(zSTRING presetName, const zCArray<zCCSCamera*>& presets);
    int DialogPresetIsValid(zCCSCamera* preset);
    void InitHelperVobs();
    void DeleteHelperVobs();
    void ShowDebug();
    void DrawClipLine(const zVEC3& start, const zVEC3& end, zCOLOR color);
    zTBSphere3D CalcMinimumBSphere(zCArray<zVEC3>& points);
    int CheckUnderWaterFX();
    zVEC3& CalcAziElevRange(const float& azi, const float& elev, const float& range,
        const zMAT4& trafo);
    virtual void DoAI(zCVob* vob, int& deleteAI);

    int d_showDots;
    int pathDetectCollision;
    float bestRange;
    float minRange;
    float maxRange;
    float bestRotX;
    float minRotX;
    float maxRotX;
    float bestRotY;
    float minRotY;
    float maxRotY;
    float bestRotZ;
    float minRotZ;
    float maxRotZ;
    float rotOffsetX;
    float rotOffsetY;
    float rotOffsetZ;
    float focusOffsetX;
    float focusOffsetY;
    float focusOffsetZ;
    float veloTrans;
    float veloRot;
    int translate;
    int rotate;
    int collision;
    zCArray<zCVob*> targetVobList;
    zCVob* camVob;
    zCVob* target;
    float targetAlpha;
    int numTargets;
    zSTRING oldCamSys;
    int sysChanged;
    int playerIsMovable;
    int followIdealPos;
    float dialogCamDuration;
    int numOUsSpoken;
    int numDialogCamTakes;
    int lastNumDialogCamTakes;
    int lastDialogCamSide;
    int firstSpeakerWasPC;
    zCCSCamera* dialogCam;
    zSTRING lastPresetName;
    int raysCasted;
    int underWater;
    int inCutsceneMode;
    zSTRING debugS;
    int showPath;
    zCVob* focusVob;
    zCVob* ctrlDot[80];
    zCPathSearch* pathSearch;
    zCMovementTracker* moveTracker;
    zSPathSearchResult* evasionSearchReport;
    zSTRING curcammode;
    float npcVolumeRangeOffset;
    float camDistOffset;
    zVEC3 camSysFirstPos;
    int firstPerson;
    int targetInPortalRoom;

    static zCClassDef* classDef;
    static zCAICamera* current;
    static zCArray<zCCSCamera*> dialogCamList;
    static float lookingCamLastAzi;
    static int lookingCamAziGoalReached;
    static int bCreated;
    static int bCamChanges;
    static short s_iLookAroundSgn;
};

#endif
