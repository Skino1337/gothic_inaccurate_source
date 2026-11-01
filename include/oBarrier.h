#ifndef OBARRIER_H
#define OBARRIER_H

#include "zPolyStrip.h"
#include "zSound.h"
#include "zSky_Outdoor.h"

class zCDecal : public zCVisual {
public:

    zCDecal();
    virtual ~zCDecal();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsBBox3DLocal();
    static void CleanupDecals();
    static void CreateDecalMeshes();
    virtual const zSTRING* GetFileExtension(int index);
    virtual zCVisual* LoadVisualVirtual(const zSTRING& fileName) const;
    virtual unsigned long GetRenderSortKey() const;
    void SetTexture(const zSTRING& fileName);
    void SetTexture(zCTexture* texture);
    void SetDecalDim(float width, float height);
    virtual int Render(zTRenderContext& context);
    virtual void SetVisualUsedBy(zCVob* vob);
    virtual zTBBox3D GetBBox3D();
    virtual zSTRING GetVisualName();
    virtual int GetAlphaTestingEnabled() const;
    virtual void DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCMaterial* decalMaterial;
    float xdim;
    float ydim;
    float xoffset;
    float yoffset;
    int decal2Sided;
    int ignoreDayLight;
    int m_bOnTop;

    static zCClassDef* classDef;
    static zCMesh* decalMesh1Sided;
    static zCMesh* decalMesh2Sided;
};

enum zTThunderSector {
    zTThunderSector_None,
    zTThunderSector_1,
    zTThunderSector_2,
    zTThunderSector_3,
    zTThunderSector_4
};

struct myVert {
    int vertIndex;
    int vertNeighbours[8];
    int numNeighbours;
    int polyIndices[50];
    int numPolyIndices;
    int active;
};

struct myThunder {
    myThunder();
    ~myThunder();

    zVEC3 originVec;
    myThunder* childs;
    int numChilds;
    float startTime[5];
    zCPolyStrip* polyStrip;
    int numSegs;
    int valid;
    float t0;
    float t1;
    int numSplits;
    int dead;
    int isChild;
    zTThunderSector sector;
};

struct myPoly {
    int Alpha;
};

class oCBarrier {
public:

    oCBarrier();
    ~oCBarrier();
    void Init();
    void Initialise(int maxThunders);
    void AddTremor(zTRenderContext& context);
    void RenderLayer(zTRenderContext& context, int layer, int& alpha);
    int Render(zTRenderContext& context, int alpha, int tremor);
    void InitThunder(myThunder* thunder);
    void RemoveThunder(myThunder* thunder);
    int AddThunderSub(myThunder* thunder, int start, int end, int splits, int depth);
    int AddThunder(int start, int end, float duration, zTThunderSector sector);
    int RenderThunder(myThunder* thunder, zTRenderContext& context);
    void RenderThunderList(zTRenderContext& context);

    zCMesh* skySphereMesh;
    myPoly* myPolyList;
    myVert* myVertList;
    int numMyVerts;
    int numMyPolys;
    myThunder* myThunderList;
    int numMaxThunders;
    int numMyThunders;
    int actualIndex;
    int rootBoltIndex;
    int startPointList1[10];
    int numStartPoints1;
    int startPointList2[10];
    int numStartPoints2;
    int startPointList3[10];
    int numStartPoints3;
    int startPointList4[10];
    int numStartPoints4;
    int topestPoint;
    int bFadeInOut;
    int fadeState;
    int fadeIn;
    int fadeOut;
    int bFlashing;
    float fFlashTime;
    zCSoundFX* sfx1;
    int sfxHandle1;
    zCSoundFX* sfx2;
    int sfxHandle2;
    zCSoundFX* sfx3;
    int sfxHandle3;
    zCSoundFX* sfx4;
    int sfxHandle4;
    zCDecal* thunderStartDecal;
    int activeThunder_Sector1;
    int activeThunder_Sector2;
    int activeThunder_Sector3;
    int activeThunder_Sector4;
    zVEC2* originalTexUVList;
};

class oCSkyControler_Barrier : public zCSkyControler_Outdoor {
public:

    oCSkyControler_Barrier();
    virtual ~oCSkyControler_Barrier();
    virtual void RenderSkyPre();

    oCBarrier* barrier;
    int bFadeInOut;
};

#endif
