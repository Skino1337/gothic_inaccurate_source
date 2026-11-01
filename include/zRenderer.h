#ifndef ZRENDERER_H
#define ZRENDERER_H

#include "z3d.h"

struct HWND__;
class zCVertexBuffer;

enum zTRnd_AlphaBlendFunc {
    zRND_ALPHA_FUNC_MAT_DEFAULT,
    zRND_ALPHA_FUNC_NONE,
    zRND_ALPHA_FUNC_BLEND,
    zRND_ALPHA_FUNC_ADD,
    zRND_ALPHA_FUNC_SUB,
    zRND_ALPHA_FUNC_MUL,
    zRND_ALPHA_FUNC_MUL2,
    zRND_ALPHA_FUNC_TEST,
    zRND_ALPHA_FUNC_BLEND_TEST
};

enum zTRnd_TextureFormat {
    zRND_TEX_FORMAT_ARGB_8888,
    zRND_TEX_FORMAT_ABGR_8888,
    zRND_TEX_FORMAT_RGBA_8888,
    zRND_TEX_FORMAT_BGRA_8888,
    zRND_TEX_FORMAT_RGB_888,
    zRND_TEX_FORMAT_BGR_888,
    zRND_TEX_FORMAT_ARGB_4444,
    zRND_TEX_FORMAT_ARGB_1555,
    zRND_TEX_FORMAT_RGB_565,
    zRND_TEX_FORMAT_PAL_8,
    zRND_TEX_FORMAT_DXT1,
    zRND_TEX_FORMAT_DXT2,
    zRND_TEX_FORMAT_DXT3,
    zRND_TEX_FORMAT_DXT4,
    zRND_TEX_FORMAT_DXT5,
    zRND_TEX_FORMAT_COUNT
};

struct zTRnd_TexFormatInfo {
    float bytesPerPixel;
    int   rPos;
    int   gPos;
    int   bPos;
    int   aPos;
    int   rSize;
    int   gSize;
    int   bSize;
    int   aSize;
};

struct zTRnd_Stats {
    int texMemUsed;
    int texMemFetched;
    int numTexturesFetched;
    int numTexturesUsed;
    int numLightmapsUsed;
    int numPolysRendered;
    int numTrisRendered;
    int numAlphaPolys;
    int numAlphaPolyBatches;
};

struct zTRnd_VidModeInfo {
    int xdim;
    int ydim;
    int bpp;
    int fullscreenOnly;
};

struct zTRnd_DeviceInfo {
    zTRnd_DeviceInfo();
    ~zTRnd_DeviceInfo() {}

    zSTRING deviceName;
};

struct zTRndSurfaceDesc {
    void* pSurface;
    unsigned long bytePitch;
    unsigned long pixelWidth;
    unsigned long pixelHeight;
    unsigned long bitRGBCount;
    unsigned long bitRMask;
    unsigned long bitGMask;
    unsigned long bitBMask;
};

struct tagRECT {
    long left;
    long top;
    long right;
    long bottom;
};

enum zTRnd_ScreenMode {
    zRND_SCRMODE_FULLSCREEN,
    zRND_SCRMODE_WINDOWED,
    zRND_SCRMODE_HIDE
};

struct zTRndSimpleVertex {
    zTRndSimpleVertex();

    zVEC2         pos;
    float         z;
    zVEC2         uv;
    unsigned long color;
};

class zCTextureConvert {
};

class zCRenderLight;
class zCRndAlphaSortObject;

enum zTRnd_TrafoType {
    zRND_TRAFO_VIEW,
    zRND_TRAFO_PROJECTION,
    zRND_TRAFO_TEXTURE0,
    zRND_TRAFO_TEXTURE1,
    zRND_TRAFO_TEXTURE2,
    zRND_TRAFO_TEXTURE3
};

enum zTRnd_TextureStageState {
    zRND_TSS_COLOROP,
    zRND_TSS_COUNT
};

enum zTRnd_RenderStateType {
    zRND_RENDERSTATE_CLIPPING,
    zRND_RENDERSTATE_TEXTUREFACTOR,
    zRND_RENDERSTATE_COUNT
};

enum zTRnd_FogMode {
    zRND_FOGMODE_LINEAR
};

enum zTRnd_PolyDrawMode {
    zRND_DRAW_MATERIAL,
    zRND_DRAW_MATERIAL_WIRE,
    zRND_DRAW_FLAT,
    zRND_DRAW_WIRE
};

enum zTRnd_PolySortMode {
    zRND_POLYSORT_NONE
};

enum zTRnd_ZBufferCmp {
    zRND_ZBUFFER_CMP_ALWAYS,
    zRND_ZBUFFER_CMP_NEVER,
    zRND_ZBUFFER_CMP_LESS,
    zRND_ZBUFFER_CMP_LESS_EQUAL
};

enum zTRnd_RenderMode {
    zRND_MODE_1_PASS_VERT_LIGHT,
    zRND_MODE_1_PASS_MULTITEXTURE,
    zRND_MODE_2_PASS_TEXTURE,
    zRND_MODE_2_PASS_LIGHTMAP,
    zRND_MODE_DYN_POLYS_VERT_LIGHT
};

enum zTRnd_AlphaBlendSource {
    zRND_ALPHA_SOURCE_MATERIAL,
    zRND_ALPHA_SOURCE_CONSTANT
};

enum zTRnd_Capability {
    zRND_CAP_GUARD_BAND,
    zRND_CAP_ALPHATEST,
    zRND_CAP_MAX_BLEND_STAGES,
    zRND_CAP_MAX_BLEND_STAGE_TEXTURES,
    zRND_CAP_DIFFUSE_LAST_BLEND_STAGE_ONLY,
    zRND_CAP_TNL,
    zRND_CAP_TNL_HARDWARE,
    zRND_CAP_TNL_MAXLIGHTS,
    zRND_CAP_DEPTH_BUFFER_PREC,
    zRND_CAP_BLENDDIFFUSEALPHA
};

class zCRenderer {
public:

    struct zTMaterial {
        zVEC4 diffuseRGBA;
        zVEC4 ambientRGBA;
    };

    virtual ~zCRenderer();
    zTRnd_AlphaBlendFunc AlphaBlendFuncStringToType(const zSTRING& value) const;
    zSTRING              AlphaBlendFuncTypeToString(zTRnd_AlphaBlendFunc value) const;
    int                  Vid_SetMode(int xdim, int ydim, int bpp, HWND__** window);
    zTRnd_TexFormatInfo  GetTexFormatInfo(zTRnd_TextureFormat format) const;
    void                 DrawTile(zCTexture* texture, const zVEC2& topLeft,
        const zVEC2& bottomRight, float z, const zVEC2& uvTopLeft,
        const zVEC2& uvBottomRight, unsigned long color);

    virtual zCTexture* CreateTexture();
    virtual zCVertexBuffer* CreateVertexBuffer();
    virtual int  SupportsTextureFormat(zTRnd_TextureFormat format);
    virtual int  SupportsTextureFormatHardware(zTRnd_TextureFormat format);
    virtual int  GetMaxTextureSize();
    virtual int  HasCapability(zTRnd_Capability capability) const;
    virtual int  HasPassedZTest();
    virtual int  Vid_GetNumModes();
    virtual int  Vid_GetActiveModeNr();
    virtual void Vid_GetModeInfo(zTRnd_VidModeInfo& info, int index);
    virtual int  Vid_SetMode(int index, HWND__** window);
    virtual int  Vid_GetScreenMode() const;
    virtual void Vid_SetScreenMode(int mode);
    virtual void Vid_SetGammaCorrection(float contrast, float gamma, float brightness);
    virtual void DrawPolySimple(zCTexture* texture, zTRndSimpleVertex* vertices, int numVertices);
    virtual void DrawPoly(zCPolygon* polygon);
    virtual void DrawLightmapList(zCPolygon** polygons, int numPolygons);
    virtual void DrawLine(float x1, float y1, float x2, float y2, zCOLOR color);
    virtual void DrawLineZ(float x1, float y1, float z1, float x2, float y2, float z2,
        zCOLOR color);
    virtual int  GetFog() const;
    virtual int  GetRadialFog() const;
    virtual zCOLOR GetFogColor() const;
    virtual void GetFogRange(float& nearZ, float& farZ, zTRnd_FogMode& mode);
    virtual void SetFogRange(float nearZ, float farZ, int mode);
    virtual void SetFog(int enabled);
    virtual void SetRadialFog(int enabled);
    virtual void SetFogColor(const zCOLOR& color);
    virtual void BeginFrame();
    virtual void EndFrame();
    virtual int  Vid_Lock(zTRndSurfaceDesc& surface);
    virtual int  Vid_Unlock();
    virtual void Vid_Blit(int flipX, void* source, void* destination);
    virtual void Vid_Clear(zCOLOR& color, int flags);
    virtual zTRnd_PolyDrawMode GetPolyDrawMode() const;
    virtual void SetPolyDrawMode(const zTRnd_PolyDrawMode& mode);
    virtual int  GetSurfaceLost() const;
    virtual void SetSurfaceLost(int lost);
    virtual int  GetSyncOnAmbientCol() const;
    virtual void SetSyncOnAmbientCol(int enabled);
    virtual void SetTextureWrapEnabled(int enabled);
    virtual int  GetTextureWrapEnabled() const;
    virtual void SetDitherEnabled(int enabled);
    virtual int  GetDitherEnabled() const;
    virtual zTRnd_PolySortMode GetPolySortMode() const;
    virtual void SetPolySortMode(const zTRnd_PolySortMode& mode);
    virtual int  GetZBufferWriteEnabled() const;
    virtual void SetZBufferWriteEnabled(int enabled);
    virtual void SetZBias(int bias);
    virtual int  GetZBias() const;
    virtual zTRnd_ZBufferCmp GetZBufferCompare();
    virtual void SetZBufferCompare(const zTRnd_ZBufferCmp& compare);
    virtual int  GetPixelWriteEnabled() const;
    virtual void SetPixelWriteEnabled(int enabled);
    virtual void SetAlphaBlendSource(const zTRnd_AlphaBlendSource& source);
    virtual zTRnd_AlphaBlendSource GetAlphaBlendSource() const;
    virtual void SetAlphaBlendFunc(const zTRnd_AlphaBlendFunc& func);
    virtual zTRnd_AlphaBlendFunc GetAlphaBlendFunc() const;
    virtual float GetAlphaBlendFactor() const;
    virtual void SetAlphaBlendFactor(const float& factor);
    virtual void SetAlphaReference(unsigned long reference);
    virtual unsigned long GetAlphaReference() const;
    virtual int  GetCacheAlphaPolys() const;
    virtual void SetCacheAlphaPolys(int enabled);
    virtual int  GetAlphaLimitReached() const;
    virtual void AddAlphaPoly(const zCPolygon* polygon);
    virtual void FlushAlphaPolys();
    virtual void SetRenderMode(zTRnd_RenderMode mode);
    virtual zTRnd_RenderMode GetRenderMode() const;
    virtual void GetGuardBandBorders(float& left, float& right, float& top, float& bottom);
    virtual void ResetZTest();
    virtual zCTextureConvert* CreateTextureConvert();
    virtual int  SetTransform(zTRnd_TrafoType type, const zMAT4& trafo);
    virtual int  SetViewport(int x, int y, int width, int height);
    virtual int  SetLight(unsigned long index, zCRenderLight* light);
    virtual int  GetMaterial(zTMaterial& material);
    virtual int  SetMaterial(const zTMaterial& material);
    virtual int  SetTexture(unsigned long stage, zCTexture* texture);
    virtual int  SetTextureStageState(unsigned long stage, zTRnd_TextureStageState state,
        unsigned long value);
    virtual int  SetAlphaBlendFuncImmed(zTRnd_AlphaBlendFunc func);
    virtual int  SetRenderState(zTRnd_RenderStateType state, unsigned long value);
    virtual unsigned long GetRenderState(zTRnd_RenderStateType state);
    virtual void AddAlphaSortObject(zCRndAlphaSortObject* object);
    virtual void RenderAlphaSortList();
    virtual unsigned long GetTotalTextureMem() const;
    virtual void SetBilerpFilterEnabled(int enabled);
    virtual int GetBilerpFilterEnabled() const;

    int vid_xdim;
    int vid_ydim;
    int vid_bpp;
    int fog;
    int radialFog;
    zCOLOR fogColor;
    float fogNear;
    float fogFar;
    zTRnd_FogMode fogMode;
    zTRnd_PolyDrawMode polyDrawMode;
    int surfaceLost;
    int syncAmbient;
    int textureWrap;
    int dither;
    zTRnd_PolySortMode polySortMode;
    int zwrite;
    int zbias;
    zTRnd_ZBufferCmp zfunc;
    int pixelWrite;
    zTRnd_AlphaBlendSource alphaSource;
    zTRnd_AlphaBlendFunc alphaFunc;
    float alphaFactor;
    unsigned long alphaReference;
    int cacheAlphaPolys;
    int alphaLimitReached;
    zCArray<const zCPolygon*> alphaPolys;
    zTRnd_RenderMode renderMode;
    float guardLeft;
    float guardRight;
    float guardTop;
    float guardBottom;
    int zTestPassed;
};

extern zCRenderer* zrenderer;
extern zTRnd_TexFormatInfo texFormatInfoList[zRND_TEX_FORMAT_COUNT];

#endif
