#include "include/oBarrier.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

static int decalState = -1;
static int decalMeshState = -1;
static zCClassDef zCDecalClassDef("zCDecal", "zCVisual", zCDecal::_CreateNewInstance,
    0, 0x54, 0, "");
zCClassDef* zCDecal::classDef = &zCDecalClassDef;
zCMesh* zCDecal::decalMesh1Sided = 0;
zCMesh* zCDecal::decalMesh2Sided = 0;

zCObject* zCDecal::_CreateNewInstance()
{
    zCDecal* decal = new zCDecal;
    zCClassDef::ObjectCreated(decal, classDef);
    return decal;
}

void zCDecal::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCDecal::_GetClassDef() const
{
    return classDef;
}

int zCDecal::IsBBox3DLocal()
{
    return 1;
}

zCDecal::zCDecal()
    : decalMaterial(new zCMaterial), xdim(25.0f), ydim(25.0f), xoffset(0.0f),
      yoffset(0.0f), decal2Sided(0), ignoreDayLight(0), m_bOnTop(0)
{
}

zCDecal::~zCDecal()
{
    if (decalMaterial) {
        decalMaterial->Release();
        decalMaterial = 0;
    }
}

void zCDecal::CleanupDecals()
{
    decalMesh2Sided->polyList[0]->material = 0;
    decalMesh2Sided->polyList[1]->material = 0;
    decalMesh1Sided->polyList[0]->material = 0;
    if (decalMesh1Sided) {
        decalMesh1Sided->Release();
        decalMesh1Sided = 0;
    }
    if (decalMesh2Sided) {
        decalMesh2Sided->Release();
        decalMesh2Sided = 0;
    }
}

void zCDecal::CreateDecalMeshes()
{
    decalMesh1Sided = zCMesh::CreateQuadMesh(0);
    decalMesh2Sided = zCMesh::CreateQuadMesh(1);
    decalMesh1Sided->SetStaticLight(0xFFFFFFFF);
    decalMesh1Sided->SetLightDyn(0xFFFFFFFF);
    decalMesh2Sided->SetStaticLight(0xFFFFFFFF);
    decalMesh2Sided->SetLightDyn(0xFFFFFFFF);
}

const zSTRING* zCDecal::GetFileExtension(int index)
{
    static const zSTRING extension(".TGA");
    return index == 0 ? &extension : 0;
}

zCVisual* zCDecal::LoadVisualVirtual(const zSTRING& fileName) const
{
    zCDecal* decal = new zCDecal;
    decal->decalMaterial->SetTexture(fileName);
    return decal;
}

unsigned long zCDecal::GetRenderSortKey() const
{
    return reinterpret_cast<unsigned long>(decalMaterial->texture);
}

void zCDecal::SetTexture(const zSTRING& fileName)
{
    decalMaterial->SetTexture(fileName);
}

void zCDecal::SetTexture(zCTexture* texture)
{
    decalMaterial->SetTexture(texture);
}

void zCDecal::SetDecalDim(float width, float height)
{
    xdim = width;
    ydim = height;
}

int zCDecal::Render(zTRenderContext& context)
{
    zCTexture* texture = decalMaterial->texture;
    if (texture) {
        if ((texture->cacheState & 3) != 3) {
            texture->TouchTimeStampLocal();
            texture->CacheIn(0.5f);
        }
        else
            texture->TouchTimeStamp();
        if ((texture->cacheState & 3) != 3) {
            if (texture->actAniFrame[0] > 0)
                texture->PrecacheTexAniFrames(0.2f);
            return 0;
        }
    }

    zCMesh* mesh = decal2Sided ? decalMesh2Sided : decalMesh1Sided;
    mesh->polyList[0]->material = decalMaterial;
    if (decal2Sided)
        mesh->polyList[1]->material = decalMaterial;
    mesh->vertList[0]->position = zVEC3(xoffset - xdim, yoffset + ydim, 0.0f);
    mesh->vertList[1]->position = zVEC3(xoffset + xdim, yoffset + ydim, 0.0f);
    mesh->vertList[2]->position = zVEC3(xoffset + xdim, yoffset - ydim, 0.0f);
    mesh->vertList[3]->position = zVEC3(xoffset - xdim, yoffset - ydim, 0.0f);
    if (context.clipFlags == 2) {
        mesh->RenderDecal(context, m_bOnTop);
        return 1;
    }
    if (ignoreDayLight) {
        context.hintLightingFullbright = 1;
        context.hintLightingHighlightColor = zCOLOR(255, 255, 255, 255);
    }
    return mesh->Render(context);
}

void zCDecal::SetVisualUsedBy(zCVob* vob)
{
    vob->showVisual ^= 0x20;
}

zTBBox3D zCDecal::GetBBox3D()
{
    zTBBox3D box;
    box.mins = zVEC3(xoffset - xdim, yoffset - ydim, 0.0f);
    box.maxs = zVEC3(xoffset + xdim, yoffset + ydim, 0.0f);
    return box;
}

zSTRING zCDecal::GetVisualName()
{
    return decalMaterial->texture ? decalMaterial->texture->GetObjectName() : zSTR_EMPTY;
}

int zCDecal::GetAlphaTestingEnabled() const
{
    if (!decalMaterial)
        return 0;
    const int blendFunc = decalMaterial->rndAlphaBlendFunc;
    if (blendFunc >= 3 && blendFunc <= 6)
        return 0;
    if (blendFunc == 7)
        return 1;
    zCTexture* texture = decalMaterial->texture;
    if (!texture)
        return 0;
    if ((texture->cacheState & 3) != 3) {
        texture->TouchTimeStampLocal();
        texture->CacheIn(-1.0f);
    }
    else
        texture->TouchTimeStamp();
    return texture->HasAlpha();
}

void zCDecal::DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo)
{
    zCOLOR color(200, 200, 200, 255);
    if (lights.numInArray) {
        const zVEC3 position = trafo ? trafo->GetTranslation() : zVEC3(0.0f);
        color = zCVobLight::SumLightsAtPositionWS(lights, position, 0);
    }
    zCMesh* mesh = decal2Sided ? decalMesh2Sided : decalMesh1Sided;
    for (int i = 0; i < mesh->numPoly; ++i) {
        for (int j = 0; j < 4; ++j) {
            mesh->polyList[i]->feature[j]->lightDyn[0] = color.b;
            mesh->polyList[i]->feature[j]->lightDyn[1] = color.g;
            mesh->polyList[i]->feature[j]->lightDyn[2] = color.r;
            mesh->polyList[i]->feature[j]->lightDyn[3] = color.a;
        }
    }
}

void zCDecal::Archive(zCArchiver& archiver)
{
    archiver.WriteString("", decalMaterial->texture
        ? decalMaterial->texture->GetObjectName() : zSTR_EMPTY);
    float dimensions[2] = { xdim, ydim };
    float offsets[2] = { xoffset, yoffset };
    archiver.WriteRawFloat("", dimensions, sizeof(dimensions));
    archiver.WriteRawFloat("", offsets, sizeof(offsets));
    archiver.WriteBool("", decal2Sided);
    archiver.WriteEnum("", "MAT_DEFAULT;NONE;BLEND;ADD;SUB;MUL;MUL2",
        decalMaterial->rndAlphaBlendFunc);
    archiver.WriteFloat("", decalMaterial->texAniCtrl.aniFPS * 1000.0f);
    archiver.WriteByte("", decalMaterial->color[3]);
    archiver.WriteBool("", ignoreDayLight);
}

void zCDecal::Unarchive(zCArchiver& archiver)
{
    zSTRING textureName;
    archiver.ReadString("", textureName);
    SetTexture(textureName);
    float dimensions[2];
    float offsets[2];
    archiver.ReadRawFloat("", dimensions, sizeof(dimensions));
    archiver.ReadRawFloat("", offsets, sizeof(offsets));
    xdim = dimensions[0];
    ydim = dimensions[1];
    xoffset = offsets[0];
    yoffset = offsets[1];
    decal2Sided = archiver.ReadBool("");
    decalMaterial->rndAlphaBlendFunc = archiver.ReadEnum("");
    if (decalMaterial->rndAlphaBlendFunc == 4)
        decalMaterial->rndAlphaBlendFunc = 2;
    decalMaterial->texAniCtrl.aniFPS = archiver.ReadFloat("") * 0.001f;
    decalMaterial->color[3] = archiver.ReadByte("");
    ignoreDayLight = archiver.ReadBool("");
}

myThunder::myThunder()
{
}

myThunder::~myThunder()
{
    if (polyStrip) {
        polyStrip->Release();
        polyStrip = 0;
    }
}

oCBarrier::oCBarrier()
    : skySphereMesh(0), myPolyList(0), myVertList(0), myThunderList(0), originalTexUVList(0),
      thunderStartDecal(0), bFadeInOut(0), fadeState(0), fadeIn(1), fadeOut(0), bFlashing(0),
      fFlashTime(0.0f), sfx1(0), sfx2(0), sfx3(0), sfx4(0), sfxHandle1(0), sfxHandle2(0),
      sfxHandle3(0), sfxHandle4(0), activeThunder_Sector1(0), activeThunder_Sector2(0),
      activeThunder_Sector3(0), activeThunder_Sector4(0)
{
    thunderStartDecal = new zCDecal;
    zCClassDef::ObjectCreated(thunderStartDecal, zCDecal::classDef);
    thunderStartDecal->SetTexture("unsun5.tga");
    thunderStartDecal->SetDecalDim(4000.0f, 4000.0f);
    thunderStartDecal->xoffset = 0.0f;
    thunderStartDecal->yoffset = 0.0f;
    thunderStartDecal->decalMaterial->color[3] = 100;
    thunderStartDecal->decalMaterial->rndAlphaBlendFunc = 3;
    srand((unsigned int)time(0));
}

oCBarrier::~oCBarrier()
{
    if (skySphereMesh) {
        skySphereMesh->Release();
        skySphereMesh = 0;
    }
    delete[] myVertList;
    myVertList = 0;
    delete[] myPolyList;
    myPolyList = 0;
    if (myThunderList) {
        for (int i = 0; i < numMaxThunders; ++i) {
            myThunder& thunder = myThunderList[i];
            if (thunder.polyStrip) {
                thunder.polyStrip->Release();
                thunder.polyStrip = 0;
            }
            for (int j = 0; j < thunder.numChilds; ++j)
                RemoveThunder(&thunder.childs[j]);
            delete[] thunder.childs;
            thunder.childs = 0;
            InitThunder(&thunder);
        }
        delete[] myThunderList;
        myThunderList = 0;
        numMyThunders = 0;
    }
    if (thunderStartDecal) {
        thunderStartDecal->Release();
        thunderStartDecal = 0;
    }
    zCSoundFX** sounds[] = { &sfx1, &sfx2, &sfx3, &sfx4 };
    int* handles[] = { &sfxHandle1, &sfxHandle2, &sfxHandle3, &sfxHandle4 };
    for (int i = 0; i < 4; ++i) {
        if (*sounds[i]) {
            zsound->StopSound(*handles[i]);
            (*sounds[i])->Release();
            *sounds[i] = 0;
        }
    }
    delete[] originalTexUVList;
    originalTexUVList = 0;
}

void oCBarrier::Init()
{
    originalTexUVList = 0;
    skySphereMesh = zCMesh::Load("magicfrontier_out.3ds", 1);
    if (!skySphereMesh)
        return;

    originalTexUVList = new zVEC2[skySphereMesh->numPoly * 3];
    int texIndex = 0;
    for (int i = 0; i < skySphereMesh->numPoly; ++i) {
        zCPolygon* poly = skySphereMesh->polyList[i];
        for (int j = 0; j < 3; ++j)
            originalTexUVList[texIndex++] = *poly->texVertex[j];
    }

    numMyVerts = skySphereMesh->numVerts;
    numMyPolys = skySphereMesh->numPoly;
    myVertList = new myVert[numMyVerts];
    memset(myVertList, 0, sizeof(myVert) * numMyVerts);
    for (int i = 0; i < numMyVerts; ++i) {
        myVertList[i].vertIndex = i;
        for (int j = 0; j < numMyPolys; ++j) {
            zCPolygon* poly = skySphereMesh->polyList[j];
            int ownVertex = -1;
            for (int k = 0; k < 3; ++k) {
                if (poly->vertexIndex[k] == i)
                    ownVertex = k;
            }
            if (ownVertex == -1)
                continue;
            myVertList[i].polyIndices[myVertList[i].numPolyIndices++] = j;
            for (int k = 0; k < 3; ++k) {
                int neighbour = poly->vertexIndex[k];
                if (neighbour == i)
                    continue;
                int known = 0;
                for (int n = 0; n < myVertList[i].numNeighbours; ++n) {
                    if (myVertList[i].vertNeighbours[n] == neighbour)
                        known = 1;
                }
                if (!known)
                    myVertList[i].vertNeighbours[myVertList[i].numNeighbours++] = neighbour;
            }
        }
    }

    numStartPoints1 = 0;
    numStartPoints2 = 0;
    numStartPoints3 = 0;
    numStartPoints4 = 0;
    topestPoint = 0;
    for (int i = 0; i < numMyVerts; ++i) {
        const zVEC3& position = skySphereMesh->vertexList[i];
        if (position.n[1] < 35000.0f)
            continue;
        if (position.n[0] >= -5008.4f && position.n[2] <= -5597.02f)
            startPointList1[numStartPoints1++] = i;
        if (position.n[0] <= -5008.4f && position.n[2] <= -5597.02f)
            startPointList2[numStartPoints2++] = i;
        if (position.n[0] <= -5008.4f && position.n[2] >= -5597.02f)
            startPointList3[numStartPoints3++] = i;
        if (position.n[0] >= -5008.4f && position.n[2] >= -5597.02f)
            startPointList4[numStartPoints4++] = i;
        if (position.n[0] == -5008.4f && position.n[2] == -5597.02f)
            topestPoint = i;
    }
    Initialise(20);
}

void oCBarrier::Initialise(int maxThunders)
{
    delete[] myThunderList;
    myThunderList = new myThunder[maxThunders];
    numMaxThunders = maxThunders;
    numMyThunders = 0;
    for (int i = 0; i < numMaxThunders; ++i) {
        myThunder& thunder = myThunderList[i];
        thunder.polyStrip = 0;
        thunder.numSegs = 0;
        thunder.valid = 0;
        thunder.t0 = 0.0f;
        thunder.t1 = 0.0f;
        thunder.numSplits = 0;
        thunder.dead = 0;
        thunder.isChild = 0;
        thunder.numChilds = 0;
        thunder.childs = 0;
    }
}

void oCBarrier::AddTremor(zTRenderContext& context)
{
}

void oCBarrier::RenderLayer(zTRenderContext& context, int layer, int& alpha)
{
    float strength = layer > 0 ? 1.5f : 1.0f;
    float time = ztimer.totalTimeFloat * 0.001f;
    float scroll = ztimer.totalTimeFloat * 0.00015f;
    scroll -= floor(scroll);
    int texIndex = 0;
    for (int i = 0; i < skySphereMesh->numPoly; ++i) {
        zCPolygon* poly = skySphereMesh->polyList[i];
        for (int j = 0; j < 3; ++j) {
            zVEC2* uv = poly->texVertex[j];
            uv->n[0] = originalTexUVList[texIndex].n[0] + scroll +
                sin((i + j) * 0.1f + time) * strength * 0.15f;
            uv->n[1] = originalTexUVList[texIndex].n[1] +
                sin((i - j) * 0.1f + time) * strength * 0.15f;
            ++texIndex;
        }
    }
    skySphereMesh->Render(context);
}

static void TriggerBarrierThunder(oCBarrier* barrier, int sector)
{
    int* active[] = {
        &barrier->activeThunder_Sector1, &barrier->activeThunder_Sector2,
        &barrier->activeThunder_Sector3, &barrier->activeThunder_Sector4
    };
    int* counts[] = {
        &barrier->numStartPoints1, &barrier->numStartPoints2,
        &barrier->numStartPoints3, &barrier->numStartPoints4
    };
    int* lists[] = {
        barrier->startPointList1, barrier->startPointList2,
        barrier->startPointList3, barrier->startPointList4
    };
    zCSoundFX** sounds[] = { &barrier->sfx1, &barrier->sfx2, &barrier->sfx3, &barrier->sfx4 };
    int* handles[] = {
        &barrier->sfxHandle1, &barrier->sfxHandle2, &barrier->sfxHandle3, &barrier->sfxHandle4
    };
    static float lastTimes[4];
    static const float delays[] = { 16000.0f, 12000.0f, 28000.0f, 4000.0f };
    int index = sector - 1;
    if (*active[index] || zsound->IsSoundActive(*handles[index]) || !*counts[index])
        return;
    if (ztimer.totalTimeFloat - lastTimes[index] <= delays[index])
        return;
    if (rand() * (1.0 / 32767.0) <= 0.999)
        return;

    int start = lists[index][rand() & (*counts[index] - 1)];
    barrier->AddThunder(start, 11, 0.0f, (zTThunderSector)sector);
    *active[index] = 1;
    lastTimes[index] = ztimer.totalTimeFloat;
    if (!*sounds[index])
        *sounds[index] = zsound->LoadSoundFXScript("MFX_Barriere_Shoot");
    *handles[index] = zsound->PlaySound3D(*sounds[index], zCCamera::activeCam->connectedVob, 0, 0);
}

int oCBarrier::Render(zTRenderContext& context, int alpha, int tremor)
{
    if (skySphereMesh) {
        int zBufferWrite = zrenderer->GetZBufferWriteEnabled();
        float farClipZ = zCCamera::activeCam->farClipZ;
        zrenderer->SetZBufferWriteEnabled(0);
        zCCamera::activeCam->SetFarClipZ(2000000.0f);
        RenderLayer(context, 0, alpha);
        if (tremor)
            AddTremor(context);
        for (int sector = 1; sector <= 4; ++sector)
            TriggerBarrierThunder(this, sector);
        RenderThunderList(context);
        zrenderer->FlushAlphaPolys();
        int* handles[] = { &sfxHandle1, &sfxHandle2, &sfxHandle3, &sfxHandle4 };
        zCSoundFX** sounds[] = { &sfx1, &sfx2, &sfx3, &sfx4 };
        for (int i = 0; i < 4; ++i) {
            if (*sounds[i])
                zsound->UpdateSound3D(*handles[i], 0);
        }
        zrenderMan.hintLightning = bFlashing;
        if (bFlashing) {
            fFlashTime -= (float)ztimer.frameTime;
            if (fFlashTime < 100.0f && rand() * (1.0 / 32767.0) <= 0.2)
                zrenderMan.hintLightning = 0;
            if (fFlashTime <= 0.0f)
                bFlashing = 0;
        }
        zrenderer->SetZBufferWriteEnabled(zBufferWrite);
        zCCamera::activeCam->SetFarClipZ(farClipZ);
    }
    return bFadeInOut;
}

void oCBarrier::InitThunder(myThunder* thunder)
{
    thunder->polyStrip = 0;
    thunder->valid = 0;
    thunder->t0 = 0.0f;
    thunder->t1 = 0.0f;
    thunder->numSplits = 0;
    thunder->dead = 0;
    thunder->isChild = 0;
    thunder->numChilds = 0;
    thunder->childs = 0;
}

void oCBarrier::RemoveThunder(myThunder* thunder)
{
    if (thunder->polyStrip) {
        thunder->polyStrip->Release();
        thunder->polyStrip = 0;
    }
    if (thunder->childs) {
        for (int i = 0; i < thunder->numChilds; ++i)
            RemoveThunder(&thunder->childs[i]);
        delete[] thunder->childs;
    }
    InitThunder(thunder);
}

static zVEC3 ProjectThunderPoint(const zVEC3& point)
{
    zVEC3 direction = {
        -5008.4f - point.n[0],
        -10500.0f - point.n[1],
        -5597.02f - point.n[2]
    };
    float length = sqrt(direction.n[0] * direction.n[0] + direction.n[1] * direction.n[1] +
        direction.n[2] * direction.n[2]);
    zVEC3 projected = {
        point.n[0] + direction.n[0] / length * 4000.0f,
        point.n[1] + direction.n[1] / length * 4000.0f,
        point.n[2] + direction.n[2] / length * 4000.0f
    };
    return projected;
}

static void BuildThunderPath(oCBarrier* barrier, myThunder* thunder, int start, int segments)
{
    thunder->polyStrip = new zCPolyStrip;
    zCClassDef::ObjectCreated(thunder->polyStrip, zCPolyStrip::classDef);
    thunder->polyStrip->AllocateResources(segments);
    thunder->polyStrip->material->SetTexture("thunder_a0.tga");
    thunder->polyStrip->camAlign = zCPolyStrip::zCAM_ALIGN_POLY;
    thunder->polyStrip->alphaFadeSpeed = 0.25f;
    thunder->polyStrip->width = 1000.0f;
    thunder->numSegs = segments;
    int current = start;
    thunder->polyStrip->AddSegment(barrier->skySphereMesh->vertexList[current]);
    thunder->originVec = ProjectThunderPoint(barrier->skySphereMesh->vertexList[current]);
    thunder->polyStrip->AddSegment(thunder->originVec);
    for (int i = 2; i < segments; ++i) {
        int first = current;
        int second = current;
        myVert& vertex = barrier->myVertList[current];
        for (int j = 0; j < vertex.numNeighbours; ++j) {
            int neighbour = vertex.vertNeighbours[j];
            float height = barrier->skySphereMesh->vertexList[neighbour].n[1];
            if (height < barrier->skySphereMesh->vertexList[first].n[1]) {
                second = first;
                first = neighbour;
            } else if (height < barrier->skySphereMesh->vertexList[second].n[1]) {
                second = neighbour;
            }
        }
        current = (rand() & 1) ? first : second;
        thunder->polyStrip->AddSegment(ProjectThunderPoint(barrier->skySphereMesh->vertexList[current]));
        thunder->valid = 1;
    }
    thunder->valid = 1;
}

int oCBarrier::AddThunderSub(myThunder* thunder, int start, int end, int segments, int depth)
{
    if (!thunder->childs || depth > 1)
        return -1;
    myThunder* child = &thunder->childs[thunder->numChilds];
    InitThunder(child);
    child->isChild = 1;
    BuildThunderPath(this, child, start, segments);
    return 11;
}

int oCBarrier::AddThunder(int start, int segments, float duration, zTThunderSector sector)
{
    int index = -1;
    for (int i = 0; i < numMaxThunders; ++i) {
        if (!myThunderList[i].valid) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return -1;

    myThunder* thunder = &myThunderList[index];
    RemoveThunder(thunder);
    thunder->sector = sector;
    thunder->isChild = 0;
    BuildThunderPath(this, thunder, start, segments);

    int current = start;
    for (int i = 1; i < segments && thunder->numChilds < 3; ++i) {
        if (rand() & 3)
            continue;
        if (!thunder->childs)
            thunder->childs = new myThunder[3];
        if (AddThunderSub(thunder, current, start, 11, 1) != -1) {
            thunder->startTime[thunder->numChilds] = (float)i / (float)segments;
            ++thunder->numChilds;
        }
    }
    thunder->valid = 1;
    ++numMyThunders;
    return index;
}

int oCBarrier::RenderThunder(myThunder* thunder, zTRenderContext& context)
{
    if (!thunder->valid)
        return 0;
    thunder->t1 += ztimer.frameTimeFloat * 0.005f;
    if (thunder->t0 >= 0.9f)
        thunder->t0 = 0.9f;
    if (thunder->t1 >= 0.9f)
        thunder->t1 = 0.9f;
    if (thunder->polyStrip->numSeg <= 2)
        thunder->t0 = 0.9f;

    if ((thunder->t0 < 0.9f || thunder->t1 < 0.9f || thunder->numChilds > 0)) {
        if (thunder->t0 != thunder->t1) {
            thunder->polyStrip->SetVisibleSegments(thunder->t0, thunder->t1);
            thunder->polyStrip->heightCheck = 1;
            thunder->polyStrip->heightBound = -2000.0f;
            thunder->polyStrip->Render(context);
        }
        for (int i = 0; i < thunder->numChilds; ++i) {
            if (thunder->t1 >= thunder->startTime[i] && !RenderThunder(&thunder->childs[i], context))
                --thunder->numChilds;
        }
        return 1;
    }

    if (!thunder->isChild) {
        if (thunder->sector == zTThunderSector_1)
            activeThunder_Sector1 = 0;
        if (thunder->sector == zTThunderSector_2)
            activeThunder_Sector2 = 0;
        if (thunder->sector == zTThunderSector_3)
            activeThunder_Sector3 = 0;
        if (thunder->sector == zTThunderSector_4)
            activeThunder_Sector4 = 0;
        --numMyThunders;
    }
    RemoveThunder(thunder);
    return 0;
}

void oCBarrier::RenderThunderList(zTRenderContext& context)
{
    for (int i = 0; i < numMaxThunders; ++i)
        RenderThunder(&myThunderList[i], context);
}

oCSkyControler_Barrier::oCSkyControler_Barrier()
    : zCSkyControler_Outdoor(1), barrier(new oCBarrier), bFadeInOut(1)
{
}

oCSkyControler_Barrier::~oCSkyControler_Barrier()
{
    delete barrier;
    barrier = 0;
}

void oCSkyControler_Barrier::RenderSkyPre()
{
    zCSkyControler_Outdoor::RenderSkyPre();
    static int initialized;
    if (!initialized) {
        initialized = 1;
        barrier->Init();
    }
    zCSkyControler_Outdoor* outdoor = dynamic_cast<zCSkyControler_Outdoor*>(s_activeSkyControler);
    if (outdoor && s_skyEffectsEnabled && outdoor->m_fFadeScale >= 0.5f &&
        outdoor->GetRenderLightning() && outdoor->GetWeatherType() != zTWEATHER_SNOW) {
        zTRenderContext context = {};
        context.clipFlags = -1;
        context.cam = zCCamera::activeCam;
        context.vob = zCCamera::activeCam->connectedVob;
        context.world = context.vob->GetHomeWorld();
        int alpha = 0;
        int tremor = 0;
        barrier->Render(context, alpha, tremor);
        return;
    }
    zrenderMan.hintLightning = 0;
}
