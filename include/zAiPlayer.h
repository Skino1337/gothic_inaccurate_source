#ifndef ZAIPLAYER_H
#define ZAIPLAYER_H

#include "zAi.h"

class zCQuadMark;

struct zCDetectLedgeReport {
    zCDetectLedgeReport(const zVEC3& origin);

    zVEC3 origin;
    int   found;
    zVEC3 normal;
    zVEC3 intersection;
    zVEC3 continuation;
};

class zCAIPlayer : public zCAIBase {
public:

    enum zTMovementState {
        zMV_STATE_STAND,
        zMV_STATE_FLY,
        zMV_STATE_SWIM,
        zMV_STATE_DIVE
    };

    struct zTConfig {
        float zMV_MIN_SPACE_MOVE_FORWARD;
        float zMV_DETECT_CHASM_SCAN_AT;
        float zMV_STEP_HEIGHT;
        float zMV_JUMP_UP_MIN_CEIL;
        float zMV_WATER_DEPTH_KNEE;
        float zMV_WATER_DEPTH_CHEST;
        float zMV_YMAX_SWIM_CLIMB_OUT;
        float zMV_FORCE_JUMP_UP;
        float zMV_YMAX_JUMPLOW;
        float zMV_YMAX_JUMPMID;
        float zMV_MAX_GROUND_ANGLE_WALK;
        float zMV_MAX_GROUND_ANGLE_SLIDE;
        float zMV_MAX_GROUND_ANGLE_SLIDE2;
        float zMV_DCUL_WALL_HEADING_ANGLE;
        float zMV_DCUL_WALL_HORIZ_ANGLE;
        float zMV_DCUL_GROUND_ANGLE;
    };

    struct zTLedgeInfo {
        zVEC3 point;
        zVEC3 normal;
        zVEC3 cont;
        float maxMoveForward;
    };

    struct zTBloodVobTrack {
        zCVob* bloodVob;
        float  alpha;
    };

    zCAIPlayer();
    virtual ~zCAIPlayer();
    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void HostVobRemovedFromWorld(zCVob* vob, zCWorld* world);
    void RemoveEffects();

    zCModel* GetModel() const;
    void     SetVob(zCVob* target);
    void     SetJumpUpForceByHeight(float height);
    void     SetBloodDefaultTexture(const zSTRING& texture);
    void     SetPhysicsEnabled(int enabled);
    void     ResetAutoRoll();
    void     DoAutoRoll();
    void     CalcModelAnisInPlace();
    void     DoSurfaceAlignment();
    int      IsSliding();
    int      CheckFloorSliding();
    void     CheckPhysics();
    void     PropagateCollObjectStates(zTMovementState newState);
    void     CalcForceModelHalt();
    void     CalcStateVars();
    void     DoProceduralMovement();
    virtual void StartStandAni();
    virtual void StartFallDownAni();
    void     AddWeaponTrailSegment(const zVEC3& start, const zVEC3& end);
    void     End();
    virtual void DoAI(zCVob* vob, int& deleteAI);
    void     UpdateEffects();
    void     Print(const zSTRING& text) const;
    void     Line3D(const zVEC3& start, const zVEC3& end, int mode) const;
    void     PrintScreen(int x, int y, zSTRING text) const;
    zCQuadMark* GetBloodQuadMark();
    void     AddBlood(const zVEC3& origin, const zVEC3& direction, float size,
        int animated, zSTRING* texture);
    int      CheckEnoughSpaceMoveForward(int dontMove);
    int      CheckEnoughSpaceMoveBackward(int dontMove);
    int      CheckForceModelHalt(zCModelAni* ani);
    int      DetectClimbUpLedge(zVEC3& point, int walkable);
    void     CreateLedgeInfo();
    int      GetFoundLedge() const;
    void     ClearFoundLedge();
    void     AlignModelToLedge(const zSTRING& aniName);
    void     AlignToFloor(float height);
    float    GetModelFloorWorld();
    int      CheckEnoughSpaceMoveDir(const zVEC3& direction, int dontMove);
    int      CheckEnoughSpaceMoveRight(int dontMove);
    int      CheckEnoughSpaceMoveLeft(int dontMove);
    void     Begin(zCVob* target);
    zVEC3    GetHandPositionWorld() const;
    float    GetJumpUpHeight();
    float    GetProtoYHeight();
    zVEC3    GetSurfaceAlignScanOrigin();
    void     DiveRotateX(const float& angle);
    void     StartPhysicsWithVel();
    void     LandAndStartAni(const zSTRING& aniName);
    void     LandAndStartAni(zCModelAni* ani);
    int      CheckForceModelHalt(const zSTRING& aniName);
    int      ShouldCorrectFloorHeight();
    void     BeginStateSwitch();
    void     SetCDStat(int enabled);
    void     Archive(zCArchiver& archiver);
    void     Unarchive(zCArchiver& archiver);
    zTLedgeInfo* GetLedgeInfo() const;

    zTConfig config;
    zCVob*   vob;
    zCModel* model;
    zCWorld* world;
    zVEC3    centerPos;
    float    feetY;
    float    headY;
    float    aboveFloor;
    int      waterLevel;
    float    velocityLen2;
    zVEC3    velocity;
    float    fallDownDistanceY;
    float    fallDownStartY;
    zVEC3    slidePolyNormal;
    float    checkWaterCollBodyLen;
    zCModelNodeInst* modelHeadNode;
    int      zMV_DO_SURFACE_ALIGN : 2;
    int      forceModelHalt : 1;
    int      autoRollEnabled : 1;
    int      modelAnisInPlace : 1;
    zTMovementState oldState : 3;
    zTMovementState state : 3;
    int      oldWaterLevel : 2;
    int      m_eCollObjectState : 4;
    int      m_eCollObjectStateOld : 4;
    zCArray<zTBloodVobTrack> bloodVobList;
    float    bleedingPerc;
    zVEC3    bleedingLastPos;
    float    bleedingNextDist;
    zCVob*   weaponTrailVob;
    zCVob*   waterRingVob;
    float    waterRingTimer;
    float    autoRollPos;
    float    autoRollPosDest;
    float    autoRollSpeed;
    float    autoRollMaxAngle;
    unsigned char dummyLastVar;
    zSTRING  bloodDefaultTexName;

    static zCClassDef* classDef;
    static int         s_bShowWeaponTrails;
};

#endif
