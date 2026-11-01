#ifndef ZCOLLISION_DETECTOR_H
#define ZCOLLISION_DETECTOR_H

#include "zModel.h"
#include "zRayCache.h"
#include "zMemPool.h"
#include "zCache.h"

class zCCollisionObject;

class zCCollisionObjectDef {
public:

    zCCollisionObjectDef(int volatileValue, zCCollisionObject* (*factory)());
    ~zCCollisionObjectDef();

    zCCollisionObject* (*m_createNewInstance)();
    int m_bIsVolatile;
    int m_iIndex;

    static zCArray<zCCollisionObjectDef*>* s_aoCollObjClassList;
};

class zCCollisionReport {
public:

    zCCollisionReport();
    virtual ~zCCollisionReport();

    float m_fTimeStamp;
    zVEC3 m_oContactPoint;
    zVEC3 m_oCollNormal0;
    zVEC3 m_oCollNormal1;
    zCCollisionObject* m_poCollObject0;
    zCCollisionObject* m_poCollObject1;
};

typedef int zTStaticIntersectionTest(float, zCCollisionObject*, const zMAT4&,
    zCCollisionObject*, const zMAT4&, zCCollisionReport**);
typedef int zTDynamicIntersectionTest(zCCollisionObject*, zCCollisionObject*, zCCollisionReport**);

class zCCollisionObject {
public:

    zCCollisionObject() {}
    virtual ~zCCollisionObject() {}
    static zCCollisionObject* _CreateNewInstance();
    virtual zCCollisionObjectDef* GetCollObjClass() const;
    virtual void DetectCollisionsSelf(const zCArray<zCCollisionObject*>& objects,
        zCArray<zCCollisionReport*>& reports);
    virtual void CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags);
    virtual float GetAdaptiveStepSize();
    virtual int SuppressCollisionResponse();
    void CalcIntermediateTrafo(float time, zMAT4& result);
    virtual void GetLargestBBox3DLocal(zTBBox3D& box);

    zMAT4 m_oTrafoOld;
    zMAT4 m_oTrafoNew;
    void* m_pvClientData;
    int m_bHintTranslation;

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectUndef : public zCCollisionObject {
public:

    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectLevelPolys : public zCCollisionObject {
public:

    ~zCCollObjectLevelPolys();
    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;

    zCArray<zCPolygon*> m_aoPolyList;

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectBase : public zCCollisionObject {
public:

    ~zCCollObjectBase() {}
    void GetLargestBBox3DLocal(zTBBox3D& box);

    static float S_CalcAdaptiveCollStepFromVisualBBox(zCVisual* visual);
    static void S_CalcGeneric_CollNormal_ContactPoint(const zCVob* first,
        const zCVob* second, zCCollisionReport& report);
    static void S_RegisterCollisionTestFuncs(zCCollisionDetector* detector);
};

int TestDynamic_Projectile_Char(zCCollisionObject* projectile, zCCollisionObject* other,
    zCCollisionReport** report);
int TestDynamic_XXX_BoxPassThrough(zCCollisionObject* first, zCCollisionObject* second,
    zCCollisionReport** report);
int TestStatic_Char_Char(float time, zCCollisionObject* first, const zMAT4& firstTrafo,
    zCCollisionObject* second, const zMAT4& secondTrafo, zCCollisionReport** report);

class zCCacheIndex {
public:

    zCVob* voba;
    zCVob* vobb;
};

class zCConvexCollisionCache {
public:

    zCConvexCollisionCache(const zCCacheIndex& index);

    zVEC3 planeNormal;
    int invert;

    static zCMemPool<zCConvexCollisionCache> s_MemPool;

    static void operator delete(void* memory);
};

class zCConvexPrimitive {
public:

    virtual ~zCConvexPrimitive() {}
    virtual void PointNearestToPlaneLocal(const zVEC3& point, zVEC3& result) const = 0;
    virtual void GetCenterLocal(zVEC3& center) const;
    virtual int PointIsInLocal(const zVEC3& point) const = 0;
    virtual int SymetrySphere() const;
    virtual int SymetryRotation(zVEC3& axis) const;
    virtual void UpdateBBox();
    virtual void DrawVirtual() const;

    void PointNearestToPlane(zVEC3& result, const zVEC3& point,
        const zMAT4& trafo, const zMAT3& rotation) const;
    void Draw(const zMAT4& trafo, zCOLOR color, const zCCamera* camera) const;

    static int CheckCollision(const zCConvexPrimitive* first, const zCConvexPrimitive* second,
        const zMAT4& firstTrafo, const zMAT4& secondTrafo, zCConvexCollisionCache* cache,
        zVEC3& contact, float& distance, zVEC3& normal);
    static float Distance(const zCConvexPrimitive* first, const zCConvexPrimitive* second,
        const zMAT4& firstTrafo, const zMAT4& secondTrafo, const zVEC3& direction,
        zVEC3& contact, zVEC3& normal);
    static zCConvexCollisionCache* GetCache(const void* first, const void* second);
    static void DrawLine(const zVEC3& first, const zVEC3& second);
    static void DrawLineStrip(const zVEC3& point);

    zTBBox3D bbox3Dlocal;
};

class zCConvexPrimitiveUnitSphere : public zCConvexPrimitive {
public:

    zCConvexPrimitiveUnitSphere();
    virtual ~zCConvexPrimitiveUnitSphere();
    virtual void PointNearestToPlaneLocal(const zVEC3& point, zVEC3& result) const;
    virtual int PointIsInLocal(const zVEC3& point) const;
    virtual int SymetrySphere() const;
    virtual int SymetryRotation(zVEC3& axis) const;
    virtual void UpdateBBox();
    virtual void DrawVirtual() const;

    static zCConvexPrimitiveUnitSphere s_unitSphere;
};

class zCConvexPrimitiveScaleTrans : public zCConvexPrimitive {
public:

    zCConvexPrimitiveScaleTrans(zCConvexPrimitive* original, const zVEC3& scale,
        const zVEC3& translation);
    virtual ~zCConvexPrimitiveScaleTrans();
    virtual void PointNearestToPlaneLocal(const zVEC3& point, zVEC3& result) const;
    virtual void GetCenterLocal(zVEC3& center) const;
    virtual int PointIsInLocal(const zVEC3& point) const;
    virtual void UpdateBBox();
    virtual void DrawVirtual() const;

    zVEC3 scale;
    zVEC3 translation;
    zCConvexPrimitive* original;
};

class zCConvexPrimitiveEllipsoid : public zCConvexPrimitiveScaleTrans {
public:

    zCConvexPrimitiveEllipsoid(const zVEC3& center, const zVEC3& radii);
    zCConvexPrimitiveEllipsoid(const zTBBox3D& box, int snap);
    virtual ~zCConvexPrimitiveEllipsoid();
    virtual int SymetryRotation(zVEC3& axis) const;
    void Snap();

    int symetric;
};

class zCCollObjectCharacter : public zCCollObjectBase {
public:

    zCCollObjectCharacter();
    ~zCCollObjectCharacter();
    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;
    int IsDead() const;
    int DontLetHimComeNearer(const zCVob* other) const;
    float TurnSpeed() const;
    void PushAround(const zVEC3& direction);
    zCConvexPrimitive* GetPrimitiveCC() const;
    void GetFallbackTrafoCC(const zMAT4& oldTrafo, const zMAT4& currentTrafo,
        const zVEC3& center, int flags, zMAT4& result) const;

    static zVEC3 S_SlideCC(const zVEC3& direction, float time, const zVEC3& normal);
    static int S_CalcAvgNormalVariance(const zTTraceRayReport (&reports)[4],
        zVEC3& averageNormal, float& variance);
    void GetCollisionPoleYDims(float& minY, float& maxY) const;
    int CalcSlideVector(const zVEC3& direction, const zVEC3& normal,
        zVEC3& slide, zVEC3& remaining, float& projection);
    void DoWallSlidingLatentRotation(zMAT4& trafo);
    void FindFloorWaterCeiling(const zVEC3& position, struct zTSpatialState& state);
    void MarkSpatialStateAsUninited(int refresh);
    zVEC3 GetSurfaceAlignScanOrigin(const zVEC3& origin);
    int TestHardCollisions(const zVEC3& start, const zVEC3& end, zVEC3& position,
        zTSpatialState& state, zVEC3& normal);
    int TestSoftCollisions(const zVEC3& direction, const zVEC3& normal,
        zVEC3& slide, zVEC3& remaining);
    void DetectCollisionsSelf(const zCArray<zCCollisionObject*>& objects,
        zCArray<zCCollisionReport*>& reports);
    void CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags);
    void GetLargestBBox3DLocal(zTBBox3D& box);

    struct zTSpatialState {
        float m_fFloorY;
        float m_fLastFloorY;
        float m_fWaterY;
        float m_fCeilingY;
        zCPolygon* m_poFloorPoly;
        zCPolygon* m_poWaterPoly;
        zCPolygon* m_poCeilingPoly;
        unsigned char m_bFloorIsStair;
    } m_oSpatialState;

    struct zTCharacterConfig {
        unsigned char zMV_STEP_HEIGHT;
        unsigned char zMV_JUMP_UP_MIN_CEIL;
    } m_oConfig;

    zCRayCache m_oUpRayCache;
    zCRayCache m_oDownRayCache;
    zCRayCache m_oFrontRayCache;
    zCRayCache m_oBackRayCache;
    zCRayCache m_oLeftRayCache;
    zCRayCache m_oRightRayCache;
    zVEC3 m_oWallSlideRotAxis;
    float m_fWallSlideTimer;
    float m_fWallSlideRotDir;

    zCConvexPrimitive* m_poPrimitiveCC;

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectPoint : public zCCollObjectBase {
public:

    ~zCCollObjectPoint() {}
    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;
    void GetLargestBBox3DLocal(zTBBox3D& box);
    void CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags);
    zVEC3 GetCenterOffsetLocal() const;
    zVEC3 GetCenterOffsetNew() const;
    void SetUpVectorNew(const zVEC3& direction);

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectProjectile : public zCCollObjectPoint {
public:

    ~zCCollObjectProjectile() {}
    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectComplex : public zCCollObjectBase {
public:

    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;
    float GetAdaptiveStepSize();
    void CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags);

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectBoxBlocker : public zCCollObjectBase {
public:

    ~zCCollObjectBoxBlocker() {}
    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollObjectBoxPassThrough : public zCCollObjectBase {
public:

    ~zCCollObjectBoxPassThrough() {}
    static zCCollisionObject* _CreateNewInstance();
    zCCollisionObjectDef* GetCollObjClass() const;
    int SuppressCollisionResponse();

    static zCCollisionObjectDef s_oCollObjClass;
};

class zCCollisionDetector {
public:

    zCCollisionDetector();
    virtual ~zCCollisionDetector();
    int GetTestFuncIndex(zCCollisionObjectDef* first, zCCollisionObjectDef* second);
    zTStaticIntersectionTest* GetStaticTestFunc(zCCollisionObjectDef* first,
        zCCollisionObjectDef* second);
    zTDynamicIntersectionTest* GetDynamicTestFunc(zCCollisionObjectDef* first,
        zCCollisionObjectDef* second);
    virtual void RegisterStaticIntersectionTest(zCCollisionObjectDef* first,
        zCCollisionObjectDef* second, zTStaticIntersectionTest* function);
    virtual void RegisterDynamicIntersectionTest(zCCollisionObjectDef* first,
        zCCollisionObjectDef* second, zTDynamicIntersectionTest* function);
    virtual int CanCollide(zCCollisionObjectDef* first, zCCollisionObjectDef* second);
    void DoDynamicIntersectionTests(zCCollisionObject* object, zCArray<zCCollisionObject*>& objects,
        zCArray<zCCollisionReport*>& reports);
    void DoStaticIntersectionTests(zCCollisionObject* object, zCArray<zCCollisionObject*>& objects,
        zCArray<zCCollisionReport*>& reports);
    virtual void DetectCollisions(zCCollisionObject* object,
        const zCArray<zCCollisionObject*>& objects, zCArray<zCCollisionReport*>& reports);

    static void S_ResetFreeCollReportList();
    static zCCollisionReport* S_GetNextFreeCollReport();
    static unsigned long s_iCollisionDetectorFrameCtr;
    static int s_iCurrentCollReport;
    static zCCollisionReport s_aoCollReportList[16];

    zTStaticIntersectionTest** m_apStaticTestFuncMatrix;
    zTDynamicIntersectionTest** m_apDynamicTestFuncMatrix;
};

#endif
