#ifndef ZBSP_H
#define ZBSP_H

#include "z3d.h"

class zCRayTurbo;
class zCRayCache;
class zCBspNode;
class zCBuildPortal;

class zCMeshOctreeNode {
public:

    zCMeshOctreeNode(int root);
    ~zCMeshOctreeNode();
    void EnlargeBox(const zCArray<zCPolygon*>& polygons);
    void CopyPolys(const zCArray<unsigned char>& mask, const zCArray<zCPolygon*>& source,
        int count, zCArray<zCPolygon*>& destination);
    void Build(const zCArray<zCPolygon*>& polygons, int threshold);

    zCMeshOctreeNode* children[8];
    zTBBox3D box;
    int flags;
    zCArray<zCPolygon*> polygons;
};

class zCRayTurbo {
public:

    zCRayTurbo(const zCBspNode& node);
    ~zCRayTurbo();
    void CalculateOptimizedPolyList(zCArray<zCPolygon*>& polygons);
    void TraverseTree(zCArray<zCPolygon*>& polygons, const zCMeshOctreeNode& node);

    const zCBspNode* bspNode;
    zCMeshOctreeNode meshOctreeRoot;
};

class zCRayTurboAdmin {
public:

    struct zSNode {
        const zCBspNode* key;
        zCRayTurbo* value;
        unsigned long hash;
        zSNode* next;
    };

    zCRayTurboAdmin();
    ~zCRayTurboAdmin();
    zCRayTurbo* GetRayTurboItem(const zCBspNode* node) const;
    zCRayTurbo* CreateRayTurboItem(const zCBspNode* node);
    zCRayTurbo* GetOrCreate(const zCBspNode* node);
    int CanHaveSpatialInfo(const zCBspNode* node);
    void ReleaseSpatialDatabase();

    static zCRayTurboAdmin& GetInstance();
    static void SetPolyTreshold(int value);
    static int GetPolyTreshold();

    static int s_iPolyTreshold;

    zCArray<zSNode*> m_mapRayTurbos;
};

class zCBspBase {
public:

    static void ReleaseStaticReferences();
    void        DescribeTree(int depth);
    void        AddVobsToRenderListSubtree(int flags);
    int         CheckRayAgainstPolys(const zVEC3& start, const zVEC3& end,
        const zVEC3& direction, zCRayTurbo* turbo) const;
    int         CheckRayAgainstPolysNearestHit(const zVEC3& start, const zVEC3& end,
        const zVEC3& direction, zCRayTurbo* turbo) const;
    int         CheckRayAgainstPolysCache(const zVEC3& start, const zVEC3& end,
        const zVEC3& direction) const;
    int         RayIntersection(const zVEC3& start, const zVEC3& end, zVEC3& direction) const;
    void        AddVobToLeafs(zCVob* vob);
    void        AddVobLightToLeafs(zCVobLight* vob);
    void        RemoveVobFromLeafs(zCVob* vob);
    void        CollectVobsSubtree(zCArray<zCVob*>& vobs) const;
    void        CollectVobsInBBox3D(zCArray<zCVob*>& vobs, const zTBBox3D& box) const;
    void        CollectVobsSubtree_I(zCArray<zCVob*>& vobs) const;
    void        CollectLightVobsSubtree(zCArray<zCVob*>& vobs) const;
    void        CollectLightVobsSubtree_I(zCArray<zCVob*>& vobs) const;
    void        CollectVobsInBBox3D_I(zCArray<zCVob*>& vobs, const zTBBox3D& box) const;
    void        CollectPolysInBBox3DRec() const;
    int         CollectPolysInBBox3D(const zTBBox3D& box, zCPolygon**& polygons, int& count) const;
    void        SortNodePolys(zCBspTree* tree);
    void        SaveBINRec(zCFileBIN& file);
    void        LoadBINRec(zCFileBIN& file);
    zCBspLeaf*  FindLeaf(const zVEC3& position);
    zTBBox2D    GetScreenBBox2D() const;
    void        CollectNodeLights();
    void        RenderTrivInIndoor();
    void        RenderIndoor(int flags);
    void        RenderNodeOutdoor(int flags);
    static void RenderOutdoor(zCBspBase* node, zTBBox3D box, int flags, int collectVobs);
    int         CountNodes();

    zCBspNode*  parent;
    zTBBox3D    bbox3D;
    zCPolygon** polyList;
    int         numPolys;
    int         nodeType;
    int         numLeafs;
    zCBspBase*  front;
    zCBspBase*  back;
    int         leafVobs;
    int         leafLights;
};

class zCBspNode : public zCBspBase {
public:

    zCBspNode();
    void CalcPlaneSignbits(zCBspTree* tree);
    void RemovePoly(zCPolygon* polygon);

    zTPlane    plane;
    zCBspLeaf* leafList;
    unsigned char planeSignbits;
};

struct zTPolyNode {
    zCPolygon* poly;
    zTPolyNode* next;
    int marked;
    unsigned char isCompleteInside;
};

class zCCBspNode {
public:

    zCCBspNode(zCCBspNode* parentNode = NULL);
    ~zCCBspNode();
    int CountNodes() const;
    int CountLeafs() const;
    int CountPolys() const;
    int CheckRayAgainstPolysNearestHit(const zVEC3& start, const zVEC3& end,
        const zVEC3& direction, zCRayTurbo* turbo) const;
    int RayIntersectionOctree(const zVEC3& start, const zVEC3& end, zVEC3& direction);
    int TraceRay(const zVEC3& start, const zVEC3& end, zCPolygon*& polygon, zVEC3& intersection);
    int TraceRay(const zVEC3& start, const zVEC3& end, zCPolygon*& polygon);
    void CollectPolysInBBox3DRec();
    int  CollectPolysInBBox3D(const zTBBox3D& box, zCPolygon**& polygons, int& count);
    void BuildTree(int level);
    void CalcSubtreeBBox3D();
    void CalcSubtreeBBox3DCell();
    int  OutdoorKillRedundantLeafs();
    void CountDepth(int& minDepth, int& maxDepth, int& avgDepth);
    void Portalize();
    void RemovePoly(zCPolygon* polygon);
    int  DoesNodeContainPoly(zCPolygon* polygon);
    int  CheckPortalConsistency();
    void SplitPortals();
    zCPortal* SplitPortalByAllPortals(zCPortal* portal);
    void EnumeratePortals();
    void ClassifyPortals();
    void SelectPlane(zTPlane& plane, zTPolyNode*& selected, int& valid);
    void SelectPlaneOctree(zCCBspTree* tree, zTPlane& plane, int level);
    int  ClassifyRayToNode(const zVEC3& start, const zVEC3& end);
    int  ClassifyPolyToNode2(zCPolygon* polygon);
    void AddPoly(zCPolygon* polygon);
    void PushPolyFront(zTPolyNode* node);
    void CalcBBox3D();
    void CountDepthRec(int depth, int& minDepth, int& maxDepth, int& sum);
    void DescribeTree(int depth);

    zTPlane plane;
    zCCBspNode* front;
    zCCBspNode* back;
    zCCBspNode* parent;
    zTBBox3D bbox3D;
    zTBBox3D bbox3DCell;
    struct {
        int count;
        zTPolyNode* root;
    } polyList;
};

class zCCBspTree : public zCCBspNode {
public:

    zCCBspTree();
    ~zCCBspTree();
    void SetBspTreeMode(const int& mode);
    void DeleteTree();
    void AddMesh(zCMesh* source);
    void AddMesh(zCMesh* source, const zMAT4& trafo);
    void BuildTree(float optimize);
    zCBspSector* GetBspSectorByName(const zSTRING& name) const;
    void GetPolyNeighbours(zCPolygon* polygon, zCPolygon**& polygons, int& count);
    void MarkGhostOccluder();
    void CreateBspSectors();

    zCMesh* mesh;
    int     bspTreeMode;
    zCArray<zCBspSector*> sectorList;
    int cfg_spanningMode;
    int cfg_useAllPolys;
    int cfg_octree;
    int cfg_bspType;
    float cfg_optimize;
    float cfg_wbalance;
    float cfg_wsplit;
    float cfg_wplanar;
    int cfg_portalize;
};

class zCBspSector {
public:

    struct zTPortalInfo {
        unsigned char visible;
        unsigned char alpha;
    };

    ~zCBspSector();
    static int  CompareOutdoorPortalBBox2D(const void* first, const void* second);
    static void InitHorizonByOutdoorPortals();
    void        CalcCenterByPortals();
    int         HasManyBigPortals() const;
    void        RenderSector();
    void        ActivateSectorRec(const zTBBox2D& portal, zCBspSector* lastSector, int depth);
    void        ActivateSectorRecIndoor(const zTBBox2D& portal, zCBspSector* lastSector, int depth);
    static void RenderActiveSectorList();

    zSTRING sectorName;
    zCArray<zCBspBase*> sectorNodes;
    unsigned long sectorIndex;
    zCArray<zCPolygon*> sectorPortals;
    zCArray<zTPortalInfo> sectorPortalInfo;
    int activated;
    int rendered;
    zTBBox2D activePortal;
    zVEC3 sectorCenter;
    unsigned char hasBigNoFade;

    static int s_sectorsRendered;
    static int s_startPointOutdoor;
    static int s_outdoorPortal;
    static zCArray<zCBspSector*> s_activeSectorList;
    static zCArray<zTBBox2D> s_activeOutdoorPortals;
};

struct zCPluecker {
    zCPluecker() {}
    zCPluecker(const zVEC3& point, const zVEC3& direction);
    void Normalize();

    float values[6];
};

class zCBspLeaf : public zCBspBase {
public:

    zCBspLeaf();
    ~zCBspLeaf();
    void AddVobsToRenderList(zTBBox2D* portal);
    void AddVobsToRenderListOutdoor();
    void AddVobsToRenderListOutdoor(zCBspSector* sector, const zTBBox2D& portal);
    void TraceRayCollectVobs(zCArray<zCVob*>& vobs, const zTBBox3D& box, int flags) const;
    void RenderLeafIndoor(int flags);
    void PolyPlueckerAddRef();
    void PolyPlueckerRelease();

    int lastTimeLighted;
    zCArray<zCVob*> leafVobList;
    zCArray<zCVobLight*> lightVobList;
    int lastTimeActivated;
    unsigned short sectorIndex;
    unsigned short polyPlueckerRefCtr;
    zCPluecker* polyPluecker;
    zVEC3 lightPosition;
};

class zCBspTree {
public:

    zCBspTree();
    ~zCBspTree();
    void InitTree();
    void DisposeTree();
    zCBspBase* BuildRecursive(zCCBspNode* source);
    void Build(zCCBspTree* source);
    void OptimizeLight();
    void Render();
    void RemoveVob(zCVob* vob);
    void AddVob(zCVob* vob);
    void AdjustVob(zCVob* vob);
    int  TraceRay(const zVEC3& start, const zVEC3& end, int flags,
        zVEC3& intersection, zCPolygon*& polygon, zCArray<zCVob*>* vobs) const;
    static void SetRayCache(zCRayCache* cache);
    zTBBox3D CalcLeafRealBBox3D(const zVEC3& position);
    void UpdateVertexDependencies(zCVertex* vertex);
    int  CollectPolysInBBox3DAngle(const zTBBox3D& box, const zVEC3& direction,
        float minAngle, float maxAngle, zCPolygon**& polygons, int& count) const;
    void RenderLightList();
    void  RenderVobList();
    int   FindLeaks(zCArray<zCPolygon*>& leaks);
    void  MarkOccluderPolys();
    float GetOccluderAreaRec(const zCPolygon* polygon, zCArray<zCPolygon*>& visited);
    const zSTRING* GetSectorNameVobIsIn(const zCVob* vob) const;
    zCBspSector* GetBspSectorByName(const zSTRING& name) const;
    void  CreateBspSectors2(zCCBspTree* source);
    void  SaveBIN(zCFileBIN& file);
    int   LoadBIN(zCFileBIN& file, int skip);
    void  ConnectMaterialsToSectors();
    void  CalcStaticLeafLightData();
    void  PreprocessIndoorPortals(zCMesh* sourceMesh, zCArray<zCBuildPortal*>& portals);
    void  GetPolyNeighbours(zCPolygon* polygon, zCPolygon**& polygons, int& count);
    void  PostprocessIndoorPortals(zCArray<zCBuildPortal*>& portals);
    int   CountNodes();

    static unsigned short GetFileVersion();

    static int s_showPortals;
    static int s_renderAllPortals;

    zCBspNode* actNodePtr;
    zCBspLeaf* actLeafPtr;
    zCBspBase* bspRoot;
    zCMesh* mesh;
    zCPolygon** treePolyList;
    zCBspNode* nodeList;
    zCBspLeaf* leafList;
    int numNodes;
    int numLeafs;
    int numPolys;
    zCArray<zCVob*> renderVobList;
    zCArray<zCVobLight*> renderLightList;
    zCArray<zCBspSector*> sectorList;
    zCArray<zCPolygon*> portalList;
    int bspTreeMode;
    int worldRenderMode;
    float vobFarClipZ;
    zTPlane vobFarPlane;
    int vobFarPlaneSignbits;
    int drawVobBBox3D;
    int leafsRendered;
    int vobsRendered;
    int m_bRenderedFirstTime;
    int masterFrameCtr;
    zCPolygon** actPolyPtr;
    int compiled;
};

class zCBuildPortal {
public:

    zCBuildPortal();
    ~zCBuildPortal();

    zCPolygon* portalPoly;
    zCPolygon* portalPolyBack;
    zCArray<zCPolygon*> borderPolyList;
    zCArray<zCPolygon*> polyBucket[2];
    zCArray<zCBuildPortal*> portalBucket[2];
    zCBspSector* sector[2];
};

class zCOBBox3D {
public:

    zCOBBox3D() { childs.next = NULL; childs.data = NULL; }
    ~zCOBBox3D();
    void Draw(int depth, zCOLOR color);
    void BuildOBB(int numPolys, zCPolygon** polygons, int useAABB);
    void BuildRec(int numPolys, zCPolygon** polygons, int depth);
    void BuildOBBTree(zCMesh* mesh, int depth);
    void BuildOBBTree(int numPolys, zCPolygon** polygons, int depth);
    int TestIntersectionOBB(const zCOBBox3D* other) const;
    int TestIntersectionTreeRec(const zCOBBox3D* other) const;
    int TestIntersectionTree(const zCOBBox3D* other, const zMAT4& trafo) const;
    int TraceRay(const zVEC3& start, const zVEC3& direction, zVEC3& intersection) const;
    int CheckOBBPolyIntersection(zCPolygon* polygon, const zMAT4& trafo) const;
    zTBBox3D GetBBox3D() const;
    void SetByBBox3D(const zTBBox3D& box);
    void Transform(const zMAT4& trafo);
    zCOBBox3D& operator=(const zCOBBox3D& other);
    void SaveBIN(zCFileBIN& file);
    void LoadBIN(zCFileBIN& file);

    static zMAT4 trafoB2ToThis;

    zVEC3 center;
    zVEC3 axis[3];
    zVEC3 extent;
    zCList<zCOBBox3D> childs;
};

class zCPortal : public zCPolygon {
public:

    zCPortal(const zTPlane& plane, zCMesh* mesh);
    ~zCPortal();
    void Init();
    void RemoveFromBsp();

    static void operator delete(void* memory);
    static zCMaterial* portalDummyMaterial;
    static zCMemPool<zCPortal> s_MemPool;

    zCBspNode* frontBspNode;
    zCBspNode* backBspNode;
    int        lastTimeCompletely;
};

#endif
