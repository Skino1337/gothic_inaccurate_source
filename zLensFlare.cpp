#include "include/zVob.h"
#include "include/zArchiver.h"
#include "include/zResource.h"
#include "include/zTools.h"

static int lensFlareState = -1;
zCClassDef zCLensFlareFX::classDef("zCLensFlareFX", "zCObject", zCLensFlareFX::_CreateNewInstance,
    1, 0x38, 2, "");
zCMesh* zCLensFlareFX::s_lensFlareMesh = NULL;
zCMesh* zCLensFlareFX::s_coronaMesh = NULL;
zCMesh* zCLensFlareFX::s_glowMesh = NULL;

zCLensFlareFX::zTVobLightLensFlare::zTVobLightLensFlare()
    : lensFlareMaterial(NULL), flareType(FT_CORONA), size(0.0f), alpha(0.0f), rangeMin(0.0f),
      rangeMax(0.0f), posScale(0.0f), fadeScale(0.0f)
{
}

zCObject* zCLensFlareFX::_CreateNewInstance()
{
    zCLensFlareFX* flare = new zCLensFlareFX;
    zCClassDef::ObjectCreated(flare, &classDef);
    return flare;
}

void zCLensFlareFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, &classDef);
    ::operator delete(memory);
}

zCClassDef* zCLensFlareFX::_GetClassDef() const
{
    return &classDef;
}

void zCLensFlareFX::ReleaseLensFlareScript()
{
    if (s_lensFlareMesh) {
        s_lensFlareMesh->Release();
        s_lensFlareMesh = NULL;
    }
    if (s_coronaMesh) {
        s_coronaMesh->Release();
        s_coronaMesh = NULL;
    }
    if (s_glowMesh) {
        s_glowMesh->Release();
        s_glowMesh = NULL;
    }
    while (classDef.objectList.numInArray > 0) {
        zCObject* object = classDef.objectList[0];
        if (object)
            object->Release();
    }
}

void zCLensFlareFX::LoadLensFlareScript()
{
    ReleaseLensFlareScript();
    s_lensFlareMesh = zCMesh::CreateQuadMesh(0);
    s_coronaMesh = zCMesh::CreateQuadMesh(0);
    s_glowMesh = zCMesh::CreateQuadMesh(0);
    s_lensFlareMesh->SetStaticLight(0xFFFFFFFF);
    s_lensFlareMesh->ResetLightDynToLightStat();
    s_coronaMesh->SetStaticLight(0xFFFFFFFF);
    s_coronaMesh->ResetLightDynToLightStat();
    s_glowMesh->SetStaticLight(0xFFFFFFFF);
    s_glowMesh->ResetLightDynToLightStat();
    zCArchiver* archiver = zarcFactory->CreateArchiverRead("lensflare.zen", 0);
    if (!archiver)
        return;
    zSTRING chunkName;
    unsigned short version;
    archiver->ReadChunkStart(chunkName, version);
    archiver->SetNoReadSearchCycles(1);
    while (archiver->ReadObject(NULL)) {
    }
    zerr.Message("D: Loading lensflare-script 'lensflare.zen'");
    archiver->Close();
    archiver->Release();
}

zCLensFlareFX::~zCLensFlareFX()
{
    for (int i = 0; i < lensFlareList.numInArray; ++i) {
        if (lensFlareList[i].lensFlareMaterial) {
            lensFlareList[i].lensFlareMaterial->Release();
            lensFlareList[i].lensFlareMaterial = NULL;
        }
    }
}

void zCLensFlareFX::Archive(zCArchiver& archiver)
{
    archiver.WriteString("name", GetObjectName());
    archiver.WriteInt("numFlares", lensFlareList.numInArray);
    for (int i = 0; i < lensFlareList.numInArray; ++i) {
        const zTVobLightLensFlare& flare = lensFlareList[i];
        archiver.WriteChunkStart("flare", 0);
        archiver.WriteString("texture", flare.lensFlareMaterial
            ? flare.lensFlareMaterial->GetObjectName() : zSTR_EMPTY);
        archiver.WriteEnum("type", "FT_CORONA;FT_GLOW;FT_FLARE", flare.flareType);
        archiver.WriteFloat("size", flare.size * (flare.flareType == 1 ? 0.01f : 0.004f));
        archiver.WriteFloat("alpha", flare.alpha);
        archiver.WriteFloat("rangeMin", flare.rangeMin);
        archiver.WriteFloat("rangeMax", flare.rangeMax);
        archiver.WriteFloat("posScale", flare.posScale);
        archiver.WriteFloat("fadeScale", flare.fadeScale);
        archiver.WriteChunkEnd();
    }
    archiver.WriteFloat("fadeScale", fadeScale);
}

void zCLensFlareFX::Unarchive(zCArchiver& archiver)
{
    SetObjectName(archiver.ReadString("name"));
    const int count = archiver.ReadInt("numFlares");
    lensFlareList.Clear();
    int hasCorona = 0;
    int hasGlow = 0;
    for (int i = 0; i < count; ++i) {
        zSTRING chunkName;
        unsigned short version;
        if (!archiver.ReadChunkStart(chunkName, version))
            continue;
        zTVobLightLensFlare flare;
        const zSTRING texture = archiver.ReadString("texture");
        flare.flareType = static_cast<zTVobLightLensFlare::zTFlareType>(archiver.ReadEnum("type"));
        const float size = archiver.ReadFloat("size");
        flare.size = flare.flareType == 1 ? size * 100.0f : 250.0f / size;
        flare.alpha = archiver.ReadFloat("alpha");
        flare.rangeMin = archiver.ReadFloat("rangeMin");
        flare.rangeMax = archiver.ReadFloat("rangeMax");
        flare.posScale = archiver.ReadFloat("posScale");
        flare.fadeScale = archiver.ReadFloat("fadeScale");
        archiver.SkipOpenChunk();
        flare.lensFlareMaterial = new zCMaterial;
        flare.lensFlareMaterial->rndAlphaBlendFunc = 3;
        flare.lensFlareMaterial->SetTexture(texture);
        flare.lensFlareMaterial->color[3] = static_cast<unsigned char>(flare.alpha);
        flare.lensFlareMaterial->color[0] = 255;
        flare.lensFlareMaterial->color[1] = 255;
        flare.lensFlareMaterial->color[2] = 255;
        lensFlareList.InsertEnd(flare);
        if (flare.flareType == 1)
            hasGlow = 1;
        else
            hasCorona = 1;
    }
    archiver.ReadFloat("fadeScale", fadeScale);
    flareMix = !hasCorona ? FM_NO_GLOW : !hasGlow ? FM_GLOW_ONLY : FM_MIXED;
}

void zCLensFlareFX::RenderLensFlares(zCVob* vob)
{
    if (!zCCamera::activeCam)
        return;
    zCCamera::activeCam->Activate();
    const zVEC3 position = vob->trafoObjToWorld.GetTranslation();
    float x;
    float y;
    zCCamera::activeCam->Project(&position, x, y);
    zVEC2 screenPosition(x, y);
    RenderLensFlares(screenPosition);
}

void zCLensFlareFX::RenderLensFlares(zVEC2& screenPosition)
{
    for (int i = 0; i < lensFlareList.numInArray; ++i) {
        zTVobLightLensFlare& flare = lensFlareList[i];
        zCMaterial* material = flare.lensFlareMaterial;
        zCTexture* texture = material->texture;
        if (texture) {
            if ((texture->cacheState & 3) != 3) {
                texture->TouchTimeStampLocal();
                texture->CacheIn(0.5f);
            }
            else
                texture->TouchTimeStamp();
            if ((texture->cacheState & 3) != 3)
                continue;
        }
        zCMesh* mesh = flare.flareType == 0 ? s_coronaMesh
            : flare.flareType == 1 ? s_glowMesh : s_lensFlareMesh;
        zCPolygon* polygon = mesh->polyList[0];
        if (polygon->material != material) {
            if (polygon->material)
                polygon->material->Release();
            material->AddRef();
            polygon->material = material;
        }
        mesh->RenderDecal(screenPosition.n[0], screenPosition.n[1] - 40.0f,
            flare.size, flare.flareType != 0);
    }
}

void zCLensFlareFX::SetAlpha(int alpha)
{
    for (int i = 0; i < lensFlareList.numInArray; ++i)
        lensFlareList[i].lensFlareMaterial->color[3] = alpha;
}
