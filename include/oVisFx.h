#ifndef OVISFX_H
#define OVISFX_H

#include "zKbSplines.h"
#include "zRenderer.h"
#include "oDialog.h"
#include "zPhysics.h"

struct zSVisualFXColl {
    zCVob* foundVob;
    zVEC3 foundContactPoint;
    zVEC3 foundNormal;
};

template<class T>
T zDYNAMIC_CAST(const zCObject* object)
{
    return dynamic_cast<T>(object);
}

class oCTrajectory {
public:

    void Calc();
    void Changed();
    zMAT4 Interpolate(float time);
    zMAT4 InterpolateN(float time);
    void Draw();
    void Clear();
    zVEC3 RecalcByColl(const zVEC3& start, const zVEC3& end, const zVEC3& direction, int flags);
    void ApplyGrav(const float& gravity);
    void SetByList(zCArray<zCVob*> vobs);
    int VobCross(const zCVob* vob);
    ~oCTrajectory();
    void InsertKey(zMAT4& matrix);
    zMAT4 GetKey(int index);
    int GetNumKeys();

    zCArray<zCPositionKey*> keyList;
    zCKBSpline* spl;
    int mode;
    float length;
    zMAT4 res;
    int lastKey;
};

class zCSoundFX;

class oCEmitterKey {
public:

    oCEmitterKey();
    ~oCEmitterKey();
    void SetByScript(const zSTRING& instanceName);
    void ParseStrings();
    void SetDefaultByFX(oCVisualFX* fx);
    void Edit();
    static void CreateNewScriptKey(const zSTRING& name);

    zSTRING visName_S;
    float visSizeScale;
    float scaleDuration;
    float pfx_ppsValue;
    int pfx_ppsIsSmoothChg;
    int pfx_ppsIsLoopingChg;
    float pfx_scTime;
    zSTRING pfx_flyGravity_S;
    zSTRING pfx_shpDim_S;
    int pfx_shpIsVolumeChg;
    float pfx_shpScaleFPS;
    float pfx_shpDistribWalkSpeed;
    zSTRING pfx_shpOffsetVec_S;
    zSTRING pfx_shpDistribType_S;
    zSTRING pfx_dirMode_S;
    zSTRING pfx_dirFOR_S;
    zSTRING pfx_dirModeTargetFOR_S;
    zSTRING pfx_dirModeTargetPos_S;
    float pfx_velAvg;
    float pfx_lspPartAvg;
    float pfx_visAlphaStart;
    zSTRING lightPresetName;
    float lightRange;
    zSTRING sfxID;
    int sfxIsAmbient;
    zSTRING emCreateFXID;
    float emFlyGravity;
    zSTRING emSelfRotVel_S;
    zSTRING emTrjMode_S;
    float emTrjEaseVel;
    int emCheckCollision;
    float emFXLifeSpan;
    unsigned char dScriptEnd;
    zVEC3 pfx_flyGravity;
    zVEC3 pfx_shpOffsetVec;
    int pfx_shpDistribType;
    int pfx_dirMode;
    int pfx_dirFOR;
    int pfx_dirModeTargetFOR;
    zVEC3 pfx_dirModeTargetPos;
    zVEC3 emSelfRotVel;
    int emTrjMode;
    zSTRING name;
    zCVob* vob;
    zVEC3 targetPos;
    zCSoundFX* sfx;
    int sfxHnd;
};

class oCVisualFXAI : public zCAIBase {
public:

    oCVisualFXAI() : vob(NULL), delete_it(0) {}
    virtual ~oCVisualFXAI();
    virtual void DoAI(zCVob* vob, int& deleteAI);
    virtual void ReportCollisionToAI(const zCCollisionReport& report);
    virtual void HostVobAddedToWorld(zCVob* vob, zCWorld* world);
    virtual int GetIsProjectile();

    zCVob* vob;
    int delete_it;
};

#endif
