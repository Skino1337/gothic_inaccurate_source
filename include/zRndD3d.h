#ifndef ZRNDD3D_H
#define ZRNDD3D_H

#include "zRenderer.h"

struct IDirectDrawSurface7;
struct _GUID;
struct HMONITOR__;

class zCDXTCCache_D3D {
public:

    struct zD3D_Entry {
        IDirectDrawSurface7* surface;
        int locked;
        zD3D_Entry* nextEntry;
    };

    struct zD3D_SlotHeader {
        zD3D_Entry* firstEntry;
        zD3D_Entry* lastEntry;
    };

    zCDXTCCache_D3D();
    ~zCDXTCCache_D3D();
    IDirectDrawSurface7* Lock(int xdim, int ydim, int format);
    int Unlock(int xdim, int ydim, int format, IDirectDrawSurface7* surface);
    void ClearCache();
    IDirectDrawSurface7* XDXTC_CreateSurface(int xdim, int ydim, int format, zD3D_Entry* entry);

    int xdxtc_cacheentryCounter;
    zD3D_SlotHeader xdxtc_slot[2][12][12];
};

class zCRndAlphaSortObject {
public:

    virtual ~zCRndAlphaSortObject();
    virtual void Draw(int flags);
    virtual int IsAlphaPoly();

    zCRndAlphaSortObject* nextSortObject;
    float zvalue;
};

class zCTex_D3D;

class zD3D_alphaPoly : public zCRndAlphaSortObject {
public:

    struct zD3D_vertexFormat {
        float a_sx;
        float a_sy;
        float a_sz;
        float a_rhw;
        unsigned long a_color;
        float a_tu;
        float a_tv;
    };

    zD3D_alphaPoly();
    virtual ~zD3D_alphaPoly();
    virtual int IsAlphaPoly();
    virtual void Draw(int flags);

    zTRnd_AlphaBlendFunc alphafunc;
    zCTex_D3D* tex;
    int wrap;
    unsigned long zfunc;
    int numVert;
    int zBias;
    zD3D_vertexFormat vertexList[8];

    static zCVertexBufferDyn* vertexbufferdyn;
    static zCVertexBuffer* vertexbuffer;
    static int locked;
    static unsigned long startvertex;
    static unsigned long actvertex;
    static int numberofvertices;
    static int actnov;
};

struct zD3D_savedTexture;

class zCTex_D3D : public zCTexture {
public:

    zCTex_D3D();
    virtual ~zCTex_D3D();
    virtual void ReleaseData();
    virtual int Lock(int mode);
    virtual int Unlock();
    virtual int SetTextureInfo(const zCTextureInfo& info);
    virtual void* GetPaletteBuffer();
    virtual int GetTextureBuffer(int mipMap, void*& data, int& pitch);
    virtual zCTextureInfo GetTextureInfo();
    virtual int CopyPaletteDataTo(void* destination);
    virtual int CopyTextureDataTo(int mipMap, void* destination, int pitch);
    void GetPixelSize(int& sizeX, int& sizeY);
    int XTEX_BuildSurfaces(int flags);
    void InsertTexture(int stage, int flags);
    int PreloadTexture();
    int SetTexture(int stage);
    int RemoveTexture();
    int RescueTexture(zD3D_savedTexture* saved);
    int RebuildTexture(zD3D_savedTexture* saved);

    unsigned int xtex_textureflag;
    int xtex_locked;
    unsigned int xtex_miplocked;
    zCTextureInfo xtex_texinfo;
    unsigned char* xtex_pPalettePtr;
    int xtex_internalnumber;
    int xtex_alphaTex;
    IDirectDrawSurface7* xtex_pddtex[12];
    IDirectDrawSurface7* xtex_pddtemporarytex[12];
    unsigned long xtex_lastFrameUsed;
    void* xtex_buffer;
    unsigned char* xtex_buffers[12];
    int xtex_pitches[12];
};

struct zD3D_savedTexture {
    zCTex_D3D* texture;
    zCTextureConvert* texCon;
    zD3D_savedTexture* next;
};

class zCRnd_D3D : public zCRenderer {
public:

    zCRnd_D3D();
    virtual ~zCRnd_D3D();
    void EmergencyExit();
    int GetRadialFog() const;
    void SetAlphaReference(unsigned long reference);
    unsigned long GetAlphaReference() const;
    int GetAlphaLimitReached() const;
    int GetCacheAlphaPolys() const;
    void SetCacheAlphaPolys(int enabled);
    int GetSurfaceLost() const;
    void SetSurfaceLost(int lost);
    int GetSyncOnAmbientCol() const;
    void SetSyncOnAmbientCol(int enabled);
    int Vid_IsLocked();
    void Vid_EndLfbAccess();
    void Vid_SetLfbAlpha(int alpha);
    void Vid_SetLfbAlphaFunc(const zTRnd_AlphaBlendFunc& func);
    int XD3D_EnumerateModes();
    int XD3D_InitPerDX(long device, int xdim, int ydim, int bpp, int flags);
    int XD3D_InitPort(int device, int xdim, int ydim, int bpp, int flags);
    int XD3D_TestCapabilities();
    int XD3D_SetDevice(int device, int xdim, int ydim, int bpp, int screenMode);
    void XD3D_ClearDevice();
    int XD3D_TextureRescue(zCTex_D3D* texture);
    int XD3D_TextureRebuild();
    int XD3D_CreateWin(int xdim, int ydim, int bpp);
    void XD3D_CloseWin();
    int XD3D_SetRenderState(int state, unsigned long value);
    void DrawPolySimple(zCTexture* texture, zTRndSimpleVertex* vertices, int numVertices);
    void DrawPoly(zCPolygon* polygon);
    void XD3D_PolyDrawOnePassVertLight(zCPolygon* polygon);
    void XD3D_PolyDrawOnePassMultiTex(zCPolygon* polygon);
    void XD3D_PolyDrawTwoPassTexture(zCPolygon* polygon);
    void XD3D_PolyDrawTwoPassLightMap(zCPolygon* polygon);
    void XD3D_AlphaTestPoly(zCPolygon* polygon);
    void XD3D_AlphaBlendPoly(zCPolygon* polygon, zCMaterial* material);
    void AddAlphaPoly(const zCPolygon* polygon);
    void FlushAlphaPolys();
    void DrawLightmapList(zCPolygon** polygons, int numPolygons);
    void SetPixel(float x, float y, zCOLOR color);
    void DrawLine(float x1, float y1, float x2, float y2, zCOLOR color);
    void DrawLineZ(float x1, float y1, float z1, float x2, float y2, float z2, zCOLOR color);
    void FlushPolys();
    void BeginFrame();
    void EndFrame();
    int SetTransform(zTRnd_TrafoType type, const zMAT4& trafo);
    int SetViewport(int x, int y, int width, int height);
    int SetLight(unsigned long index, zCRenderLight* light);
    int SetMaterial(const zTMaterial& material);
    int GetMaterial(zTMaterial& material);
    int XD3D_SetTexture(unsigned long stage, IDirectDrawSurface7* surface);
    int SetTexture(unsigned long stage, zCTexture* texture);
    int SetTextureStageState(unsigned long stage, zTRnd_TextureStageState state,
        unsigned long value);
    int SetAlphaBlendFuncImmed(zTRnd_AlphaBlendFunc func);
    int SetRenderState(zTRnd_RenderStateType state, unsigned long value);
    unsigned long GetRenderState(zTRnd_RenderStateType state);
    void AddAlphaSortObject(zCRndAlphaSortObject* object);
    void RenderAlphaSortList();
    int XD3D_DrawVertexBufferWire(zCVertexBuffer* buffer, int firstVertex, int numVertices,
        unsigned short* indices, unsigned long numIndices);
    int XD3D_DrawVertexBufferFlat(zCVertexBuffer* buffer, int firstVertex, int numVertices,
        unsigned short* indices, unsigned long numIndices);
    int DrawVertexBuffer(zCVertexBuffer* buffer, int firstVertex, int numVertices,
        unsigned short* indices, unsigned long numIndices);
    void Vid_BeginLfbAccess();
    zCVertexBuffer* CreateVertexBuffer();
    void SetFog(int enabled);
    void SetRadialFog(int enabled);
    int GetFog() const;
    void SetFogColor(const zCOLOR& color);
    void SetFogType(int type);
    zCOLOR GetFogColor() const;
    void SetFogRange(float nearZ, float farZ, zTRnd_FogMode mode);
    void GetFogRange(float& nearZ, float& farZ, zTRnd_FogMode& mode);
    void SetPolyDrawMode(const zTRnd_PolyDrawMode& mode);
    zTRnd_PolyDrawMode GetPolyDrawMode() const;
    void SetTextureWrapEnabled(int enabled);
    int GetTextureWrapEnabled() const;
    void SetBilerpFilterEnabled(int enabled);
    int GetBilerpFilterEnabled() const;
    void SetDitherEnabled(int enabled);
    int GetDitherEnabled() const;
    void SetPolySortMode(const zTRnd_PolySortMode& mode);
    zTRnd_PolySortMode GetPolySortMode() const;
    void SetZBufferWriteEnabled(int enabled);
    int GetZBufferWriteEnabled() const;
    void SetZBufferCompare(const zTRnd_ZBufferCmp& compare);
    zTRnd_ZBufferCmp GetZBufferCompare();
    void SetPixelWriteEnabled(int enabled);
    int GetPixelWriteEnabled() const;
    void SetAlphaBlendSource(const zTRnd_AlphaBlendSource& source);
    zTRnd_AlphaBlendSource GetAlphaBlendSource() const;
    void SetAlphaBlendFunc(const zTRnd_AlphaBlendFunc& func);
    zTRnd_AlphaBlendFunc GetAlphaBlendFunc() const;
    void SetAlphaBlendFactor(const float& factor);
    float GetAlphaBlendFactor() const;
    int SupportAlphaTesting();
    void SetRenderMode(zTRnd_RenderMode mode);
    zTRnd_RenderMode GetRenderMode() const;
    int HasCapability(zTRnd_Capability capability) const;
    void ResetZTest();
    int HasPassedZTest();
    int GetZBias() const;
    void SetZBias(int bias);
    unsigned short GetTexturePortableBinaryVersion();
    void GetGuardBandBorders(float& left, float& right, float& top, float& bottom);
    void GetStatistics(zTRnd_Stats& statistics);
    void ResetStatistics();
    zCTexture* CreateTexture();
    int GetTotalTextureMem();
    int SupportsTextureFormat(zTRnd_TextureFormat format);
    int SupportsTextureFormatHardware(zTRnd_TextureFormat format);
    int GetMaxTextureSize();
    zCTextureConvert* CreateTextureConvert();
    void Vid_Blit(int flags, tagRECT* source, tagRECT* destination);
    int Vid_Lock(zTRndSurfaceDesc& surface);
    int Vid_Unlock();
    int Vid_GetFrontBufferCopy(zCTextureConvert& texture);
    void Vid_Clear(zCOLOR& color, int flags);
    int Vid_GetNumDevices();
    int Vid_GetActiveDeviceNr();
    int Vid_SetDevice(int device);
    int Vid_GetDeviceInfo(zTRnd_DeviceInfo& info, int device);
    int Vid_GetNumModes();
    int Vid_GetModeInfo(zTRnd_VidModeInfo& info, int mode);
    int Vid_GetActiveModeNr();
    int Vid_SetMode(int mode, HWND__** window);
    int XD3D_SetMode(int xdim, int ydim, int bpp);
    void Vid_SetScreenMode(zTRnd_ScreenMode mode);
    zTRnd_ScreenMode Vid_GetScreenMode();
    void Vid_SetGammaCorrection(float contrast, float gamma, float brightness);
    float Vid_GetGammaCorrection();

    int fogType;
    struct zTActStatus {
        int modeNr;
        int devnum;
        int devmax;
        float gammavalue;
        int fog;
        int zwrite;
        int zfunc;
        int zd3dfunc;
        int dither;
        int texwrap;
        int filter;
    } xd3d_actStatus;

    struct zTCapabilities {
        unsigned long vidmemfree;
        unsigned long maxtexsize;
        int window;
        int gamma;
        int guardband;
        float guardbandleft;
        float guardbandright;
        float guardbandtop;
        float guardbandbottom;
        int alphatest;
        int num_stages;
        int num_texstages;
        int tnl;
        int tnl_hardware;
        int tnl_maxlights;
        int blendDiffuseAlpha;
    } xd3d_Capabilities;

    zTRnd_Stats xd3d_statistics;

    unsigned long xd3d_actRenderState[160];
    unsigned long xd3d_actTexStageState[4][24];
    zTMaterial xd3d_actmaterial;
    zCTexture* xd3d_ptexture[4];
    IDirectDrawSurface7* xd3d_pd3dtexture[4];
    unsigned long xd3d_texturefactor;
    int xd3d_vbclipping;
    zCRndAlphaSortObject* xd3d_alphaSortBucket[256];
    zMAT4 xd3d_transform[6];
    float xd3d_zMIN_from_Engine;
    float xd3d_zMAX_from_Engine;
    float xd3d_scale_A;
    float xd3d_scale_B;
    float xd3d_scale_C;
    int xd3d_numLightmapsUsedThisFrame;
    int xd3d_numAlphaPolys;
    int xd3d_inScene;
    int xd3d_backlocked;
    int xd3d_zwBufferLevel;
    int xd3d_wBuffer;
    unsigned long xd3d_vendorId;
    unsigned long xd3d_deviceId;
    int m_bAnisotropicFilter;
    int xd3d_disableFog;
    float xd3d_bucketSize;
    int xd3d_scrWidth;
    int xd3d_scrHeight;
    int xd3d_scrStatus;
    int xd3d_scrMode;
    int xd3d_alphaBlendImmed;
    int xd3d_num;
    IDirectDrawSurface7* xd3d_pfrontBuffer;
    IDirectDrawSurface7* xd3d_pbackBuffer;
    unsigned char* xd3d_frontPixels;
    int xd3d_frontPitch;
    int xd3d_tryDevnum;
    zD3D_savedTexture* xd3d_numTexturesUsedThisFrame;
    int xd3d_scrBpp;
    int xd3d_deviceChange;
    void* xd3d_newWinHandle;

    static ID3DXContext* xd3d_pd3dx;
    static IDirectDraw7* xd3d_pdd7;
    static IDirect3D7* xd3d_pd3d7;
    static IDirect3DDevice7* xd3d_pd3dDevice7;
    static unsigned long xd3d_actFrameNumber;
    static int xd3d_dd7inuse;
    static int xd3d_palsupp;
};

class zCSurfaceCache_D3D {
public:

    struct zCSlotIndex {
        int dynamic;
        int pixelformat;
        int mipmaps;
        int width;
        int height;
    };

    struct zD3D_Entry {
        IDirectDrawSurface7* surface;
        unsigned long frameNumber;
        zD3D_Entry* nextEntry;
    };

    struct zD3D_SlotHeader {
        zD3D_Entry* firstEntry;
        zD3D_Entry* lastEntry;
    };

    zCSurfaceCache_D3D();
    ~zCSurfaceCache_D3D();
    int CacheInSurface(IDirectDrawSurface7* surface, zCSlotIndex* slotIndex);
    IDirectDrawSurface7* CacheOutSurface(zCSlotIndex* slotIndex);
    void ClearCache();
    void XSC_ClearSlot(zD3D_SlotHeader* slot);
    void XSC_KillLastSurfaceInSlot(zD3D_SlotHeader* slot);

    int xsc_cacheentryCounter;
    zD3D_SlotHeader xsc_slot[2][6][12][12][12];

    static zD3D_SlotHeader* xsc_listofsurfaces[128];
    static int xsc_listposition;

    static void ResetSurfaceList();
};

int XD3D_AppIDDEnumCallbackEx(_GUID* guid, char* driver, char* description, void* context,
    HMONITOR__* monitor);
int DXTryWarning(long result, const char* message);

#endif
