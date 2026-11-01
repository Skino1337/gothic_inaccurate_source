#include "include/zFlash.h"

zCFlash::zCBolt::zCBolt()
{
    aniPhase = 0.0f;
}

zCFlash::zCBolt::~zCBolt()
{
    FreeResources();
    for (int i = 0; i < childList.numInArray; ++i)
    {
        childList[i]->FreeResources();
        childList[i]->Release();
    }
    childList.DeleteList();
}

zCFlash::zCFlash()
{
    timeAlive = 0.0f;
    LastUpdateTime = 0.0f;
    step = zVEC3(0.0f, 0.0f, 0.0f);
    recalcStep = 1;
    boltStartPos = zVEC3(0.0f, 0.0f, 0.0f);
    boltEndPos = zVEC3(0.0f, 0.0f, 0.0f);
    boltRoughness = 1.0f;
    textureName = "flash_a0.TGA";
    rootBolt = new zCBolt;
    splitBolt = new zCBolt;
}

zCFlash::~zCFlash()
{
    if (splitBolt)
    {
        splitBolt->Release();
        splitBolt = NULL;
    }
    if (rootBolt)
    {
        rootBolt->Release();
        rootBolt = NULL;
    }
}

int zCFlash::Render(zTRenderContext& context)
{
    timeAlive += ztimer.frameTimeFloat * 0.001f;
    if (ztimer.totalTimeFloat - LastUpdateTime > 70.0f)
    {
        LastUpdateTime = ztimer.totalTimeFloat;
        Generate(rootBolt, boltStartPos, boltEndPos, boltRoughness);
    }
    return rootBolt->Render(context);
}

int zCFlash::IsBBox3DLocal()
{
    return 1;
}

zTBBox3D zCFlash::GetBBox3D()
{
    zTBBox3D result;
    result.mins = zVEC3(-1000.0f, -1000.0f, -1000.0f);
    result.maxs = zVEC3(1000.0f, 1000.0f, 1000.0f);
    return result;
}

zSTRING zCFlash::GetVisualName()
{
    return zSTRING("");
}

int zCFlash::GetVisualDied()
{
    return timeAlive >= 1.0f;
}

void zCFlash::SetVisualUsedBy(zCVob* vob)
{
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
}

void zCFlash::DynLightVisual(const zCArray<zCVobLight*>&, zMAT4*)
{
}

void zCFlash::SetProjectionSphere(const zTBSphere3D& sphere)
{
    rootBolt->SetProjectionSphere(sphere);
}

void zCFlash::zCBolt::SetProjectionSphere(const zTBSphere3D& sphere)
{
    projectionSphere = sphere;
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetProjectionSphere(sphere);
}

void zCFlash::Generate(zCBolt* bolt, const zVEC3& start, const zVEC3& end, float roughness)
{
    bolt->FreeResources();
    boltStartPos = start;
    boltEndPos = end;
    bolt->material->color[3] = 255;
    bolt->isRootBolt = 1;
    bolt->numberOfChilds = 2;
    bolt->roughness = roughness;
    bolt->boltIndex = 0;
    bolt->width = 50.0f;
    bolt->Generate_R(start, end, roughness, 0);
    splitBolt->FreeResources();
    splitBolt->material->color[3] = 255;
    splitBolt->isRootBolt = 1;
    splitBolt->numberOfChilds = 2;
    splitBolt->roughness = roughness;
    splitBolt->boltIndex = 0;
    splitBolt->width = 50.0f;
    zVEC3 splitEnd = end;
    splitEnd.n[0] += float(rand() % 40 - 20);
    splitEnd.n[1] += float(rand() % 40 - 20);
    splitEnd.n[2] += float(rand() % 40 - 20);
    zVEC3 normal(float(rand() % 200 - 100), float(rand() % 200 - 100),
        float(rand() % 200 - 100));
    normal.NormalizeSafe();
    splitBolt->camAlign = zCAM_ALIGN_STRIP;
    splitBolt->GenerateCircle(start, splitEnd, normal);
    splitBolt->GenerateCPLightning0(start, splitEnd, roughness);
}

void zCFlash::zCBolt::Generate_R(const zVEC3& start, const zVEC3& end, float roughness, int depth)
{
    float distance = (end - start).LengthApprox();
    int segments = 2;
    while (segments < distance * 0.008f && segments < 1024)
        segments <<= 1;
    AllocateResources(segments + 1);
    camAlign = zCAM_ALIGN_STRIP;
    zVEC3 normal(float(rand() % 200 - 100), float(rand() % 200 - 100),
        float(rand() % 200 - 100));
    normal.NormalizeSafe();
    GenerateCircle(start, end, normal);
    zVEC3 jitteredEnd = end;
    jitteredEnd.n[0] += float(rand() % 200 - 100);
    jitteredEnd.n[1] += float(rand() % 200 - 100);
    jitteredEnd.n[2] += float(rand() % 200 - 100);
    GenerateCPLightning0(start, jitteredEnd, roughness);
    if (depth >= 2 || numSeg < 2)
        return;
    if (!isRootBolt)
    {
        width = 20.0f / (depth * 3.0f);
        if (width < 3.0f) width = 3.0f;
        material->color[3] = 165;
    }
    for (int i = 0; i < numberOfChilds; ++i)
    {
        int index = isRootBolt ? rand() % numSeg : (i + 1) * numSeg / (numberOfChilds + 1);
        zCBolt* child = new zCBolt;
        child->isRootBolt = 0;
        child->numberOfChilds = numberOfChilds;
        child->roughness = roughness * 0.4f;
        child->boltIndex = i;
        child->Generate_R(centerPointList[index], end, roughness * 0.4f, depth + 1);
        childList.InsertEnd(child);
    }
}

void zCFlash::zCBolt::FreeResources()
{
    zCPolyStrip::FreeResources();
    for (int i = 0; i < childList.numInArray; ++i)
    {
        childList[i]->FreeResources();
        childList[i]->Release();
    }
    childList.DeleteList();
}

int zCFlash::zCBolt::Render(zTRenderContext& context)
{
    zCPolyStrip::Render(context);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->Render(context);
    return 1;
}

void zCFlash::SetTexture(const zSTRING& value)
{
    textureName = value;
    rootBolt->SetTexture(value);
    splitBolt->SetTexture(value);
}

void zCFlash::zCBolt::SetTexture(const zSTRING& value)
{
    material->SetTexture(value);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetTexture(value);
}

void zCFlash::zCBolt::InterpolateTo(zCBolt* other, float amount)
{
    if (other->numPoly != numPoly)
        return;
    for (int i = 0; i < numPoly; ++i)
    {
        zVEC3 delta = centerPointList[i] - other->centerPointList[i];
        float length = sqrt(delta.Length2());
        if (length != 0.0f)
            centerPointList[i] += delta * (amount / length);
    }
}

void zCFlash::zCBolt::Copy(zCBolt* other)
{
    for (int i = 0; i < numPoly; ++i)
        centerPointList[i] = other->centerPointList[i];
}

void zCFlash::Update(float regenerateTime)
{
    timeAlive += ztimer.frameTimeFloat * 0.001f;
    if (ztimer.totalTimeFloat - LastUpdateTime > regenerateTime)
    {
        LastUpdateTime = ztimer.totalTimeFloat;
        Generate(rootBolt, boltStartPos, boltEndPos, boltRoughness);
        rootBolt->SetTexture(textureName);
        splitBolt->SetTexture(textureName);
    }
}

void zCFlash::Create(zVEC3 start, zVEC3 end, float roughness)
{
    boltStartPos = start;
    boltEndPos = end;
    boltRoughness = roughness;
    Generate(rootBolt, start, end, roughness);
    rootBolt->SetTexture(textureName);
    splitBolt->SetTexture(textureName);
}
