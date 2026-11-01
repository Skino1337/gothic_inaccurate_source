#ifndef ZFILE3D_H
#define ZFILE3D_H

#include "zModel.h"
#include "zFile.h"

class zCFileBIN {
public:

    zCFileBIN();
    ~zCFileBIN();
    void BinCreate(const zSTRING& fileName);
    void BinClose();
    void BinStartChunk(unsigned short id);
    void BinEndChunk();
    int  BinWriteSrcFileStats(zSTRING& fileName);
    void BinWriteString(const zSTRING& text);
    int  BinReadCompareSrcFileStats();
    void BinWriteFloat(float value);
    void BinWriteInt(int value);
    void BinSkipChunk();
    void BinSetFile(zFILE* file);

    int    lastStart;
    int    nextStart;
    int    externalFile;
    zFILE* file;
};

class TPoly3ds {
public:

    void CheckForWrap();

    int vertIndex[3];
    zCMaterial* material;
    int flags;
    int numVerts;
};

struct TVertex3ds {
    zVEC3 position;
    zVEC2 texCoord;
    int frameCtr;
    int mappedIndex;
    int reserved[2];
};

struct zTBinTreeNode {
    zCArray<int> vertList;
    zCArray<int> polyList;
};

class zCMesh3DS {
public:

    ~zCMesh3DS();

    struct zTMatEntry {
        zCMaterial* material;
        zCArray<int> polyList;
    };

    int reserved;
    zSTRING meshName;
    zCArray<int> vertList;
    zCArray<int> polyList;
    zCArray<zTMatEntry*> matList;
};

class zCBinTree3DS {
public:

    zCBinTree3DS();
    ~zCBinTree3DS();
    void GetMatListComplete(zCArray<zCMaterial*>& materials);
    void CalcNumPolyVertSubtree(int nodeIndex, int& numPolys, int& numVerts);
    int CalcNum3DSObject();
    void GetPolyListFor3DSObj(int nodeIndex, zCArray<int>& polys, zCArray<int>& verts);
    void BuildMesh3DS(int objectIndex, int nodeIndex, zCMesh3DS* mesh);
    void CreateNodeList(int numVerts, int numPolys);
    int GetLeafIndex(const zVEC3& position);
    int GetVert3DSIndex(const zVEC3& position, const zVEC2& texCoord);
    void DropMesh(zCMesh* mesh);

    int numVerts;
    int numPolys;
    TVertex3ds* vertices;
    int vertexAlloc;
    int vertexCapacity;
    TPoly3ds* polys;
    int polyCapacity;
    int frameCtr;
    zTBinTreeNode* nodes;
    int nodeAlloc;
    int nodeCount;
    float minX;
    float maxX;
    float splitX;
    zVEC3 center;
};

class zCFile3DS {
public:

    void Load3DS(const zSTRING& fileName, zCMesh* mesh);
    void Save3DS(const zSTRING& fileName, zCMesh* mesh);
};

class zCFileQuakeBSP {
public:

    zCFileQuakeBSP();
    ~zCFileQuakeBSP();
    void LoadQuakeBSP(const zSTRING& fileName, zCMesh* mesh, zCWorld* world);

    zFILE* file;
};

#endif
