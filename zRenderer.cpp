#include "include/zRenderer.h"

void zCRenderer::DrawLightmapList(zCPolygon**, int)
{
}

void zCRenderer::DrawLineZ(float, float, float, float, float, float, zCOLOR)
{
}

void zCRenderer::DrawPolySimple(zCTexture*, zTRndSimpleVertex*, int)
{
}

int zCRenderer::GetFog() const
{
    return fog;
}

int zCRenderer::GetRadialFog() const
{
    return radialFog;
}

zCOLOR zCRenderer::GetFogColor() const
{
    return fogColor;
}

void zCRenderer::GetFogRange(float& nearZ, float& farZ, zTRnd_FogMode& mode)
{
    nearZ = fogNear;
    farZ = fogFar;
    mode = fogMode;
}

zTRnd_PolyDrawMode zCRenderer::GetPolyDrawMode() const
{
    return polyDrawMode;
}

void zCRenderer::SetPolyDrawMode(const zTRnd_PolyDrawMode& mode)
{
    polyDrawMode = mode;
}

int zCRenderer::GetSurfaceLost() const
{
    return surfaceLost;
}

void zCRenderer::SetSurfaceLost(int lost)
{
    surfaceLost = lost;
}

int zCRenderer::GetSyncOnAmbientCol() const
{
    return syncAmbient;
}

void zCRenderer::SetSyncOnAmbientCol(int enabled)
{
    syncAmbient = enabled;
}

void zCRenderer::SetTextureWrapEnabled(int enabled)
{
    textureWrap = enabled;
}

int zCRenderer::GetTextureWrapEnabled() const
{
    return textureWrap;
}

void zCRenderer::SetDitherEnabled(int enabled)
{
    dither = enabled;
}

int zCRenderer::GetDitherEnabled() const
{
    return dither;
}

zTRnd_PolySortMode zCRenderer::GetPolySortMode() const
{
    return polySortMode;
}

void zCRenderer::SetPolySortMode(const zTRnd_PolySortMode& mode)
{
    polySortMode = mode;
}

int zCRenderer::GetZBufferWriteEnabled() const
{
    return zwrite;
}

void zCRenderer::SetZBufferWriteEnabled(int enabled)
{
    zwrite = enabled;
}

void zCRenderer::SetZBias(int bias)
{
    zbias = bias;
}

int zCRenderer::GetZBias() const
{
    return zbias;
}

zTRnd_ZBufferCmp zCRenderer::GetZBufferCompare()
{
    return zfunc;
}

void zCRenderer::SetZBufferCompare(const zTRnd_ZBufferCmp& compare)
{
    zfunc = compare;
}

int zCRenderer::GetPixelWriteEnabled() const
{
    return pixelWrite;
}

void zCRenderer::SetPixelWriteEnabled(int enabled)
{
    pixelWrite = enabled;
}

void zCRenderer::SetAlphaBlendSource(const zTRnd_AlphaBlendSource& source)
{
    alphaSource = source;
}

zTRnd_AlphaBlendSource zCRenderer::GetAlphaBlendSource() const
{
    return alphaSource;
}

void zCRenderer::SetAlphaBlendFunc(const zTRnd_AlphaBlendFunc& func)
{
    alphaFunc = func;
}

zTRnd_AlphaBlendFunc zCRenderer::GetAlphaBlendFunc() const
{
    return alphaFunc;
}

float zCRenderer::GetAlphaBlendFactor() const
{
    return alphaFactor;
}

void zCRenderer::SetAlphaBlendFactor(const float& factor)
{
    alphaFactor = factor;
}

void zCRenderer::SetAlphaReference(unsigned long reference)
{
    alphaReference = reference;
}

unsigned long zCRenderer::GetAlphaReference() const
{
    return alphaReference;
}

int zCRenderer::GetCacheAlphaPolys() const
{
    return cacheAlphaPolys;
}

void zCRenderer::SetCacheAlphaPolys(int enabled)
{
    cacheAlphaPolys = enabled;
}

int zCRenderer::GetAlphaLimitReached() const
{
    return alphaLimitReached;
}

void zCRenderer::AddAlphaPoly(const zCPolygon* polygon)
{
    alphaPolys.InsertEnd(polygon);
}

void zCRenderer::FlushAlphaPolys()
{
    alphaPolys.numInArray = 0;
}

void zCRenderer::SetRenderMode(zTRnd_RenderMode mode)
{
    renderMode = mode;
}

zTRnd_RenderMode zCRenderer::GetRenderMode() const
{
    return renderMode;
}

int zCRenderer::HasCapability(zTRnd_Capability) const
{
    return 0;
}

void zCRenderer::GetGuardBandBorders(float& left, float& right, float& top, float& bottom)
{
    left = guardLeft;
    right = guardRight;
    top = guardTop;
    bottom = guardBottom;
}

void zCRenderer::ResetZTest()
{
    zTestPassed = 1;
}

int zCRenderer::HasPassedZTest()
{
    return zTestPassed;
}

zCTextureConvert* zCRenderer::CreateTextureConvert()
{
    return new zCTextureConvert;
}

int zCRenderer::SupportsTextureFormat(zTRnd_TextureFormat)
{
    return 0;
}

int zCRenderer::SupportsTextureFormatHardware(zTRnd_TextureFormat)
{
    return 0;
}

int zCRenderer::GetMaxTextureSize()
{
    return 256;
}

int zCRenderer::SetTransform(zTRnd_TrafoType, const zMAT4&)
{
    return 0;
}

int zCRenderer::SetViewport(int, int, int, int)
{
    return 0;
}

int zCRenderer::SetLight(unsigned long, zCRenderLight*)
{
    return 0;
}

int zCRenderer::GetMaterial(zTMaterial&)
{
    return 0;
}

int zCRenderer::SetMaterial(const zTMaterial&)
{
    return 0;
}

int zCRenderer::SetTexture(unsigned long, zCTexture*)
{
    return 0;
}

int zCRenderer::SetTextureStageState(unsigned long, zTRnd_TextureStageState, unsigned long)
{
    return 0;
}

int zCRenderer::SetAlphaBlendFuncImmed(zTRnd_AlphaBlendFunc)
{
    return 0;
}

int zCRenderer::SetRenderState(zTRnd_RenderStateType, unsigned long)
{
    return 0;
}

unsigned long zCRenderer::GetRenderState(zTRnd_RenderStateType)
{
    return 0;
}

void zCRenderer::AddAlphaSortObject(zCRndAlphaSortObject*)
{
}

void zCRenderer::RenderAlphaSortList()
{
}
