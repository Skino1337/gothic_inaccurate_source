#ifndef ZPROGMESH_H
#define ZPROGMESH_H

#include "zVertex.h"

struct zTPMWedge {
    zVEC3 normal;
    zVEC2 texUV;
    unsigned short position;
};

struct zTPMTriangle {
    unsigned short wedge[3];
};

struct zTPMTriangleEdges {
    unsigned short edge[3];
};

struct zTPMEdge {
    unsigned short wedge[2];
};

struct zTPMVertexUpdate {
    unsigned short numNewTri;
    unsigned short numNewWedge;
};

struct zTLazyMesh {
    zTLazyMesh();
    ~zTLazyMesh();

    zCArray<int> wedgePermut;
    zCArray<int> wedgeParent;
    zCArray<unsigned short> triMatIndexList;
    zCArray<zVEC3> posList;
    zCArray<zVEC3> posNormalList;
    zCArray<zCMaterial*> matList;
    zCArray<zTPMTriangle> triList;
    zCArray<zTPMWedge> wedgeList;
    zCArray<float> colorList;
    zCArray<unsigned short> wedgeMap;
    zCArray<zTPMVertexUpdate> vertexUpdates;
    zCArray<unsigned short> triPlaneIndexList;
    zCArray<zTPlane> triPlaneList;
    zCArray<zTPMTriangleEdges> triEdgeList;
    zCArray<zTPMEdge> edgeList;
    zCArray<float> edgeScoreList;
};

class zCProgMeshBuilder {
public:

    void CreatePMTriPosList();
    void CalcPrelight(zTLazyMesh& source, zTLazyMesh& destination);
    void CalcPosNormals(zTLazyMesh& mesh);
    void CalcTriPlaneList(const zTLazyMesh& source, zTLazyMesh& destination);
    void CalcSubdivData(const zTLazyMesh& source, zTLazyMesh& destination);
    void CreateLazySubMesh(int flags, zTLazyMesh& source, zTLazyMesh& destination);
    void CopySubMeshData(unsigned char*& data, const zTLazyMesh& source, zCSubMesh* destination);
    void CreateFinalPMData(int flags);
    void Clear();
    int  BuildProgMeshProto(zCMesh* mesh, zCProgMeshProto* proto, zCArray<int>* wedgeMap,
        int flags);

    static zCProgMeshProto* currentProto;
    static zCArray<zCMaterial*> currentMaterials;
    static zCArray<zTPMTriangle> currentTriList;
    static zCArray<unsigned short> currentTriMatIndexList;
    static zCArray<zTPMWedge*> currentWedgeList;
    static zCArray<zTPMVertexUpdate> currentVertexUpdates;
};

class zCProgMeshConvertFileHandler : public zCScanDirFileHandler {
public:

    zCProgMeshConvertFileHandler();
    virtual ~zCProgMeshConvertFileHandler() {}

    virtual int HandleFile(const zSTRING& path, const char* fileName,
        const zFINDDATA& findData);
};

zVEC3 zLerp(float fraction, const zVEC3& first, const zVEC3& second);

#endif
