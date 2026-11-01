#ifndef ZPOLYSTRIP_H
#define ZPOLYSTRIP_H

#include "z3d.h"

class zCPolyStrip : public zCVisual {
public:

    enum zTCamAlign {
        zCAM_ALIGN_NONE,
        zCAM_ALIGN_STRIP,
        zCAM_ALIGN_POLY
    };

    zCPolyStrip();
    virtual ~zCPolyStrip();

    void CalcBBox3D();
    void EmptyEffect();
    void AllocateResources(int count);
    void SetVisibleSegments(float first, float last);
    void AddSegment(const zVEC3& first, const zVEC3& second);
    void AddSegment(const zVEC3& center);
    void AddSegmentSmart(const zVEC3& first, const zVEC3& second);
    void UpdateSegment(int index, const zVEC3& first, const zVEC3& second, const zVEC3& center);
    void UpdateSegmentAlpha(int index, float alpha);
    void AlignToCamera();
    void ProjectToSphere(const zTBSphere3D& sphere);
    void Lightning0Subdivide(const zVEC3& first, const zVEC3& second, float amount, int depth);
    void GenerateCPLightning0(const zVEC3& first, const zVEC3& second, float amount);
    void GenerateCircle(const zTBSphere3D& sphere, const zVEC3& start, const zVEC3& normal);
    int  Render(zTRenderContext& context);
    int  IsBBox3DLocal();
    zTBBox3D GetBBox3D();
    void SetVisualUsedBy(zCVob* vob);
    void FreeResources();

    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual zSTRING GetVisualName();

    zCMaterial* material;
    zCVertex* vertList;
    zCPolygon* polyList;
    int numPoly;
    int numVert;
    zVEC3* centerPointList;
    float* alphaList;
    float width;
    zCVob* connectedVob;
    zTBBox3D bbox3D;
    zTCamAlign camAlign;
    int heightCheck;
    int everyFrameUpdate;
    float heightBound;
    int firstSeg;
    int lastSeg;
    int numSeg;
    float visLastFrac;
    float visFirstFrac;
    float alphaFadeSpeed;
    float newAlphaFadeSpeed;
    float newAlpha;
    int lastDirSeg;
    zVEC3 lastDirNormal;
    unsigned char localFOR : 1;

    static zCClassDef* classDef;
};

class zCLightning : public zCVisual {
public:

    class zCBolt : public zCPolyStrip {
    public:

        ~zCBolt();

        void Generate_R(const zVEC3& first, const zVEC3& second, float amount);
        void FreeResources();
        int  Render(zTRenderContext& context);

        zCArray<zCBolt*> childList;
    };

    int  Render(zTRenderContext& context);
    void SetVisualUsedBy(zCVob* vob);
    void SetProjectionSphere(const zTBSphere3D& sphere);
    void Generate(const zVEC3& first, const zVEC3& second);

    zCBolt rootBolt;
};

struct zTQuadMarkVert {
    zVEC3 position;
    zVEC2 texCoord;
};

struct zTQuadMarkPoly {
    ~zTQuadMarkPoly();

    zCArray<zTQuadMarkVert> vertex;
};

struct zTQuadMarkAniState {
    int    firstPolyIndex;
    int    lastPolyIndex;
    float  scaleStart;
    float  scaleCur;
    float  scaleSpeed;
    float  scaleSpeedCur;
    zVEC2  uvDelta;
    zVEC2  uvScale;
    int    texAniType;
    float  alpha;
};

struct zTQuadMarkEffectParams {
    int   texAniType;
    float texAniScaleStart;
    float texAniSpeed;
    float alphaFadeSpeed;
};

class zCQuadMark : public zCVisual {
public:

    zCQuadMark();
    virtual ~zCQuadMark();

    static zCObject* _CreateNewInstance();
    zCClassDef*      _GetClassDef() const;
    int              IsBBox3DLocal();
    zSTRING          GetVisualName();
    void             SetVisualUsedBy(zCVob* vob);
    zTBBox3D         GetBBox3D();
    int              Render(zTRenderContext& context);
    void             ProcessAniTracker();
    int              ClipQuadPolyToPlane(zTQuadMarkPoly& source,
        zTQuadMarkPoly& destination, const zTPlane& plane);
    void             CalcQuadAxis(zCPolygon* polygon, zVEC3& firstAxis, zVEC3& secondAxis);
    void             InitQuadPoly(zTQuadMarkPoly& destination, zCPolygon* polygon);
    void             ProjectQuadPolyToPlane(const zTQuadMarkPoly& source,
        zTQuadMarkPoly& destination, const zTPlane& plane);
    void             ClipQuadPolyToWorldPoly(zTQuadMarkPoly& polygon,
        zCPolygon* worldPoly, zCArray<int>& clippedEdges);
    void             InitFirstQuadPoly(zCPolygon* worldPoly, const zVEC3& center,
        const zVEC3& firstAxis, const zVEC3& secondAxis, zTQuadMarkEffectParams* params);
    void             AddQuadPolyToQuadMesh(zTQuadMarkPoly& polygon, zCPolygon* worldPoly);
    zCPolygon*       GetNextWorldPoly(zCVertex* first, zCVertex* second);
    void             ProcessWorldPoly(zCPolygon* worldPoly);
    void             CreateQuadMark(const zVEC3& origin, const zVEC3& direction,
        const zVEC2& size, zSTRING& textureName, zTQuadMarkEffectParams* params);
    void             CreateQuadMark(const zVEC3& origin, const zVEC3& direction,
        const zVEC2& size, zCMaterial* materialValue, zTQuadMarkEffectParams* params);
    void             CreateQuadMark(const zVEC3& origin, const zVEC3& direction,
        const zVEC2& size, zTQuadMarkEffectParams* params);
    void             CreateQuadMark(zCPolygon* worldPoly, const zVEC3& center,
        const zVEC2& size, zTQuadMarkEffectParams* params);
    int              GetNumActive();

    zCMesh*                     quadMesh;
    zCVob*                      connectedVob;
    zCMaterial*                 material;
    zCMaterial*                 actMaterial;
    float                       alphaFadeSpeed;
    int                         dontRepositionConnectedVob;
    zCArray<zTQuadMarkAniState> quadMarkAniTracker;
    zCPolygon**                 worldPolyList;
    int                         numWorldPoly;
    zCArray<zCPolygon*>         visitedPolyList;
    zTQuadMarkPoly              firstQuadPoly;
    zCPolygon*                  firstWorldPoly;
    zVEC3                       averageNormal;

    static zCClassDef* classDef;
    static void operator delete(void* memory);
};

#endif
