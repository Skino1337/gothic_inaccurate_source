#ifndef ZPARTICLE_H
#define ZPARTICLE_H

#include "zModel.h"

class zCParticleFX;
class zCCamera;

class zCParser {
public:

    void Reset();
    void Parse(const zSTRING& fileName);
    void CreatePCode();
    int     GetNumSymbols() const;
    zSTRING GetSymbolName(int index) const;
    int     GetSymbolType(int index) const;
    zSTRING GetSymbolStringValue(int index, int element) const;
};

struct zSParticle {
    zSParticle* next;
    zVEC3       position;
    zVEC3       positionWS;
    zVEC3       vel;
    float       lifeSpan;
    float       alpha;
    float       alphaVel;
    zVEC2       size;
    zVEC2       sizeVel;
    zVEC3       color;
    zVEC3       colorVel;
    zCVisual*   polyStrip;
};

enum zTPFX_EmitterShape {
    zPFX_EMITTER_SHAPE_POINT,
    zPFX_EMITTER_SHAPE_LINE,
    zPFX_EMITTER_SHAPE_BOX,
    zPFX_EMITTER_SHAPE_CIRCLE,
    zPFX_EMITTER_SHAPE_SPHERE,
    zPFX_EMITTER_SHAPE_MESH
};

enum zTPFX_EmitterDirMode {
    zPFX_EMITTER_DIRMODE_NONE,
    zPFX_EMITTER_DIRMODE_DIR,
    zPFX_EMITTER_DIRMODE_TARGET,
    zPFX_EMITTER_DIRMODE_MESH
};

class zCParticleEmitter {
public:

    zCParticleEmitter();
    zCParticleEmitter(const zCParticleEmitter& other);
    zCParticleEmitter& operator=(const zCParticleEmitter& other);
    ~zCParticleEmitter();

    void UpdateVelocity();
    void AddCompoundReferences();
    void ResetStrings();
    void Reset();
    void UpdateInternals();
    void SetOutputDir(const zVEC3& direction);
    void ConvertAnglesIntoBox();
    zVEC3 GetPosition();
    zVEC3 GetVelocity(zSParticle* particle, zCParticleFX* pfx);

    static zVEC3 String2Vec3(const zSTRING& text);
    static zVEC2 String2Vec2(const zSTRING& text);

    float   ppsValue;
    zSTRING ppsScaleKeys_S;
    int     ppsIsLooping;
    int     ppsIsSmooth;
    float   ppsFPS;
    zSTRING ppsCreateEm_S;
    float   ppsCreateEmDelay;
    zSTRING shpType_S;
    zSTRING shpFOR_S;
    zSTRING shpOffsetVec_S;
    zSTRING shpDistribType_S;
    float   shpDistribWalkSpeed;
    int     shpIsVolume;
    zSTRING shpDim_S;
    zSTRING shpMesh_S;
    int     shpMeshRender_B;
    zSTRING shpScaleKeys_S;
    int     shpScaleIsLooping;
    int     shpScaleIsSmooth;
    float   shpScaleFPS;
    zSTRING dirMode_S;
    zSTRING dirFOR_S;
    zSTRING dirModeTargetFOR_S;
    zSTRING dirModeTargetPos_S;
    float   dirAngleHead;
    float   dirAngleHeadVar;
    float   dirAngleElev;
    float   dirAngleElevVar;
    float   velAvg;
    float   velVar;
    float   lspPartAvg;
    float   lspPartVar;
    zSTRING flyGravity_S;
    int     flyCollDet_B;
    zSTRING visName_S;
    zSTRING visOrientation_S;
    int     visTexIsQuadPoly;
    float   visTexAniFPS;
    int     visTexAniIsLooping;
    zSTRING visTexColorStart_S;
    zSTRING visTexColorEnd_S;
    zSTRING visSizeStart_S;
    float   visSizeEndScale;
    zSTRING visAlphaFunc_S;
    float   visAlphaStart;
    float   visAlphaEnd;
    float   trlFadeSpeed;
    zSTRING trlTexture_S;
    float   trlWidth;
    float   mrkFadeSpeed;
    zSTRING mrkTexture_S;
    float   mrkSize;
    zSTRING m_flockMode_S;
    float   m_fFlockWeight;
    int     m_bSlowLocalFOR;
    zSTRING m_timeStartEnd_S;
    int     m_bIsAmbientPFX;
    int     endOfDScriptPart;
    zSTRING particleFXName;
    zCArray<float> ppsScaleKeys;
    zCParticleEmitter* ppsCreateEmitter;
    zTPFX_EmitterShape shpType;
    float   shpCircleSphereRadius;
    zVEC3   shpLineBoxDim;
    zVEC3*  shpMeshLastPolyNormal;
    zCMesh* shpMesh;
    zCProgMeshProto* shpProgMesh;
    zCModel* shpModel;
    int     shpFOR;
    int     shpDistribType;
    zVEC3   shpOffsetVec;
    zCArray<float> shpScaleKeys;
    zTPFX_EmitterDirMode dirMode;
    int     dirFOR;
    int     dirModeTargetFOR;
    zVEC3   dirModeTargetPos;
    zTBBox3D dirAngleBox;
    zVEC3   dirAngleBoxDim;
    zVEC3   flyGravity;
    zCTexture* visTexture;
    int     visOrientation;
    zVEC2   visSizeStart;
    zVEC3   visTexColorRGBAStart;
    zVEC3   visTexColorRGBAEnd;
    int     visAlphaFunc;
    zCTexture* trlTexture;
    zCTexture* mrkTexture;
    int     isOneShotFX;
    float   dirAngleHeadVarRad;
    float   dirAngleElevVarRad;
    int     m_flockMode;
    float   m_ooAlphaDist;
    float   m_startTime;
    float   m_endTime;
};

struct zCParticleEmitterVars {
    float ppsScaleKeysActFrame;
    float ppsNumParticlesFraction;
    float ppsTotalLifeTime;
    int   ppsDependentEmitterCreated;
    float shpScaleKeysActFrame;
    float uniformValue;
    float uniformDelta;
};

class zCParticleFX : public zCVisual {
public:

    class zCStaticPfxList {
    public:

        void InsertPfxHead(zCParticleFX* pfx);
        void RemovePfx(zCParticleFX* pfx);
        void TouchPfx(zCParticleFX* pfx);
        void ProcessList();
        int  IsInList(zCParticleFX* pfx);

        zCParticleFX* pfxListHead;
        zCParticleFX* pfxListTail;
        int           numInList;
    };

    zCParticleFX();
    virtual ~zCParticleFX();

    void InitEmitterVars();
    void UpdateParticleFX();
    void CreateParticlesUpdateDependencies();
    void UpdateParticle(zSParticle* particle);
    float GetShapeScaleThisFrame();
    int  GetNumParticlesThisFrame();
    void CheckDependentEmitter();
    void CreateParticles();
    void FreeParticles();
    void RemoveEmitter();
    int  SetEmitter(zCParticleEmitter* emitter, int copy);
    int  SetEmitter(const zSTRING& name, int copy);
    int  SetAndStartEmitter(zCParticleEmitter* emitter, int copy);
    int  SetAndStartEmitter(const zSTRING& name, int copy);
    void StopEmitterOutput();
    void RestoreEmitterOutput();
    int  CalcIsDead();
    int  Render(zTRenderContext& context);
    int  IsBBox3DLocal();
    int  GetVisualDied();
    zTBBox3D GetBBox3D();
    zSTRING GetVisualName();
    const zSTRING* GetFileExtension(int index);
    zCVisual* LoadVisualVirtual(const zSTRING& fileName) const;
    void SetVisualUsedBy(zCVob* vob);
    unsigned long GetRenderSortKey() const;
    void HostVobAddedToWorld(zCVob* vob, zCWorld* world);
    void HostVobRemovedFromWorld(zCVob* vob, zCWorld* world);

    static zCParticleEmitter* SearchParticleEmitter(const zSTRING& name);
    static zCParticleFX* Load(const zSTRING& fileName);
    static float PartRand();
    static zCObject* _CreateNewInstance();
    static void ParseParticleFXScript();
    static void InitParticleFX();
    static void CleanupParticleFX();

    zCClassDef* _GetClassDef() const;

    zSParticle*           firstPart;
    zCParticleEmitterVars emitterVars;
    zCParticleEmitter*    emitter;
    zTBBox3D              bbox3DWorld;
    zCVob*                connectedVob;
    int                   bboxUpdateCtr;
    unsigned char         emitterIsOwned : 1;
    unsigned char         dontKillPFXWhenDone : 1;
    unsigned char         dead : 1;
    unsigned char         isOneShotFX : 1;
    unsigned char         forceEveryFrameUpdate : 1;
    unsigned char         renderUnderWaterOnly : 1;
    zCParticleFX*         nextPfx;
    zCParticleFX*         prevPfx;
    float                 privateTotalTime;
    float                 lastTimeRendered;
    float                 timeScale;
    float                 localFrameTimeF;
    void*                 quadMark;
    zTBBox3D              quadMarkBBox3DWorld;
    float                 m_BboxYRangeInv;
    int                   m_bVisualNeverDies;

    static zCClassDef* classDef;
    static void operator delete(void* memory);
    static zCParser* s_pfxParser;
    static zCArraySort<zCParticleEmitter*> s_emitterPresetList;
    static zCParticleEmitter s_emitterDummyDefault;
    static int s_bAmbientPFXEnabled;
    static int s_globNumPart;
    static zSParticle* s_globPartList;
    static zSParticle* s_globFreePart;
    static zCMesh* s_partMeshTri;
    static zCMesh* s_partMeshQuad;
    static int s_showDebugInfo;
    static zCStaticPfxList s_pfxList;
};

class zCPFXPerFrameCallback {
public:

    void DoWorldPerFrameCallback(zCWorld* world, zCCamera* camera);
};

#endif
