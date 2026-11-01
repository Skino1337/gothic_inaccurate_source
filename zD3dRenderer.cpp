#include "include/zRndD3d.h"
#include "include/zResource.h"
#include "include/zTexture.h"
#include "include/zTexConvert.h"
#include "include/zMorphMesh.h"
#include "include/zVertexBuffer.h"
#include <cstring>

static int dxDeviceNum;
static int dxDeviceNumber;
static int dxSelectDevice;

// EXE: 0x009FCA04
zCVertexBufferDyn* zD3D_alphaPoly::vertexbufferdyn;
// EXE: 0x009FCA08
zCVertexBuffer* zD3D_alphaPoly::vertexbuffer;
// EXE: 0x009FCA0C
int zD3D_alphaPoly::locked;
// EXE: 0x009FCA10
unsigned long zD3D_alphaPoly::startvertex;
// EXE: 0x009FCA14
unsigned long zD3D_alphaPoly::actvertex;
// EXE: 0x009FCA18
int zD3D_alphaPoly::numberofvertices;
// EXE: 0x009FCA1C
int zD3D_alphaPoly::actnov;
// EXE: 0x009FC9E8
ID3DXContext* zCRnd_D3D::xd3d_pd3dx;
// EXE: 0x009FC9EC
IDirectDraw7* zCRnd_D3D::xd3d_pdd7;
// EXE: 0x009FC9F0
IDirect3D7* zCRnd_D3D::xd3d_pd3d7;
// EXE: 0x009FC9F4
IDirect3DDevice7* zCRnd_D3D::xd3d_pd3dDevice7;
// EXE: 0x009FCA00
unsigned long zCRnd_D3D::xd3d_actFrameNumber;
// EXE: 0x009FC9F8
int zCRnd_D3D::xd3d_dd7inuse;
// EXE: 0x009FC9FC
int zCRnd_D3D::xd3d_palsupp;

int XD3D_AppIDDEnumCallbackEx(_GUID*, char*, char*, void*, HMONITOR__*)
{
    ++dxDeviceNumber;
    dxDeviceNum = dxDeviceNumber;
    return dxDeviceNum - 1 != dxSelectDevice;
}

int zCRnd_D3D::XD3D_EnumerateModes()
{
    dxDeviceNum = 0;
    dxDeviceNumber = 0;
    dxSelectDevice = -1;
    XD3D_AppIDDEnumCallbackEx(0, 0, 0, 0, 0);
    xd3d_actStatus.devmax = dxDeviceNum;
    return 0;
}

int zCRnd_D3D::XD3D_SetRenderState(int state, unsigned long value)
{
    if (xd3d_actRenderState[state] == value)
        return 1;
    xd3d_actRenderState[state] = value;
    if (state == 0x1c && !value) {
        if (!xd3d_disableFog)
            xd3d_actRenderState[0x23] = 0x1f1f1f1f;
        else {
            xd3d_actRenderState[0x8c] = 0x1f1f1f1f;
            xd3d_actRenderState[0x30] = 0x1f1f1f1f;
        }
        xd3d_actRenderState[0x22] = 0x1f1f1f1f;
        xd3d_actRenderState[0x24] = 0x1f1f1f1f;
        xd3d_actRenderState[0x25] = 0x1f1f1f1f;
    }
    return 1;
}

int DXTryWarning(long result, const char* message)
{
    if (result >= 0)
        return 1;
    zSTRING text;
    text.Sprintf("%s Error: 0x%08X", message ? message : "(w/o info)", result);
    zerr->Message(text);
    return 0;
}

zCSurfaceCache_D3D::zD3D_SlotHeader* zCSurfaceCache_D3D::xsc_listofsurfaces[128];
int zCSurfaceCache_D3D::xsc_listposition;

void zCSurfaceCache_D3D::ResetSurfaceList()
{
    std::memset(xsc_listofsurfaces, 0, sizeof(xsc_listofsurfaces));
    xsc_listposition = 0;
}

zCSurfaceCache_D3D::zCSurfaceCache_D3D()
    : xsc_cacheentryCounter(0)
{
    std::memset(xsc_slot, 0, sizeof(xsc_slot));
}

zCSurfaceCache_D3D::~zCSurfaceCache_D3D()
{
    ClearCache();
}

static bool D3DSurfaceSlotValid(const zCSurfaceCache_D3D::zCSlotIndex* slotIndex)
{
    return slotIndex->dynamic >= 0 && slotIndex->dynamic < 2
        && slotIndex->pixelformat >= 0 && slotIndex->pixelformat < 6
        && slotIndex->mipmaps >= 0 && slotIndex->mipmaps < 12
        && slotIndex->width >= 0 && slotIndex->width < 12
        && slotIndex->height >= 0 && slotIndex->height < 12;
}

int zCSurfaceCache_D3D::CacheInSurface(IDirectDrawSurface7* surface, zCSlotIndex* slotIndex)
{
    if (!D3DSurfaceSlotValid(slotIndex))
        return 0;
    zD3D_SlotHeader& slot = xsc_slot[slotIndex->dynamic][slotIndex->pixelformat]
        [slotIndex->mipmaps][slotIndex->width][slotIndex->height];
    if (xsc_cacheentryCounter >= 128) {
        while (!xsc_listofsurfaces[xsc_listposition]) {
            xsc_listposition = (xsc_listposition + 1) & 127;
        }
        XSC_KillLastSurfaceInSlot(xsc_listofsurfaces[xsc_listposition]);
    }
    zD3D_Entry* entry = new zD3D_Entry;
    entry->surface = surface;
    entry->frameNumber = 0;
    entry->nextEntry = 0;
    if (slot.lastEntry)
        slot.lastEntry->nextEntry = entry;
    else
        slot.firstEntry = entry;
    slot.lastEntry = entry;
    ++xsc_cacheentryCounter;
    xsc_listofsurfaces[xsc_listposition] = &slot;
    xsc_listposition = (xsc_listposition + 1) & 127;
    return 1;
}

IDirectDrawSurface7* zCSurfaceCache_D3D::CacheOutSurface(zCSlotIndex* slotIndex)
{
    if (!D3DSurfaceSlotValid(slotIndex))
        return 0;
    zD3D_SlotHeader& slot = xsc_slot[slotIndex->dynamic][slotIndex->pixelformat]
        [slotIndex->mipmaps][slotIndex->width][slotIndex->height];
    zD3D_Entry* entry = slot.firstEntry;
    if (!entry)
        return 0;
    slot.firstEntry = entry->nextEntry;
    if (!slot.firstEntry)
        slot.lastEntry = 0;
    IDirectDrawSurface7* surface = entry->surface;
    delete entry;
    --xsc_cacheentryCounter;
    return surface;
}

void zCSurfaceCache_D3D::ClearCache()
{
    for (int dynamic = 0; dynamic < 2; ++dynamic)
        for (int format = 0; format < 6; ++format)
            for (int mipmaps = 0; mipmaps < 12; ++mipmaps)
                for (int width = 0; width < 12; ++width)
                    for (int height = 0; height < 12; ++height)
                        XSC_ClearSlot(&xsc_slot[dynamic][format][mipmaps][width][height]);
    ResetSurfaceList();
}

void zCSurfaceCache_D3D::XSC_ClearSlot(zD3D_SlotHeader* slot)
{
    while (slot->firstEntry)
        XSC_KillLastSurfaceInSlot(slot);
    slot->lastEntry = 0;
}

void zCSurfaceCache_D3D::XSC_KillLastSurfaceInSlot(zD3D_SlotHeader* slot)
{
    zD3D_Entry* entry = slot->firstEntry;
    if (!entry)
        return;
    slot->firstEntry = entry->nextEntry;
    if (!slot->firstEntry)
        slot->lastEntry = 0;
    entry->surface = 0;
    delete entry;
    --xsc_cacheentryCounter;
}

zCRnd_D3D::zCRnd_D3D()
    : m_bAnisotropicFilter(0), xd3d_disableFog(0), xd3d_bucketSize(0)
{
    xd3d_actStatus.devmax = 1;
    std::memset(xd3d_actRenderState, 0xfd, sizeof(xd3d_actRenderState));
    XD3D_EnumerateModes();
}

zCRenderer::~zCRenderer()
{
}

static int d3dCacheState = -1;
static int d3dRendererState = -1;
static int d3dTextureState = -1;
static int d3dDeviceState = -1;
static int d3dRuntimeState = -1;
static int d3dResourceState = -1;
static int d3dSurfaceState = -1;
zCSurfaceCache_D3D global_surfaceCache;
zCDXTCCache_D3D global_DXTCCache;

int zCRnd_D3D::XD3D_InitPerDX(long device, int xdim, int ydim, int bpp, int flags)
{
    return XD3D_InitPort(static_cast<int>(device), xdim, ydim, bpp, flags);
}

int zCRnd_D3D::XD3D_InitPort(int device, int xdim, int ydim, int bpp, int)
{
    global_surfaceCache.ClearCache();
    global_DXTCCache.ClearCache();
    xd3d_scrWidth = xdim;
    xd3d_scrStatus = device;
    vid_xdim = xdim;
    vid_ydim = ydim;
    vid_bpp = bpp;
    return XD3D_TestCapabilities();
}

int zCRnd_D3D::XD3D_TestCapabilities()
{
    return 1;
}

int zCRnd_D3D::XD3D_SetDevice(int device, int xdim, int ydim, int bpp, int screenMode)
{
    xd3d_deviceChange = screenMode;
    if (xd3d_deviceChange == 2)
        xd3d_deviceChange = 1;
    XD3D_ClearDevice();
    const int result = XD3D_InitPort(device, xdim, ydim, bpp, screenMode);
    if (result)
        xd3d_actStatus.devnum = screenMode ? 0 : device;
    return result;
}

void zCRnd_D3D::XD3D_ClearDevice()
{
    global_surfaceCache.ClearCache();
    global_DXTCCache.ClearCache();
    xd3d_scrBpp = -1;
}

int zCRnd_D3D::XD3D_TextureRescue(zCTex_D3D* texture)
{
    zD3D_savedTexture* saved = new zD3D_savedTexture;
    saved->texture = texture;
    saved->texCon = 0;
    saved->next = xd3d_numTexturesUsedThisFrame;
    xd3d_numTexturesUsedThisFrame = saved;
    if (!texture->RescueTexture(saved)) {
        delete saved;
        return 0;
    }
    return 1;
}

int zCRnd_D3D::XD3D_TextureRebuild()
{
    while (xd3d_numTexturesUsedThisFrame) {
        zD3D_savedTexture* saved = xd3d_numTexturesUsedThisFrame;
        xd3d_numTexturesUsedThisFrame = saved->next;
        saved->texture->RebuildTexture(saved);
        delete saved;
    }
    return 1;
}

int zCRnd_D3D::XD3D_CreateWin(int xdim, int ydim, int bpp)
{
    vid_xdim = xdim;
    vid_ydim = ydim;
    vid_bpp = bpp;
    return 1;
}

void zCRnd_D3D::XD3D_CloseWin()
{
    xd3d_newWinHandle = GetActiveWindow();
    SetFocus(static_cast<HWND>(xd3d_newWinHandle));
}

int zCRnd_D3D::GetRadialFog() const
{
    return radialFog;
}

void zCRnd_D3D::SetAlphaReference(unsigned long reference)
{
    alphaReference = reference;
}

unsigned long zCRnd_D3D::GetAlphaReference() const
{
    return alphaReference;
}

int zCRnd_D3D::GetAlphaLimitReached() const
{
    return alphaLimitReached;
}

int zCRnd_D3D::GetCacheAlphaPolys() const
{
    return cacheAlphaPolys;
}

void zCRnd_D3D::SetCacheAlphaPolys(int enabled)
{
    cacheAlphaPolys = enabled;
}

int zCRnd_D3D::GetSurfaceLost() const
{
    return surfaceLost;
}

void zCRnd_D3D::SetSurfaceLost(int lost)
{
    surfaceLost = lost;
}

int zCRnd_D3D::GetSyncOnAmbientCol() const
{
    return syncAmbient;
}

void zCRnd_D3D::SetSyncOnAmbientCol(int enabled)
{
    syncAmbient = enabled;
}

int zCRnd_D3D::Vid_IsLocked()
{
    return 0;
}

void zCRnd_D3D::Vid_EndLfbAccess()
{
}

void zCRnd_D3D::Vid_SetLfbAlpha(int alpha)
{
    alphaFactor = alpha;
}

void zCRnd_D3D::Vid_SetLfbAlphaFunc(const zTRnd_AlphaBlendFunc& func)
{
    alphaFunc = func;
}

zCRndAlphaSortObject::~zCRndAlphaSortObject()
{
}

void zCRndAlphaSortObject::Draw(int)
{
}

int zCRndAlphaSortObject::IsAlphaPoly()
{
    return 0;
}

zD3D_alphaPoly::zD3D_alphaPoly()
{
}

zD3D_alphaPoly::~zD3D_alphaPoly()
{
}

int zD3D_alphaPoly::IsAlphaPoly()
{
    return 1;
}

zCRnd_D3D::~zCRnd_D3D()
{
    XD3D_ClearDevice();
    if (xd3d_scrMode)
        XD3D_CloseWin();
}

void zCRnd_D3D::EmergencyExit()
{
    XD3D_ClearDevice();
    if (xd3d_scrMode)
        XD3D_CloseWin();
}

int zCTexAniCtrl::masterFrameCtr;

void zCTexAniCtrl::AdvanceAni(zCTexture*)
{
}

zCTexture* zCMaterial::GetAniTexture()
{
    if (!texture)
        return 0;
    if (texture->changingRealtime)
        texAniCtrl.AdvanceAni(texture);
    return texture->GetAniTexture();
}

static void D3DDrawPolygon(zCRnd_D3D* renderer, zCPolygon* polygon, zCTexture* texture)
{
    if (!polygon || renderer->GetSurfaceLost())
        return;
    if (texture && (texture->cacheState & 3) != 3)
        zresMan->CacheIn(texture, 0.5f);
    zTRndSimpleVertex vertices[8];
    const int count = polygon->numClipVert < 8 ? polygon->numClipVert : 8;
    for (int i = 0; i < count; ++i) {
        vertices[i].pos.n[0] = polygon->clipVert[i]->position.n[0];
        vertices[i].pos.n[1] = polygon->clipVert[i]->position.n[1];
        vertices[i].z = polygon->clipVert[i]->position.n[2];
        if (polygon->clipFeat) {
            vertices[i].uv.n[0] = polygon->clipFeat[i]->mapping.n[0];
            vertices[i].uv.n[1] = polygon->clipFeat[i]->mapping.n[1];
        } else {
            vertices[i].uv.n[0] = 0.0f;
            vertices[i].uv.n[1] = 0.0f;
        }
        vertices[i].color = 0xffffffff;
    }
    renderer->DrawPolySimple(texture, vertices, count);
}

void zCRnd_D3D::DrawPolySimple(zCTexture* texture, zTRndSimpleVertex*, int numVertices)
{
    if (!texture || GetSurfaceLost())
        return;
    if ((texture->cacheState & 3) != 3)
        zresMan->CacheIn(texture, 0.5f);
    else
        texture->TouchTimeStamp();
    if ((texture->cacheState & 3) != 3 || numVertices < 3)
        return;
    const int oldFog = GetFog();
    SetFog(0);
    xd3d_statistics.numPolysRendered += numVertices - 2;
    ++xd3d_statistics.numLightmapsUsed;
    SetFog(oldFog);
}

void zCRnd_D3D::DrawPoly(zCPolygon* polygon)
{
    if (GetSurfaceLost())
        return;
    switch (renderMode) {
    case zRND_MODE_1_PASS_VERT_LIGHT:
        XD3D_PolyDrawOnePassVertLight(polygon);
        break;
    case zRND_MODE_1_PASS_MULTITEXTURE:
        XD3D_PolyDrawOnePassMultiTex(polygon);
        break;
    case zRND_MODE_2_PASS_TEXTURE:
        XD3D_PolyDrawTwoPassTexture(polygon);
        break;
    case zRND_MODE_2_PASS_LIGHTMAP:
        XD3D_PolyDrawTwoPassLightMap(polygon);
        break;
    default:
        XD3D_PolyDrawOnePassVertLight(polygon);
        break;
    }
}

void zCRnd_D3D::XD3D_PolyDrawOnePassVertLight(zCPolygon* polygon)
{
    if (polygon && polygon->material)
        D3DDrawPolygon(this, polygon, polygon->material->GetAniTexture());
}

void zCRnd_D3D::XD3D_PolyDrawOnePassMultiTex(zCPolygon* polygon)
{
    if (polygon && polygon->material)
        D3DDrawPolygon(this, polygon, polygon->material->GetAniTexture());
}

void zCRnd_D3D::XD3D_PolyDrawTwoPassTexture(zCPolygon* polygon)
{
    if (polygon && polygon->material)
        D3DDrawPolygon(this, polygon, polygon->material->GetAniTexture());
}

void zCRnd_D3D::XD3D_AlphaTestPoly(zCPolygon* polygon)
{
    if (polygon && polygon->material)
        D3DDrawPolygon(this, polygon, polygon->material->GetAniTexture());
}

void zCRnd_D3D::XD3D_AlphaBlendPoly(zCPolygon* polygon, zCMaterial*)
{
    if (alphaPolys.numInArray >= 2048) {
        alphaLimitReached = 1;
        return;
    }
    alphaPolys.InsertEnd(polygon);
    ++xd3d_statistics.numTrisRendered;
}

void zCRnd_D3D::AddAlphaPoly(const zCPolygon* polygon)
{
    if (polygon && polygon->material)
        XD3D_AlphaBlendPoly(const_cast<zCPolygon*>(polygon), polygon->material);
}

void zCRnd_D3D::FlushAlphaPolys()
{
    for (int i = 0; i < alphaPolys.numInArray; ++i) {
        zCPolygon* polygon = const_cast<zCPolygon*>(alphaPolys.parray[i]);
        if (polygon && polygon->material)
            D3DDrawPolygon(this, polygon, polygon->material->GetAniTexture());
    }
    alphaPolys.numInArray = 0;
}

void zCRnd_D3D::XD3D_PolyDrawTwoPassLightMap(zCPolygon* polygon)
{
    zCLightMap* lightmap = polygon->lightmap;
    zCTexture* texture = lightmap->tex;
    if (!texture)
        return;
    if ((texture->cacheState & 3) != 3)
        zresMan->CacheIn(texture, 0.5f);
    else
        texture->TouchTimeStamp();
    if ((texture->cacheState & 3) != 3)
        return;

    for (int i = 0; i < polygon->numClipVert; ++i) {
        zVEC3 position = polygon->clipVert[i]->position - lightmap->lightmapOrigin;
        float u = position * lightmap->lightmapUVRight;
        float v = position * lightmap->lightmapUVUp;
        xd3d_scale_B = u + v;
    }
    SetTextureStageState(0, zRND_TSS_COLOROP, 0);
    ++xd3d_numLightmapsUsedThisFrame;
    xd3d_scale_C = 0.0f;
}

void zCRnd_D3D::DrawLightmapList(zCPolygon** polygons, int numPolygons)
{
    XD3D_SetRenderState(0x88, 0);
    XD3D_SetRenderState(0x16, 1);
    xd3d_statistics.numLightmapsUsed += numPolygons;
    for (int i = 0; i < numPolygons; ++i) {
        xd3d_statistics.numPolysRendered += polygons[i]->numClipVert - 2;
        XD3D_PolyDrawTwoPassLightMap(polygons[i]);
    }
}

void zCRnd_D3D::SetPixel(float x, float y, zCOLOR color)
{
    XD3D_SetRenderState(0x88, 0);
    XD3D_SetRenderState(0x16, 1);
    XD3D_SetRenderState(0x1b, 0);
    SetTextureStageState(0, zRND_TSS_COLOROP, 3);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    XD3D_SetRenderState(0x0e, 0);
    XD3D_SetRenderState(0x0f, 0);
    SetZBufferCompare(xd3d_actStatus.zfunc);
    SetTexture(0, 0);
    xd3d_scale_A = x;
    xd3d_scale_B = y;
    xd3d_texturefactor = color | 0xff000000;
    SetTextureStageState(0, zRND_TSS_COLOROP, color);
    XD3D_SetRenderState(0x0e, xd3d_actStatus.zwrite);
    XD3D_SetRenderState(0x17, xd3d_actStatus.zfunc);
    xd3d_scale_C = 0.0f;
}

void zCRnd_D3D::DrawLine(float x1, float y1, float x2, float y2, zCOLOR color)
{
    int oldFog = GetFog();
    SetFog(0);
    XD3D_SetRenderState(0x88, 0);
    XD3D_SetRenderState(0x16, 1);
    XD3D_SetRenderState(0x1b, 0);
    SetTextureStageState(0, zRND_TSS_COLOROP, 4);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    XD3D_SetRenderState(0x0e, 0);
    XD3D_SetRenderState(0x0f, 0);
    SetZBufferCompare(xd3d_actStatus.zfunc);
    SetTexture(0, 0);
    xd3d_scale_A = x2 - x1;
    xd3d_scale_B = y2 - y1;
    xd3d_texturefactor = color | 0xff000000;
    XD3D_SetRenderState(0x0e, xd3d_actStatus.zwrite);
    XD3D_SetRenderState(0x17, xd3d_actStatus.zd3dfunc);
    xd3d_scale_C = 0.0f;
    SetFog(oldFog);
}

void zCRnd_D3D::DrawLineZ(float x1, float y1, float z1, float x2, float y2, float z2,
    zCOLOR color)
{
    int oldFog = GetFog();
    SetFog(0);
    XD3D_SetRenderState(0x88, 0);
    XD3D_SetRenderState(0x16, 1);
    XD3D_SetRenderState(0x1b, 0);
    SetTextureStageState(0, zRND_TSS_COLOROP, 3);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    XD3D_SetRenderState(0x1b, 0);
    XD3D_SetRenderState(0x0f, 0);
    SetZBufferCompare(xd3d_actStatus.zfunc);
    SetTexture(0, 0);
    const float depthScale = xd3d_scale_A;
    xd3d_scale_A = z1 * depthScale + xd3d_zMIN_from_Engine;
    xd3d_scale_B = z2 * depthScale + xd3d_zMIN_from_Engine;
    xd3d_texturefactor = color | 0xff000000;
    xd3d_scale_C = 0.0f;
    SetFog(oldFog);
}

void zCRnd_D3D::FlushPolys()
{
    RenderAlphaSortList();
}

void zCRnd_D3D::BeginFrame()
{
    static unsigned long frameNumber;
    ++frameNumber;
    ResetStatistics();
    std::memset(xd3d_alphaSortBucket, 0, sizeof(xd3d_alphaSortBucket));
    xd3d_numAlphaPolys = 0;
    float farZ = zCCamera::activeCam ? zCCamera::activeCam->farClipZ : 65535.0f;
    if (farZ < 500.0f)
        farZ = 500.0f;
    xd3d_bucketSize = 256.0f / farZ;
    float range = farZ - xd3d_zMAX_from_Engine;
    xd3d_zMIN_from_Engine = farZ / range;
    xd3d_scale_A = -(xd3d_zMAX_from_Engine * farZ / range);
    xd3d_inScene = 1;
    if (xd3d_actStatus.fog)
        SetFog(xd3d_actStatus.fog);
    SetTextureStageState(0, zRND_TSS_COLOROP, 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(2), 0);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    SetZBias(0);
}

void zCRnd_D3D::EndFrame()
{
    if (zCWorld::showTextureStats && screen) {
        screen->Print(0, 1500, zSTRING("bThrashing\t\t\t: ") + zSTRING(0));
        screen->Print(0, 1700, zSTRING("numTexturesUsed\t\t: ") + zSTRING(0));
        screen->Print(0, 1900, zSTRING("numTexturesUsedInVid: ") + zSTRING(0));
        screen->Print(0, 2100, zSTRING("numTexturesInVid\t: ") + zSTRING(0));
        screen->Print(0, 2300, zSTRING("totalInVid   (kb)\t: ") + zSTRING(0));
        screen->Print(0, 2500, zSTRING("numManagedTotal\t\t: ") + zSTRING(0));
        screen->Print(0, 2700, zSTRING("managedTotal   (kb)\t: ") + zSTRING(0));
        screen->Print(0, 2900, zSTRING("dwNumLoads\t\t\t: ") + zSTRING(0));
        screen->Print(0, 3100, zSTRING("kb loaded via Load(): ") + zSTRING(0));
        screen->Print(0, 3300, zSTRING("dwNumCreates\t\t: ") + zSTRING(0));
        screen->Print(0, 3500, zSTRING("dwNumDestroys\t\t: ") + zSTRING(0));
        screen->Print(0, 3700, zSTRING("dwNumGetDCs\t\t\t: ") + zSTRING(0));
        screen->Print(0, 3900, zSTRING("dwNumLocks\t\t\t: ") + zSTRING(0));
        screen->Print(0, 4100, zSTRING("dwNumPreLoads\t\t: ") + zSTRING(0));
        screen->Print(0, 4300, zSTRING("dwNumSet\t\t\t: ") + zSTRING(0));
        screen->Print(0, 4500, zSTRING("dwNumSetLODs\t\t: ") + zSTRING(0));
        screen->Print(0, 4700, zSTRING("dwNumSetPriorities\t: ") + zSTRING(0));
        screen->Print(0, 6100, zSTRING("num Lightmaps\t\t: ") + zSTRING(xd3d_statistics.numTexturesUsed));
        screen->Print(0, 6300, zSTRING("num Alphapolys\t\t: ") + zSTRING(xd3d_statistics.numTrisRendered));
        screen->Print(0, 6500, zSTRING("dito, batched\t\t: ") + zSTRING(xd3d_statistics.numAlphaPolys));
    }
    xd3d_inScene = 0;
}

int zCRnd_D3D::SetTransform(zTRnd_TrafoType type, const zMAT4& trafo)
{
    if (type < zRND_TRAFO_VIEW || type > zRND_TRAFO_TEXTURE3)
        return 1;
    xd3d_transform[type] = trafo;
    if (type == zRND_TRAFO_PROJECTION) {
        if (trafo.v[3].n[2] != 0.0f)
            zerr->Message("X: SetTransform: matrix[3][3]!=0.0f !!!");
        xd3d_zMIN_from_Engine = trafo.v[2].n[2];
        xd3d_zMAX_from_Engine = trafo.v[3].n[2];
        xd3d_scale_B = trafo.v[2].n[3];
    }
    return 1;
}

int zCRnd_D3D::SetViewport(int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0)
        return 0;
    xd3d_scrWidth = width;
    vid_xdim = x + width;
    vid_ydim = y + height;
    return 1;
}

int zCRnd_D3D::SetLight(unsigned long index, zCRenderLight* light)
{
    if (index >= 32)
        return 0;
    if (!light) {
        XD3D_SetRenderState(0x8b, 0);
        return 1;
    }
    if (light->lightType == 0) {
        unsigned long color = (static_cast<unsigned long>(light->colorDiffuse.n[0]) << 16)
            | (static_cast<unsigned long>(light->colorDiffuse.n[1]) << 8)
            | static_cast<unsigned long>(light->colorDiffuse.n[2]);
        XD3D_SetRenderState(0x8b, color);
    } else if (light->lightType == 3 && light->directionLS.n[0] == 0.0f
        && light->directionLS.n[1] == 0.0f && light->directionLS.n[2] == 0.0f) {
        light->directionLS = zVEC3(1.0f, 0.0f, 0.0f);
    }
    return 1;
}

int zCRnd_D3D::SetMaterial(const zTMaterial& material)
{
    xd3d_actmaterial = material;
    return 1;
}

int zCRnd_D3D::GetMaterial(zTMaterial& material)
{
    material = xd3d_actmaterial;
    return 1;
}

int zCRnd_D3D::XD3D_SetTexture(unsigned long stage, IDirectDrawSurface7* surface)
{
    if (GetPolyDrawMode() <= zRND_DRAW_MATERIAL_WIRE && xd3d_pd3dtexture[stage] != surface)
        xd3d_pd3dtexture[stage] = surface;
    return 1;
}

int zCRnd_D3D::SetTexture(unsigned long stage, zCTexture* texture)
{
    if (GetPolyDrawMode() > zRND_DRAW_MATERIAL_WIRE)
        return 1;
    if (texture) {
        if ((texture->cacheState & 3) != 3)
            zresMan->CacheIn(texture, 0.5f);
        else
            texture->TouchTimeStamp();
        if ((texture->cacheState & 3) == 3) {
            if (!static_cast<zCTex_D3D*>(texture)->SetTexture(stage))
                return 0;
            xd3d_ptexture[stage] = texture;
            return 1;
        }
    }
    XD3D_SetTexture(stage, 0);
    xd3d_ptexture[stage] = 0;
    return 1;
}

int zCRnd_D3D::SetTextureStageState(unsigned long stage, zTRnd_TextureStageState state,
    unsigned long value)
{
    xd3d_actTexStageState[stage][state] = value;
    return 1;
}

int zCRnd_D3D::SetAlphaBlendFuncImmed(zTRnd_AlphaBlendFunc func)
{
    switch (func) {
    case zRND_ALPHA_FUNC_BLEND:
        XD3D_SetRenderState(0x1b, 1);
        XD3D_SetRenderState(0x13, 5);
        XD3D_SetRenderState(0x14, 6);
        break;
    case zRND_ALPHA_FUNC_ADD:
        XD3D_SetRenderState(0x1b, 1);
        XD3D_SetRenderState(0x13, 5);
        XD3D_SetRenderState(0x14, 2);
        break;
    case zRND_ALPHA_FUNC_MUL:
        XD3D_SetRenderState(0x1b, 1);
        XD3D_SetRenderState(0x13, 9);
        XD3D_SetRenderState(0x14, 1);
        break;
    case zRND_ALPHA_FUNC_MUL2:
        XD3D_SetRenderState(0x1b, 1);
        XD3D_SetRenderState(0x13, 9);
        XD3D_SetRenderState(0x14, 3);
        break;
    case zRND_ALPHA_FUNC_TEST:
        XD3D_SetRenderState(0x1b, 1);
        XD3D_SetRenderState(0x13, 2);
        XD3D_SetRenderState(0x14, 1);
        XD3D_SetRenderState(0x1a, 0);
        XD3D_SetRenderState(0x0e, 1);
        XD3D_SetRenderState(0x0f, 1);
        XD3D_SetRenderState(0x18, alphaReference);
        return 1;
    case zRND_ALPHA_FUNC_BLEND_TEST:
        XD3D_SetRenderState(0x1b, 1);
        XD3D_SetRenderState(0x13, 5);
        XD3D_SetRenderState(0x14, 6);
        XD3D_SetRenderState(0x1a, 0);
        XD3D_SetRenderState(0x0e, 1);
        XD3D_SetRenderState(0x0f, 1);
        XD3D_SetRenderState(0x18, alphaReference);
        return 1;
    default:
        XD3D_SetRenderState(0x1b, 0);
        XD3D_SetRenderState(0x0f, 0);
        XD3D_SetRenderState(0x1a, xd3d_actStatus.dither);
        XD3D_SetRenderState(0x0e, xd3d_actStatus.zwrite);
        SetZBufferCompare(xd3d_actStatus.zfunc);
        return 1;
    }
    XD3D_SetRenderState(0x0f, 0);
    XD3D_SetRenderState(0x1a, 0);
    XD3D_SetRenderState(0x0e, 0);
    SetZBufferCompare(xd3d_actStatus.zfunc);
    return 1;
}

int zCRnd_D3D::SetRenderState(zTRnd_RenderStateType state, unsigned long value)
{
    if (state == zRND_RENDERSTATE_CLIPPING) {
        alphaReference = value;
        return 1;
    }
    if (state != zRND_RENDERSTATE_TEXTUREFACTOR)
        return 0;
    XD3D_SetRenderState(0x3c, value);
    xd3d_vbclipping = value;
    return 1;
}

unsigned long zCRnd_D3D::GetRenderState(zTRnd_RenderStateType state)
{
    if (state == zRND_RENDERSTATE_CLIPPING)
        return alphaReference;
    if (state == zRND_RENDERSTATE_TEXTUREFACTOR)
        return xd3d_vbclipping;
    return 0;
}

void zCRnd_D3D::AddAlphaSortObject(zCRndAlphaSortObject* object)
{
    if (object->zvalue == 0.0f)
        return;
    int bucket = static_cast<int>(object->zvalue * xd3d_bucketSize);
    if (bucket >= 256)
        bucket = 255;
    else if (bucket < 0)
        bucket = 0;
    zCRndAlphaSortObject* current = xd3d_alphaSortBucket[bucket];
    if (!current || current->zvalue <= object->zvalue) {
        object->nextSortObject = current;
        xd3d_alphaSortBucket[bucket] = object;
        return;
    }
    zCRndAlphaSortObject* next = current->nextSortObject;
    while (next && next->zvalue > object->zvalue) {
        current = next;
        next = next->nextSortObject;
    }
    current->nextSortObject = object;
    object->nextSortObject = next;
}

void zCRnd_D3D::RenderAlphaSortList()
{
    SetTexture(0, 0);
    int numDrawn = 0;
    for (int bucket = 255; bucket >= 0; --bucket) {
        zCRndAlphaSortObject* object = xd3d_alphaSortBucket[bucket];
        while (object) {
            object->Draw(numDrawn);
            ++numDrawn;
            object = object->nextSortObject;
        }
        xd3d_alphaSortBucket[bucket] = 0;
    }
    XD3D_SetRenderState(0x0e, xd3d_actStatus.zwrite);
    static const int zFunctions[] = { 8, 1, 2, 4 };
    if (xd3d_actStatus.zfunc < 4)
        xd3d_actStatus.zd3dfunc = zFunctions[xd3d_actStatus.zfunc];
    XD3D_SetRenderState(0x17, xd3d_actStatus.zd3dfunc);
    XD3D_SetRenderState(0x1a, xd3d_actStatus.dither);
    XD3D_SetRenderState(0x1b, 0);
    SetTextureStageState(0, zRND_TSS_COLOROP, 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(2), 0);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(12), xd3d_actStatus.texwrap ? 1 : 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(13), xd3d_actStatus.texwrap ? 1 : 3);
    XD3D_SetRenderState(0x0f, 0);
    SetTexture(0, 0);
    xd3d_scale_C = 0.0f;
}

void zD3D_alphaPoly::Draw(int flags)
{
    zCRnd_D3D* renderer = static_cast<zCRnd_D3D*>(zrenderer);
    renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(4), 3);
    renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(5), 1);
    renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 3);
    renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(2), 1);
    renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(23), 0);
    renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(10), 0);
    renderer->SetZBias(zBias);
    int oldFog = renderer->GetFog();
    if (tex) {
        renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(12), wrap ? 1 : 3);
        renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(13), wrap ? 1 : 3);
        renderer->SetTexture(0, tex);
    } else {
        renderer->SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(3), 2);
        renderer->SetTextureStageState(0, zRND_TSS_COLOROP, 2);
    }
    if (alphafunc == zRND_ALPHA_FUNC_SUB)
        zerr->Message("X: FlushPolys: Alpha function zRND_ALPHA_FUNC_SUB is not supported");
    else
        renderer->SetAlphaBlendFuncImmed(alphafunc);
    if (renderer->GetPolyDrawMode() == zRND_DRAW_WIRE) {
        for (int i = 0; i < numVert; ++i)
            vertexList[i].a_color = 0xffffffff;
        renderer->SetTexture(0, 0);
    }
    renderer->SetFog(oldFog);
    if (flags == 0)
        ++renderer->xd3d_statistics.numAlphaPolyBatches;
}

int zCRnd_D3D::XD3D_DrawVertexBufferWire(zCVertexBuffer* buffer, int, int,
    unsigned short* indices, unsigned long numIndices)
{
    SetTexture(0, 0);
    if (numIndices == 0) {
        indices = buffer->GetIndexListPtr();
        numIndices = buffer->GetIndexListSize();
    }
    XD3D_SetRenderState(8, 2);
    SetRenderState(zRND_RENDERSTATE_TEXTUREFACTOR, 0xffffffff);
    SetTextureStageState(0, zRND_TSS_COLOROP, 1);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 4);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(2), 1);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    xd3d_numAlphaPolys = numIndices;
    XD3D_SetRenderState(8, 3);
    return 1;
}

int zCRnd_D3D::XD3D_DrawVertexBufferFlat(zCVertexBuffer* buffer, int, int,
    unsigned short* indices, unsigned long numIndices)
{
    SetTexture(0, 0);
    if (numIndices == 0) {
        indices = buffer->GetIndexListPtr();
        numIndices = buffer->GetIndexListSize();
    }
    XD3D_SetRenderState(0x3c, 0xffffffff);
    SetTextureStageState(0, zRND_TSS_COLOROP, 1);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 1);
    SetTextureStageState(1, zRND_TSS_COLOROP, 0);
    xd3d_numAlphaPolys = numIndices;
    return 1;
}

int zCRnd_D3D::DrawVertexBuffer(zCVertexBuffer* buffer, int firstVertex, int numVertices,
    unsigned short* indices, unsigned long numIndices)
{
    if (numVertices == 0)
        return 1;
    if (numVertices < 0)
        numVertices = buffer->numVertex;
    if (numIndices == 0) {
        indices = buffer->GetIndexListPtr();
        numIndices = buffer->GetIndexListSize();
    }
    int count = numIndices ? numIndices : numVertices;
    int primitive = buffer->GetPrimitiveType();
    if (primitive == zVBUFFER_PT_TRIANGLELIST)
        count /= 3;
    else if (primitive == zVBUFFER_PT_TRIANGLESTRIP || primitive == zVBUFFER_PT_TRIANGLEFAN)
        count -= 2;
    else
        count = 0;
    xd3d_statistics.numPolysRendered += count;
    xd3d_statistics.numLightmapsUsed += primitive == zVBUFFER_PT_TRIANGLEFAN ? 1 : count;
    int vertexType = buffer->GetVertexType();
    XD3D_SetRenderState(0x89, vertexType != zVBUFFER_VERTTYPE_T_L);
    XD3D_SetRenderState(0x88, vertexType != zVBUFFER_VERTTYPE_T_L);
    XD3D_SetRenderState(0x16, vertexType == zVBUFFER_VERTTYPE_T_L ? 1 : 2);
    int result = 1;
    if (polyDrawMode == zRND_DRAW_FLAT)
        result = XD3D_DrawVertexBufferFlat(buffer, firstVertex, numVertices, indices, numIndices);
    else if (polyDrawMode == zRND_DRAW_WIRE || polyDrawMode == zRND_DRAW_MATERIAL_WIRE)
        result = XD3D_DrawVertexBufferWire(buffer, firstVertex, numVertices, indices, numIndices);
    XD3D_SetRenderState(0x89, 0);
    return result;
}

void zCRnd_D3D::Vid_BeginLfbAccess()
{
    float farZ = zCCamera::activeCam ? zCCamera::activeCam->farClipZ : 65535.0f;
    float range = farZ - xd3d_zMAX_from_Engine;
    xd3d_zMIN_from_Engine = farZ / range;
    xd3d_scale_A = -(xd3d_zMAX_from_Engine * farZ / range);
}

zCVertexBuffer* zCRnd_D3D::CreateVertexBuffer()
{
    zCVertexBuffer_D3D* buffer = new zCVertexBuffer_D3D;
    zCClassDef::ObjectCreated(buffer, &zCVertexBuffer::classDef);
    return buffer;
}

void zCRnd_D3D::SetFog(int enabled)
{
    if (xd3d_actStatus.fog == enabled)
        return;
    xd3d_actStatus.fog = enabled;
    if (xd3d_inScene)
        return;
    XD3D_SetRenderState(0x1c, enabled != 0);
    if (!enabled) {
        XD3D_SetRenderState(0x30, 0);
        XD3D_SetRenderState(0x8c, 0);
        XD3D_SetRenderState(0x23, 0);
        return;
    }
    XD3D_SetRenderState(xd3d_disableFog ? 0x8c : 0x23, 3);
    unsigned long color = (static_cast<unsigned long>(fogColor) & 0x00ffffff);
    XD3D_SetRenderState(0x22, color);
    XD3D_SetRenderState(0x24, *reinterpret_cast<unsigned long*>(&fogNear));
    XD3D_SetRenderState(0x25, *reinterpret_cast<unsigned long*>(&fogFar));
    XD3D_SetRenderState(0x30, xd3d_disableFog != 0);
}

void zCRnd_D3D::SetRadialFog(int enabled)
{
    xd3d_disableFog = enabled;
    fogType = xd3d_inScene ? 2 : (enabled ? 1 : 2);
    SetFog(0);
    if (!xd3d_inScene)
        SetFog(1);
}

int zCRnd_D3D::GetFog() const
{
    return xd3d_actStatus.fog;
}

void zCRnd_D3D::SetFogColor(const zCOLOR& color)
{
    fogColor = color;
    XD3D_SetRenderState(0x22, static_cast<unsigned long>(color) & 0x00ffffff);
}

void zCRnd_D3D::SetFogType(int type)
{
    fogType = type;
    if (type == 1) {
        XD3D_SetRenderState(0x23, 0);
        XD3D_SetRenderState(0x8c, 3);
        XD3D_SetRenderState(0x30, 1);
    } else if (type == 2) {
        XD3D_SetRenderState(0x30, 0);
        XD3D_SetRenderState(0x8c, 0);
        XD3D_SetRenderState(0x23, 3);
    }
}

zCOLOR zCRnd_D3D::GetFogColor() const
{
    return fogColor;
}

void zCRnd_D3D::SetFogRange(float nearZ, float farZ, zTRnd_FogMode mode)
{
    fogNear = nearZ;
    fogFar = farZ;
    fogMode = mode;
    XD3D_SetRenderState(0x24, *reinterpret_cast<unsigned long*>(&fogNear));
    XD3D_SetRenderState(0x25, *reinterpret_cast<unsigned long*>(&fogFar));
    XD3D_SetRenderState(xd3d_disableFog ? 0x8c : 0x23, 3);
    if (xd3d_disableFog)
        XD3D_SetRenderState(0x30, 1);
}

void zCRnd_D3D::GetFogRange(float& nearZ, float& farZ, zTRnd_FogMode& mode)
{
    nearZ = fogNear;
    farZ = fogFar;
    mode = fogMode;
}

void zCRnd_D3D::SetPolyDrawMode(const zTRnd_PolyDrawMode& mode)
{
    polyDrawMode = mode;
}

zTRnd_PolyDrawMode zCRnd_D3D::GetPolyDrawMode() const
{
    return polyDrawMode;
}

void zCRnd_D3D::SetTextureWrapEnabled(int enabled)
{
    xd3d_actStatus.texwrap = enabled;
}

int zCRnd_D3D::GetTextureWrapEnabled() const
{
    return xd3d_actStatus.texwrap;
}

void zCRnd_D3D::SetBilerpFilterEnabled(int enabled)
{
    xd3d_actStatus.filter = enabled;
    if (!enabled) {
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(15), 1);
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(16), 1);
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(17), 2);
        return;
    }
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(15), cacheAlphaPolys ? 5 : 2);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(16), cacheAlphaPolys ? 3 : 2);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(17), 3);
}

int zCRnd_D3D::GetBilerpFilterEnabled() const
{
    return xd3d_actStatus.filter;
}

void zCRnd_D3D::SetDitherEnabled(int enabled)
{
    xd3d_actStatus.dither = enabled;
}

int zCRnd_D3D::GetDitherEnabled() const
{
    return xd3d_actStatus.dither;
}

void zCRnd_D3D::SetPolySortMode(const zTRnd_PolySortMode& mode)
{
    polySortMode = mode;
    if (mode == zRND_POLYSORT_NONE)
        XD3D_SetRenderState(7, 0);
    else
        XD3D_SetRenderState(7, xd3d_numLightmapsUsedThisFrame ? 2 : 1);
}

zTRnd_PolySortMode zCRnd_D3D::GetPolySortMode() const
{
    return polySortMode;
}

void zCRnd_D3D::SetZBufferWriteEnabled(int enabled)
{
    xd3d_actStatus.zwrite = enabled;
    XD3D_SetRenderState(0x0e, enabled);
}

int zCRnd_D3D::GetZBufferWriteEnabled() const
{
    return xd3d_actStatus.zwrite;
}

void zCRnd_D3D::SetZBufferCompare(const zTRnd_ZBufferCmp& compare)
{
    xd3d_actStatus.zfunc = compare;
    static const int zFunctions[] = { 8, 1, 2, 4 };
    xd3d_actStatus.zd3dfunc = compare <= zRND_ZBUFFER_CMP_LESS_EQUAL ? zFunctions[compare] : 4;
    XD3D_SetRenderState(0x17, xd3d_actStatus.zd3dfunc);
}

zTRnd_ZBufferCmp zCRnd_D3D::GetZBufferCompare()
{
    return static_cast<zTRnd_ZBufferCmp>(xd3d_actStatus.zfunc);
}

void zCRnd_D3D::SetPixelWriteEnabled(int)
{
    pixelWrite = 1;
}

int zCRnd_D3D::GetPixelWriteEnabled() const
{
    return pixelWrite;
}

void zCRnd_D3D::SetAlphaBlendSource(const zTRnd_AlphaBlendSource& source)
{
    alphaSource = source;
}

zTRnd_AlphaBlendSource zCRnd_D3D::GetAlphaBlendSource() const
{
    return alphaSource;
}

void zCRnd_D3D::SetAlphaBlendFunc(const zTRnd_AlphaBlendFunc& func)
{
    alphaFunc = func;
}

zTRnd_AlphaBlendFunc zCRnd_D3D::GetAlphaBlendFunc() const
{
    return alphaFunc;
}

void zCRnd_D3D::SetAlphaBlendFactor(const float& factor)
{
    alphaFactor = factor;
}

float zCRnd_D3D::GetAlphaBlendFactor() const
{
    return alphaFactor;
}

int zCRnd_D3D::SupportAlphaTesting()
{
    return xd3d_zwBufferLevel;
}

void zCRnd_D3D::SetRenderMode(zTRnd_RenderMode mode)
{
    renderMode = mode;
    if (mode != zRND_MODE_2_PASS_LIGHTMAP) {
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(13), 1);
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(14), 1);
        SetZBufferWriteEnabled(1);
        SetZBufferCompare(zRND_ZBUFFER_CMP_LESS);
        XD3D_SetRenderState(0x1b, 0);
        SetTextureStageState(0, zRND_TSS_COLOROP, 3);
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 3);
        SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(2), 1);
        renderMode = zRND_MODE_1_PASS_VERT_LIGHT;
        return;
    }
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(13), 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(14), 3);
    SetZBufferWriteEnabled(0);
    SetZBufferCompare(zRND_ZBUFFER_CMP_LESS_EQUAL);
    XD3D_SetRenderState(0x1b, 1);
    XD3D_SetRenderState(0x13, 9);
    XD3D_SetRenderState(0x14, 1);
    SetTextureStageState(0, zRND_TSS_COLOROP, 6);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(1), 3);
    SetTextureStageState(0, static_cast<zTRnd_TextureStageState>(2), 1);
}

zTRnd_RenderMode zCRnd_D3D::GetRenderMode() const
{
    return renderMode;
}

int zCRnd_D3D::HasCapability(zTRnd_Capability capability) const
{
    switch (capability) {
    case zRND_CAP_GUARD_BAND:
        return xd3d_Capabilities.guardband;
    case zRND_CAP_ALPHATEST:
        return xd3d_Capabilities.alphatest;
    case zRND_CAP_MAX_BLEND_STAGES:
        return xd3d_Capabilities.num_stages;
    case zRND_CAP_MAX_BLEND_STAGE_TEXTURES:
        if ((xd3d_vendorId == 0x1002 && xd3d_deviceId == 0x4742)
            || (xd3d_vendorId == 0x102b && xd3d_deviceId == 0x0521))
            return 1;
        return xd3d_Capabilities.num_texstages;
    case zRND_CAP_DIFFUSE_LAST_BLEND_STAGE_ONLY:
        return xd3d_vendorId == 0x121a && xd3d_deviceId == 5;
    case zRND_CAP_TNL:
        return xd3d_Capabilities.tnl;
    case zRND_CAP_TNL_HARDWARE:
        return xd3d_Capabilities.tnl_hardware;
    case zRND_CAP_TNL_MAXLIGHTS:
        return xd3d_Capabilities.tnl_maxlights > 0 ? xd3d_Capabilities.tnl_maxlights : 8;
    case zRND_CAP_DEPTH_BUFFER_PREC: {
        static const int precision[] = { -32, -24, -16, 32, 24, 16 };
        return xd3d_wBuffer >= 0 && xd3d_wBuffer < 6 ? precision[xd3d_wBuffer] : 0;
    }
    case zRND_CAP_BLENDDIFFUSEALPHA:
        return xd3d_Capabilities.blendDiffuseAlpha;
    }
    return 0;
}

void zCRnd_D3D::ResetZTest()
{
}

int zCRnd_D3D::HasPassedZTest()
{
    return 0;
}

int zCRnd_D3D::GetZBias() const
{
    return zbias;
}

void zCRnd_D3D::SetZBias(int bias)
{
    if (zbias == bias)
        return;
    XD3D_SetRenderState(0x2f, bias);
    zbias = bias;
}

unsigned short zCRnd_D3D::GetTexturePortableBinaryVersion()
{
    return 0;
}

void zCRnd_D3D::GetGuardBandBorders(float& left, float& right, float& top, float& bottom)
{
    if (!xd3d_Capabilities.guardband) {
        left = 0.0f;
        right = 0.0f;
        top = 0.0f;
        bottom = 0.0f;
        return;
    }
    left = xd3d_Capabilities.guardbandleft;
    right = xd3d_Capabilities.guardbandright;
    top = xd3d_Capabilities.guardbandtop;
    bottom = xd3d_Capabilities.guardbandbottom;
}

void zCRnd_D3D::GetStatistics(zTRnd_Stats& statistics)
{
    xd3d_statistics.texMemUsed = -1;
    xd3d_statistics.texMemFetched = -1;
    statistics = xd3d_statistics;
}

void zCRnd_D3D::ResetStatistics()
{
    std::memset(&xd3d_statistics, 0, sizeof(xd3d_statistics));
}

zCTexture* zCRnd_D3D::CreateTexture()
{
    zCTex_D3D* texture = new zCTex_D3D;
    zCClassDef::ObjectCreated(texture, &zCTexture::classDef);
    return texture;
}

int zCRnd_D3D::GetTotalTextureMem()
{
    return xd3d_Capabilities.vidmemfree;
}

int DXTCSupportSwitch;
int supportDXTC;
static int supportDXT3;

int zCRnd_D3D::SupportsTextureFormat(zTRnd_TextureFormat format)
{
    if (format == zRND_TEX_FORMAT_ARGB_4444 || format == zRND_TEX_FORMAT_ARGB_1555
        || format == zRND_TEX_FORMAT_RGB_565)
        return 1;
    if (format == zRND_TEX_FORMAT_DXT1 && DXTCSupportSwitch) {
        if (DXTCSupportSwitch == 1)
            return 1;
        if (DXTCSupportSwitch != 2)
            return supportDXTC;
        supportDXTC = 0;
        return 1;
    }
    if (format == zRND_TEX_FORMAT_DXT3 && DXTCSupportSwitch) {
        if (DXTCSupportSwitch == 1)
            return 1;
        if (DXTCSupportSwitch != 2)
            return supportDXT3;
        supportDXT3 = 0;
        return 1;
    }
    return 0;
}

int zCRnd_D3D::SupportsTextureFormatHardware(zTRnd_TextureFormat format)
{
    if (format == zRND_TEX_FORMAT_ARGB_4444 || format == zRND_TEX_FORMAT_ARGB_1555
        || format == zRND_TEX_FORMAT_RGB_565)
        return 1;
    if (format == zRND_TEX_FORMAT_DXT1)
        return supportDXTC;
    if (format == zRND_TEX_FORMAT_DXT3)
        return supportDXT3;
    return 0;
}

int zCRnd_D3D::GetMaxTextureSize()
{
    return xd3d_Capabilities.maxtexsize;
}

zCTextureConvert* zCRnd_D3D::CreateTextureConvert()
{
    return new zCTexConGeneric;
}
