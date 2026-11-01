#ifndef Z3D_H
#define Z3D_H

#include "zModel.h"
#include "zMemPool.h"

class zCPolygon;
struct zCVertex;

struct zCVertexTransform {
    ~zCVertexTransform();

    zCVertex* untransformed;
    zVEC3     vertCamSpace;
    float     vertCamSpaceZInv;
    float     vertScrX;
    float     vertScrY;

    static zCVolatileMemory<zCVertexTransform> s_MemMan;
};

struct zCVertex {
    zCVertex();
    ~zCVertex();

    void ResetVertexTransform();
    void Draw(const zCOLOR& color, int mode);
    zCVertexTransform* CreateVertexTransform();
    static void ResetVertexTransforms();

    zVEC3 position;
    int   transformedIndex;
    int   myIndex;

    static zCVertexTransform* transformedVertexList;
    static zCMemPool<zCVertex> s_MemPool;
};

struct zCVertFeature {
    zCVertFeature();

    zVEC3         vertNormal;
    unsigned char color[4];
    unsigned char lightStat[4];
    unsigned char lightDyn[4];
    zVEC3         mapping;

    static zCMemPool<zCVertFeature> s_MemPool;
};

struct zTPlane {
    int     ClassifyPointBSPBuild(const zVEC3& point) const;
    zSTRING Print() const;
    void    Calc(const zVEC3& first, const zVEC3& second, const zVEC3& third);
    void    GetOriginUpRight(zVEC3& origin, zVEC3& up, zVEC3& right) const;
    int     CalcSignbits() const;

    zVEC3 normal;
    float distance;
};

struct zTBSphere3D {
    int  IsIntersecting(const zVEC3& start, const zVEC3& end) const;
    int  IsIntersecting(const zTBSphere3D& other) const;
    int  ClassifyPlane(zTPlane plane) const;
    void Draw(const zCOLOR& color) const;

    zVEC3 center;
    float radius;
};

struct zTBCylinder3D {
    int  IsIntersecting(const zTBCylinder3D& other) const;
    void Draw(const zCOLOR& color) const;
    void SetByBBox3D(zTBBox3D box);

    zVEC3 center;
    float heightY;
    float radius;
};

class zCLightMap : public zCObject {
public:

    zCLightMap();
    void SetTexture(zCTexture* texture);
    static void operator delete(void* memory);

    zVEC3 lightmapOrigin;
    zVEC3 lightmapUVUp;
    zVEC3 lightmapUVRight;
    zCTexture* tex;
    zVEC3* colorBuffer;
    int xdim;
    int ydim;

    static zCClassDef* classDef;
};

struct zSVertexSavedValues {
    zCVertex* vert;
    float     offset;
};

struct zSFeatureSavedValues {
    zCVertFeature* feat;
    unsigned long  color;
    zVEC3          mapping;
    zVEC3          vertNormal;
};

struct zSPolySavedValues {
    zCPolygon* poly;
    zTPlane    plane;
};

class zCPolygon {
public:

    zCPolygon();
    ~zCPolygon();
    static void operator delete(void* memory);

    int  ClipToPlane(zCVertex** vertices, zCVertFeature** features, int numVerts,
        zCVertex** outVertices, zCVertFeature** outFeatures, int& outNumVerts,
        const zTPlane& plane);
    int  ClipToPlane_Occluder(zCVertex** vertices, int numVerts, zCVertex** outVertices,
        int& outNumVerts, const zTPlane& plane);
    void Unclip();
    void Unclip_Occluder();
    void CopyClipFeaturesForTexAniMapping();
    int  ClipToFrustum(int planeMask);
    int  ClipToFrustum_Occluder(int planeMask);
    int  ClassifyPlane(const zTPlane& plane);
    int  ClassifyPlaneBSPBuild(const zTPlane& plane);
    int  CalcNormal();
    int  CalcNormalApprox();
    void GetScreenBBox2D(struct zTBBox2D& bbox) const;
    void DrawWire(unsigned long color) const;
    void LightDynCamSpace(const zVEC3& lightPosition, int lightRange);
    zVEC3 GetLightStatAtPos(const zVEC3& position);
    int  CheckRayPolyIntersection(const zVEC3& origin, const zVEC3& direction,
        zVEC3& intersection, float& distance) const;
    int  CheckRayPolyIntersection2Sided(const zVEC3& origin, const zVEC3& direction,
        zVEC3& intersection, float& distance) const;
    zTBBox3D GetBBox3D() const;
    zVEC3 GetCenter();
    void SetMaterial(zCMaterial* material);
    void SetMapping(const zVEC2& first, const zVEC2& second, const zVEC2& third);
    void SetMapping(int index, const zVEC2& value);
    void Flip();
    int  VertPartOfPoly(zCVertex* vertex) const;
    int  EdgePartOfPoly(zCVertex* first, zCVertex* second) const;
    int  EdgePositionsPartOfPoly(zCVertex* first, zCVertex* second) const;
    void TexTranslate(const zVEC2& offset);
    void TexSetTranslation(const zVEC2& offset);
    void TexScale(float u, float v);
    void TexScale(zVEC2& center, float u, float v);
    void TexRotate(zVEC2& center, float angle);
    void TexMirrorU(float value);
    void TexMirrorV(float value);
    void TexShearU(float value);
    void TexShearV(float value);
    void TexApplyPlanarMapping(const zVEC3& origin, const zVEC3& right, const zVEC3& up,
        int useTexScale);
    void TexApplyPlanarMapping(float scaleU, float scaleV, int useTexScale);
    void TexCorrectUV();
    void AllocVerts(int count);
    void AddVertex(zCVertex* value);
    void AddVertexAndFeature(zCVertex* vertexValue, zCVertFeature* featureValue);
    void SetVertices(zCVertex* first, zCVertex* second, zCVertex* third);
    void SetVertices(zCVertex* first, zCVertex* second, zCVertex* third, zCVertex* fourth,
        int calcNormal);
    void SetVertex(int index, zCVertex* value);
    void SetFeature(int index, zCVertFeature* value);
    void CopyValuesInto(zCPolygon* destination) const;
    void CopyPolyInto(zCPolygon* destination) const;
    void RemoveVerticesAndFeatures();
    float GetArea() const;
    float GetLongestEdgeLen() const;
    void ResetStaticLight();
    void SetStaticLight(unsigned long color);
    void SetLightDyn(unsigned long color);
    void SetLightStatAlpha(unsigned char alpha);
    void ResetLightDynToLightStat();
    void AddToLightDyn(unsigned long color);
    void LightStatic(zCVobLight& light, const zVEC3& lightPosition, zMAT4& trafo, zCWorld* world);
    void SmoothSectorBorder(zCWorld* world);
    int  CheckBBoxPolyIntersection(const zTBBox3D& bbox) const;
    int  CheckBSpherePolyIntersection(const struct zTBSphere3D& sphere) const;
    void SplitPoly(const zTPlane& plane, zCPolygon* front, zCPolygon* back, zCMesh* mesh);
    int  IsIntersectingProjection(zCPolygon* other, const zVEC3& direction);
    int  IsIntersecting(zCPolygon* other);
    int  IsNeighbourOf(const zCPolygon* other);
    int  GetStaticLODPossible();

    static void AlignPolyListToAvgPlane(zCArray<zCPolygon*> polygons);
    static void TexApplyPlanarMapping(const zCList<zCPolygon>& polygons, float scaleU,
        float scaleV, int useTexScale);
    static void LightPolyList(const zCArray<class zCVobLight*>& lights, zCPolygon** polygons,
        int numPolygons, zMAT4* trafo);

    static void* S_AllocPointers(int count);
    static void S_DeletePointers(int count, void* pointers);
    static zCVertFeature* S_AllocVertFeatures(int count);
    static void S_DeleteVertFeatures(int count, zCVertFeature* features);
    static void S_InitMorph();
    static void S_EmptyMorphedVerts();
    static void S_ResetMorphedVerts();
    static void S_DeleteMorphedVerts();
    static void PrepareRendering();

    zCVertex** vertex;
    int lastTimeDrawn;
    zTPlane polyPlane;
    zCMaterial* material;
    zCLightMap* lightmap;
    zCVertex** clipVert;
    zCVertFeature** clipFeat;
    int numClipVert;
    zCVertFeature** feature;
    unsigned char polyNumVert;
    unsigned char flags[3];

    static zCVertex* s_clipVertScene;
    static int s_numClipVertScene;
    static zCVertFeature* s_clipFeatScene;
    static int s_numClipFeatScene;
    static int s_numVertListScene;
    static int s_numFeatListScene;
    static zCVertex** s_actClipVert;
    static int s_actNumClipVert;
    static zCVertFeature** s_featListScene;
    static zCArraySort<zSVertexSavedValues> morphedVerticesList_Height;
    static zCArraySort<zSVertexSavedValues> morphedVerticesList_Wall;
    static zCArraySort<zSFeatureSavedValues> morphedFeatureList;
    static zCArray<zSPolySavedValues> morphedPolysSavedValuesList;
    static zCMemPool<zCPolygon> s_MemPool;
};

struct zVEC4 {
    zVEC4() {}
    zVEC4(const zVEC4& value) { n[0] = value.n[0]; n[1] = value.n[1]; n[2] = value.n[2]; n[3] = value.n[3]; }
    zVEC4(const zVEC3& value, float w);
    zVEC4(float x, float y, float z, float w)
    {
        n[0] = x;
        n[1] = y;
        n[2] = z;
        n[3] = w;
    }

    float n[4];
};

class zCCamera {
public:

    zCCamera();
    ~zCCamera();
    void PushTransform(int type);
    void PopTransform(int type);
    void SetTransform(int type, const zMAT4& trafo);
    const zMAT4& GetTransform(int type) const;
    void Activate();
    void CreateProjectionMatrix(zMAT4& result, float nearClip, float farClip,
        float horizontalFOV, float verticalFOV);
    void SetRenderTarget(void* viewport);
    void Project(const zVEC3* point, float scale, float& x, float& y) const;
    void Project(const zVEC3* point, float& x, float& y) const;
    void Project(const zVEC3* point, int& x, int& y);
    void ProjectClamp(zCVertexTransform* transform, float depthInv) const;
    void SetFarClipZ(float farClip);
    void UpdateProjectionMatrix();
    void UpdateViewport();
    void SetRenderScreenFadeTex(zSTRING fileName);
    void SetFOV(float fov);
    void SetFOV(float horizontalFOV, float verticalFOV);
    float GetFOV() const;
    void GetFOV(float& horizontalFOV, float& verticalFOV) const;
    zMAT4* SetUpFrustum();
    int  BBox3DInFrustum(const zTBBox3D& box);
    int  BBox3DInFrustum(const zTBBox3D& box, int& planeMask);
    int  ScreenProjectionTouchesPortalRough(const zTBBox3D& box, const zTBBox2D& portal);
    int  ScreenProjectionTouchesPortal(const zTBBox3D& box, const zTBBox2D& portal);
    void StopTremor();
    void AddTremor(const zVEC3& amplitude, float velocity, float range, const zVEC3& origin);
    void PreRenderProcessing();
    void PostRenderProcessing();
    void GetCamPos(zVEC3& position) const;
    void InitDrawPolySimple();
    void CleanupDrawPolySimple();
    void DrawPolySimple(struct zTCamVertSimple* vertices, int count,
        zCMaterial* material, int flags);
    void SetRenderScreenFade(zCOLOR color);
    void SetRenderScreenFadeTexAniFPS(float value);
    void SetRenderScreenFadeTexBlendFunc(int value);
    void SetRenderCinemaScope(zCOLOR color);
    void RenderScreenFade();
    void RenderCinemaScope();

    struct zTCamVertSimple {
        float x;
        float y;
        float z;
        float u;
        float v;
        float pad;
    };

    struct zTCamTrafoStack {
        zMAT4 stack[8];
        int pos;
    } trafoViewStack, trafoWorldStack, trafoWorldViewStack;

    zSTRING screenFadeTexture;
    zMAT4 trafoWorld;
    zMAT4 trafoProjection;
    zMAT4 trafoViewInv;
    zMAT4 trafoView;
    float nearClipZ;
    float farClipZ;
    float fovH;
    float fovV;
    void* targetView;
    zTPlane frustumplanes[6];
    unsigned char signbits[6];
    zMAT4 camMatrix;
    zMAT4 camMatrixInv;
    float viewDistanceX;
    float viewDistanceY;
    float viewDistanceXInv;
    float viewDistanceYInv;
    zCVob* connectedVob;
    struct {
        float xcenter;
        float ycenter;
        int xdim;
        int ydim;
        int x0;
        int y0;
        int x1;
        int y1;
    } vpData;
    zVEC3 tremorAmplitude;
    zVEC3 tremorOrigin;
    int tremorToggle;
    float tremorScale;
    float tremorVelo;
    float topBottomCos;
    float topBottomSin;
    float leftRightCos;
    float leftRightSin;
    int projection;
    int drawMode;
    int shadeMode;
    int drawWire;
    int screenFadeEnabled;
    zCOLOR screenFadeColor;
    float screenFadeTextureAniFPS;
    int screenFadeTextureBlendFunc;
    int cinemaScopeEnabled;
    zCOLOR cinemaScopeColor;
    zCMesh* polyMesh;
    zCPolygon* poly;
    zCMaterial* polyMaterial;
    zTCamVertSimple polyCamVerts[4];

    static zCCamera* activeCam;
    static zVEC3 activeCamPos;
};

struct zTBBox2D {
    zTBBox2D();
    int  Classify(const zTBBox2D& other) const;
    void ClipToBBox2D(const zTBBox2D& other);
    void Draw() const;

    zVEC2 mins;
    zVEC2 maxs;
};

class zCVobLight {
public:

    int Release();

    zVEC3 position;
    zVEC3 positionMax;
    zVEC3 positionWorld;
    unsigned char color[4];
    float range;
    float rangeInv;
    int lightType;
};

struct zTEngineStats {
    int numVobAI;
    int numVobAI_only;
    int numVobAI_full;
    int numVob_EndMovement;
    int numVobFirstTestInterpen;
    int numModelUpdateBBoxTree;
    int numPolysClipped;
    int numPlanesClipped;
    int numTraceRay;

    void PrintScreen(int x, int y);
};

class zCViewBase;

class zCLineCache {
public:

    struct zTCacheEntry {
        int    x1;
        int    y1;
        int    x2;
        int    y2;
        zCOLOR color;
    };

    zCLineCache() : numEntries(0), nextFree(0), cache(0) {}
    ~zCLineCache();
    void Line(int x1, int y1, int x2, int y2, zCOLOR color);
    void Flush(zCViewBase* view);
    void Line3D(const zVEC3& start, const zVEC3& end, zCOLOR color, int mode);
    void LineCS3D(const zVEC3& start, const zVEC3& end, zCOLOR color);

    int          numEntries;
    int          nextFree;
    zTCacheEntry* cache;
};

extern zCLineCache zlineCache;

zVEC4 Alg_FindPlaneCoeffApprox(const zVEC3& first, const zVEC3& second, const zVEC3& third);

int Compare_Verts(const void* ele1, const void* ele2);
int Compare_Features(const void* ele1, const void* ele2);

#endif
