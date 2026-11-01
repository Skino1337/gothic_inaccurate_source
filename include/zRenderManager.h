#ifndef ZRENDERMANAGER_H
#define ZRENDERMANAGER_H

#include "z3d.h"

class zCShaderStage {
public:

    zCShaderStage();

    int         shaderFXMode;
    zCTexture*  texture;
    int         alphaFunc;
    unsigned long colorFactor;
    unsigned long avgTextureColor;
    int         alphaGen;
    int         rgbGen;
    int         tcGen;
    int         tcMod;
    float       tcModParams[2];
    int         multiTextureNext;
};

class zCShader {
public:

    zCShader();
    ~zCShader();

    void Clear();
    void InsertStage(int index, zCShaderStage* stage);

    int           numStages;
    zCShaderStage* stageList[4];
    int           hasLightmap;
    int           hasTcGenEnv;
    int           inSector;
    int           baseTexCachedIn;
};

class zCRndAlphaSortObject_WorldPoly {
public:

    zCRndAlphaSortObject_WorldPoly();
    virtual ~zCRndAlphaSortObject_WorldPoly();

    static zCRndAlphaSortObject_WorldPoly* GetFreeAlphaWorldPoly();
    static void ResetFreeList();

    void SetZValueByPoly(zCPolygon* polygon, const zTPlane& plane);
    virtual void Draw(int index);

    zCRndAlphaSortObject_WorldPoly* nextSortObject;
    float zvalue;
    zCPolygon* polygon;
    zCShader* shader;

    static int s_currentAlphaWorldPoly;
    static int s_lastPolyIndex;
    static zCShader* s_lastShader;
    static int s_polysRendered;
    static zCRndAlphaSortObject_WorldPoly s_alphaWorldPolyList[4096];
    static zCArray<zCPolygon*> s_polyList;
};

class zCVertexBufferDyn2 {
public:

    zCVertexBufferDyn2(int format = 0, int numVertices = 0);

    int format;
    int numVertices;
    zCVertexBuffer* vertexBuffer;
};

class zCRenderManager {
public:

    zCRenderManager();
    ~zCRenderManager();

    void InitRenderManager();
    void CleanupRenderManager();
    void AddPolyAlphaSimple(zCPolygon* polygon);
    void FlushSimpleAlphaPolys();
    void Flush();
    void SortMaterialPolys(zCMaterial* material);
    void BuildShader(zCMaterial* material, zCTexture* lightmap,
        unsigned long color, int lightmapMode, int inSector, zCShader*& shader);
    zCVertexBufferDyn2* GetFreeDynVertexBuffer(zCShader* shader);
    void SetRGBA(zCShaderStage* stage);
    void SetStageColorOp(unsigned long stage, int alphaFunc);
    void SetTextureStage0(zCShader* shader, unsigned long stage, zCVertexBuffer* vertexBuffer);
    void SetTextureTransform(zCShader* shader, unsigned long stage, unsigned long rendererStage);
    void SetAlphaFunc(zCShader* shader, unsigned long stage, unsigned long rendererStage);
    void DrawVertexBuffer(zCVertexBuffer* vertexBuffer, int firstVertex, int numVertices,
        unsigned short* indices, unsigned long numIndices, zCMaterial* material, int inSector);
    void DrawVertexBuffer(zCVertexBuffer* vertexBuffer, int firstVertex, int numVertices,
        unsigned short* indices, unsigned long numIndices, zCShader* shader);
    void DrawVertexBufferSingleMaterial(const zCArray<zCPolygon*>& polygons, zCShader* shader);
    void PackVB(const zCArray<zCPolygon*>& polygons, zCShader* shader);

    static zCShader*      GetFreeShader();
    static zCShaderStage* GetFreeShaderStage();

    zCArray<zCPolygon*>  sceneSimpleAlphaPolyList;
    zCArray<zCMaterial*> sceneMaterialList;
    int                  frameCtr;
    int                  rendererMaterialModified;
    unsigned long        overrideAlphaValue;
    int                  overrideAlphaBlendFunc;
    int                  hintLightning;

    static int s_detailTexturesEnabled;
    static int s_useRendererTNL;
    static zCShader s_shaderList[64];
    static zCShaderStage* s_shaderStageList;
    static int s_currentShader;
    static int s_currentShaderStage;
    static zCVertexBufferDyn2* s_vertBufferDynTex1;
    static zCVertexBufferDyn2* s_vertBufferDynTex2;
    static zCVertexBufferDyn2* s_vertBufferDynTex2Normal;
};

struct zTMapDetailTextureEntry {
    zSTRING detailTextureName;
    float   detailTextureScale;
};

struct zTMapDetailTextureMapping {
    zSTRING                 textureName;
    zTMapDetailTextureEntry entry;
};

class zCMapDetailTexture {
public:

    static void S_Init();
    static void S_Cleanup();
    static const zTMapDetailTextureEntry* S_GetDetailTextureInfo(const zSTRING& textureName);

    static zCArray<zTMapDetailTextureMapping> s_mappings;
};

#endif
