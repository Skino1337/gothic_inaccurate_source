#include "include/zRenderManager.h"

#include <cstring>

static int rendererState = -1;
static int alphaSortState = -1;
zCRenderManager zrenderMan;
zCRndAlphaSortObject_WorldPoly zCRndAlphaSortObject_WorldPoly::s_alphaWorldPolyList[4096];
zCShader zCRenderManager::s_shaderList[64];
static zCShaderStage renderShaderStageList[256];

void zCShader::Clear()
{
    numStages = 0;
    baseTexCachedIn = 0;
    hasTcGenEnv = 0;
    hasLightmap = 0;
}

zCShader::zCShader()
{
    Clear();
    inSector = 0;
    for (int i = 0; i < 4; ++i)
        stageList[i] = 0;
}

zCShader::~zCShader()
{
}

void zCShader::InsertStage(int index, zCShaderStage* stage)
{
    if (numStages >= 4)
        return;
    for (int i = numStages; i > index; --i)
        stageList[i] = stageList[i - 1];
    stageList[index] = stage;
    ++numStages;
}

zCShaderStage::zCShaderStage()
    : shaderFXMode(0), texture(0), alphaFunc(1), colorFactor(0), avgTextureColor(0),
      alphaGen(0), rgbGen(0), tcGen(0), tcMod(0), multiTextureNext(0)
{
    tcModParams[0] = 0;
    tcModParams[1] = 0;
}

static zCRndAlphaSortObject_WorldPoly alphaWorldPolyList[0x1000];

int zCRndAlphaSortObject_WorldPoly::s_currentAlphaWorldPoly;
int zCRndAlphaSortObject_WorldPoly::s_lastPolyIndex;
zCShader* zCRndAlphaSortObject_WorldPoly::s_lastShader;
int zCRndAlphaSortObject_WorldPoly::s_polysRendered;
zCArray<zCPolygon*> zCRndAlphaSortObject_WorldPoly::s_polyList;

zCRndAlphaSortObject_WorldPoly::zCRndAlphaSortObject_WorldPoly()
    : nextSortObject(0), zvalue(0)
{
}

zCRndAlphaSortObject_WorldPoly::~zCRndAlphaSortObject_WorldPoly()
{
}

zCRndAlphaSortObject_WorldPoly* zCRndAlphaSortObject_WorldPoly::GetFreeAlphaWorldPoly()
{
    if (s_currentAlphaWorldPoly >= 0x1000)
        s_currentAlphaWorldPoly = 0;
    return &alphaWorldPolyList[s_currentAlphaWorldPoly++];
}

void zCRndAlphaSortObject_WorldPoly::ResetFreeList()
{
    s_currentAlphaWorldPoly = 0;
    s_lastPolyIndex = 0;
    s_lastShader = 0;
    s_polysRendered = 0;
    s_polyList.numInArray = 0;
}

void zCRndAlphaSortObject_WorldPoly::SetZValueByPoly(zCPolygon* polygonValue,
    const zTPlane& plane)
{
    polygon = polygonValue;
    float depth = 0;
    for (int i = 0; i < polygon->polyNumVert; ++i) {
        const zVEC3& position = polygon->vertex[i]->position;
        depth += position.n[0] * plane.normal.n[0] + position.n[1] * plane.normal.n[1] +
            position.n[2] * plane.normal.n[2] - plane.distance;
    }
    zvalue = depth / polygon->polyNumVert + 4000.0f;
}

void zCRndAlphaSortObject_WorldPoly::Draw(int index)
{
    if ((shader != s_lastShader || index != s_lastPolyIndex + 1) && s_polysRendered)
        s_polyList.numInArray = 0;
    s_polyList.InsertEnd(polygon);
    s_lastPolyIndex = index;
    s_lastShader = shader;
    ++s_polysRendered;
    if (s_polysRendered >= s_currentAlphaWorldPoly)
        s_polyList.numInArray = 0;
}

// EXE: 0x00982F88
zCShaderStage* zCRenderManager::s_shaderStageList = renderShaderStageList;
// EXE: 0x0099AA84
int zCRenderManager::s_currentShader;
// EXE: 0x0099AA88
int zCRenderManager::s_currentShaderStage;
// EXE: 0x008A5A00
int zCRenderManager::s_useRendererTNL = 1;
// EXE: 0x008A5A04
int zCRenderManager::s_detailTexturesEnabled = 1;
zCVertexBufferDyn2* zCRenderManager::s_vertBufferDynTex1;
zCVertexBufferDyn2* zCRenderManager::s_vertBufferDynTex2;
zCVertexBufferDyn2* zCRenderManager::s_vertBufferDynTex2Normal;

zCRenderManager::zCRenderManager()
    : frameCtr(0), rendererMaterialModified(0), overrideAlphaValue(255),
      overrideAlphaBlendFunc(1), hintLightning(0)
{
}

zCRenderManager::~zCRenderManager()
{
    CleanupRenderManager();
}

void zCRenderManager::InitRenderManager()
{
    frameCtr = 0;
    rendererMaterialModified = 0;
    overrideAlphaValue = 255;
    overrideAlphaBlendFunc = 1;
    s_detailTexturesEnabled = 1;
}

void zCRenderManager::CleanupRenderManager()
{
    delete s_vertBufferDynTex1;
    s_vertBufferDynTex1 = 0;
    delete s_vertBufferDynTex2;
    s_vertBufferDynTex2 = 0;
    delete s_vertBufferDynTex2Normal;
    s_vertBufferDynTex2Normal = 0;
}

// EXE: 0x005D7650
zCShader* zCRenderManager::GetFreeShader()
{
    if (s_currentShader >= 0x40)
        s_currentShader = 0;
    return &s_shaderList[s_currentShader++];
}

// EXE: 0x005D7740
zCShaderStage* zCRenderManager::GetFreeShaderStage()
{
    if (s_currentShaderStage >= 0x100)
        s_currentShaderStage = 0;
    return &s_shaderStageList[s_currentShaderStage++];
}

void zCRenderManager::AddPolyAlphaSimple(zCPolygon* polygon)
{
    sceneSimpleAlphaPolyList.InsertEnd(polygon);
}

void zCRenderManager::FlushSimpleAlphaPolys()
{
    sceneSimpleAlphaPolyList.numInArray = 0;
}

// EXE: 0x005D79F0
void zCRenderManager::Flush()
{
    ++frameCtr;
    s_currentShader = 0;
    s_currentShaderStage = 0;
    zCRndAlphaSortObject_WorldPoly::ResetFreeList();
    FlushSimpleAlphaPolys();
    for (int i = 0; i < sceneMaterialList.numInArray; ++i)
        sceneMaterialList.parray[i]->polyList.numInArray = 0;
    sceneMaterialList.numInArray = 0;
}

void zCRenderManager::SortMaterialPolys(zCMaterial* material)
{
    if (material->smooth >= 0)
        return;
    for (int i = 0; i < material->polyList.numInArray; ++i) {
        for (int j = i + 1; j < material->polyList.numInArray; ++j) {
            if (material->polyList.parray[j]->lightmap < material->polyList.parray[i]->lightmap) {
                zCPolygon* polygon = material->polyList.parray[i];
                material->polyList.parray[i] = material->polyList.parray[j];
                material->polyList.parray[j] = polygon;
            }
        }
    }
}

void zCRenderManager::BuildShader(zCMaterial* material, zCTexture* lightmap,
    unsigned long color, int lightmapMode, int sector, zCShader*& shader)
{
    shader = GetFreeShader();
    shader->Clear();
    shader->hasLightmap = lightmapMode;
    shader->inSector = sector;
    shader->baseTexCachedIn = material->texture != 0;
    zCShaderStage* stage = GetFreeShaderStage();
    stage->texture = material->texture;
    stage->alphaFunc = material->rndAlphaBlendFunc;
    stage->colorFactor = color;
    stage->shaderFXMode = 1;
    shader->InsertStage(0, stage);
    if (lightmap) {
        zCShaderStage* lightStage = GetFreeShaderStage();
        lightStage->texture = lightmap;
        lightStage->shaderFXMode = 2;
        lightStage->colorFactor = color;
        shader->InsertStage(1, lightStage);
        shader->hasLightmap = 1;
    }
}

zCVertexBufferDyn2::zCVertexBufferDyn2(int formatValue, int numVerticesValue)
    : format(formatValue), numVertices(numVerticesValue), vertexBuffer(new zCVertexBuffer)
{
    std::memset(vertexBuffer, 0, sizeof(zCVertexBuffer));
}

zCVertexBufferDyn2* zCRenderManager::GetFreeDynVertexBuffer(zCShader* shader)
{
    if (!s_vertBufferDynTex1) {
        s_vertBufferDynTex1 = new zCVertexBufferDyn2(0x19, 0x800);
        s_vertBufferDynTex2 = new zCVertexBufferDyn2(0x29, 0x800);
        s_vertBufferDynTex2Normal = new zCVertexBufferDyn2(0x2d, 0x800);
    }
    if (shader->hasTcGenEnv)
        return s_vertBufferDynTex2Normal;
    return shader->hasLightmap ? s_vertBufferDynTex2 : s_vertBufferDynTex1;
}

void zCRenderManager::SetRGBA(zCShaderStage* stage)
{
    if (hintLightning)
        stage->shaderFXMode = 10;
    else if (stage->tcGen == 2)
        stage->shaderFXMode = 6;
    else if (stage->alphaGen == 5)
        stage->shaderFXMode = 11;
    else if (stage->rgbGen == 2)
        stage->shaderFXMode = 3;
    else
        stage->shaderFXMode = stage->rgbGen != 0;
}

void zCRenderManager::SetStageColorOp(unsigned long stageIndex, int alphaFunc)
{
    zCShaderStage* stage = &s_shaderStageList[stageIndex];
    if (alphaFunc == 2)
        stage->shaderFXMode = 11;
    else if (alphaFunc == 3)
        stage->shaderFXMode = 6;
    else if (alphaFunc == 5)
        stage->shaderFXMode = 3;
    else if (alphaFunc == 6)
        stage->shaderFXMode = 4;
}

void zCRenderManager::SetTextureStage0(zCShader* shader, unsigned long stageIndex,
    zCVertexBuffer*)
{
    rendererMaterialModified = !shader->baseTexCachedIn &&
        !shader->stageList[stageIndex]->texture;
}

void zCRenderManager::SetTextureTransform(zCShader* shader, unsigned long stageIndex,
    unsigned long rendererStage)
{
    shader->stageList[stageIndex]->multiTextureNext = rendererStage;
}

void zCRenderManager::SetAlphaFunc(zCShader* shader, unsigned long stageIndex,
    unsigned long rendererStage)
{
    zCShaderStage* stage = shader->stageList[stageIndex];
    stage->multiTextureNext = rendererStage;
    if (!stage->texture && stage->alphaGen == 5)
        stage->alphaFunc = 3;
    else if (stage->alphaGen == 0)
        stage->alphaFunc = 1;
}

void zCRenderManager::DrawVertexBuffer(zCVertexBuffer* vertexBuffer, int firstVertex,
    int numVertices, unsigned short* indices, unsigned long numIndices,
    zCMaterial* material, int inSector)
{
    zCShader* shader;
    BuildShader(material, 0, 0, 1, inSector, shader);
    DrawVertexBuffer(vertexBuffer, firstVertex, numVertices, indices, numIndices, shader);
}

void zCRenderManager::DrawVertexBuffer(zCVertexBuffer* vertexBuffer, int,
    int, unsigned short*, unsigned long, zCShader* shader)
{
    hintLightning = hintLightning && !shader->inSector;
    for (int i = 0; i < shader->numStages; ++i) {
        SetTextureStage0(shader, i, vertexBuffer);
        SetTextureTransform(shader, i, i);
        SetAlphaFunc(shader, i, i);
        SetRGBA(shader->stageList[i]);
    }
}

void zCRenderManager::DrawVertexBufferSingleMaterial(const zCArray<zCPolygon*>& polygons,
    zCShader* shader)
{
    PackVB(polygons, shader);
}

void zCRenderManager::PackVB(const zCArray<zCPolygon*>& polygons, zCShader* shader)
{
    if (!polygons.numInArray)
        return;
    int numVertices = 0;
    int numIndices = 0;
    for (int i = 0; i < polygons.numInArray; ++i) {
        numVertices += polygons.parray[i]->polyNumVert;
        numIndices += (polygons.parray[i]->polyNumVert - 2) * 3;
    }
    zCVertexBufferDyn2* dynamic = GetFreeDynVertexBuffer(shader);
    zCVertexBuffer* buffer = dynamic->vertexBuffer;
    buffer->positionList = new zVEC3[numVertices];
    buffer->normalList = new zVEC3[numVertices];
    buffer->texCoordList = new zVEC2[numVertices];
    unsigned short* indices = new unsigned short[numIndices];
    int vertexIndex = 0;
    int indexIndex = 0;
    for (int i = 0; i < polygons.numInArray; ++i) {
        const zCPolygon& polygon = *polygons.parray[i];
        const int firstVertex = vertexIndex;
        for (int j = 0; j < polygon.polyNumVert; ++j) {
            buffer->positionList[vertexIndex] = polygon.vertex[j]->position;
            buffer->normalList[vertexIndex] = polygon.feature[j]->vertNormal;
            buffer->texCoordList[vertexIndex].n[0] = polygon.feature[j]->mapping.n[0];
            buffer->texCoordList[vertexIndex].n[1] = polygon.feature[j]->mapping.n[1];
            ++vertexIndex;
        }
        for (int j = 1; j < polygon.polyNumVert - 1; ++j) {
            indices[indexIndex++] = firstVertex;
            indices[indexIndex++] = firstVertex + j;
            indices[indexIndex++] = firstVertex + j + 1;
        }
    }
    buffer->arrayStride = numVertices;
    dynamic->numVertices = numVertices;
    DrawVertexBuffer(buffer, 0, numVertices, indices, numIndices, shader);
    delete[] indices;
}

zCArray<zTMapDetailTextureMapping> zCMapDetailTexture::s_mappings;

void zCMapDetailTexture::S_Init()
{
    s_mappings.DeleteList();
    zFILE_FILE file("detailTexturePresets.txt");
    if (!file.Exists())
        return;
    file.Open(0);
    while (!file.Eof()) {
        zSTRING line;
        file.Read(line);
        if (!line.Length())
            continue;
        zTMapDetailTextureMapping mapping;
        mapping.textureName = line.PickWord(1, " ", "\"");
        mapping.textureName.Upper();
        mapping.entry.detailTextureName = line.PickWord(2, " ", "\"");
        mapping.entry.detailTextureScale = line.PickWord(3, " ", "\"").ToFloat();
        s_mappings.InsertEnd(mapping);
    }
    file.Close();
}

void zCMapDetailTexture::S_Cleanup()
{
}

const zTMapDetailTextureEntry* zCMapDetailTexture::S_GetDetailTextureInfo(
    const zSTRING& textureName)
{
    zSTRING upperName = textureName;
    upperName.Upper();
    for (int i = 0; i < s_mappings.numInArray; ++i) {
        if (s_mappings.parray[i].textureName == upperName)
            return &s_mappings.parray[i].entry;
    }
    return 0;
}
