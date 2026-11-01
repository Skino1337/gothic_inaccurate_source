#ifndef ZTEXTURE_H
#define ZTEXTURE_H

#include "zResource.h"
#include "zRenderer.h"
#include "zFile.h"
#include "zModel.h"

class zCTextureExchange;

enum zTTexScaleSize {
    zTEX_SCALE_SIZE_XY_ONE_LOWER,
    zTEX_SCALE_SIZE_X_ONE_LOWER,
    zTEX_SCALE_SIZE_REFERENCE,
    zTEX_SCALE_SIZE_Y_ONE_HIGHER,
    zTEX_SCALE_SIZE_X_ONE_HIGHER,
    zTEX_SCALE_SIZE_XY_ONE_HIGHER,
    zTEX_SCALE_SIZE_XY_MAX,
    zTEX_SCALE_SIZE_COUNT
};

enum zTTexScaleBPP {
    zTEX_SCALE_BPP_S3TC,
    zTEX_SCALE_BPP_REFERENCE,
    zTEX_SCALE_BPP_ONE_HIGHER,
    zTEX_SCALE_BPP_MAX
};

class zCTextureInfo {
public:

    zTRnd_TextureFormat texFormat;
    int                 sizeX;
    int                 sizeY;
    int                 numMipMap;
    int                 refSizeX;
    int                 refSizeY;
    unsigned long       averageColor;
};

class zCTextureFileFormat : public zCObject {
public:

    virtual ~zCTextureFileFormat();

    zCClassDef* _GetClassDef() const;
    virtual int LoadTexture(const zSTRING& fileName, zCTextureExchange* texture) = 0;
    virtual int LoadTexture(zFILE& file, zCTextureExchange* texture) = 0;
    virtual int SaveTexture(const zSTRING& fileName, zCTextureExchange* texture) = 0;
    virtual int SaveTexture(zFILE& file, zCTextureExchange* texture) = 0;
    virtual const zSTRING& GetFileExtension() const = 0;
    virtual int CanSave() const = 0;
    virtual int CanLoad() const = 0;

    static zCClassDef* classDef;
};

class zCTextureFileFormatTGA : public zCTextureFileFormat {
public:

#pragma pack(push, 1)
    struct zTTgaHeader {
        unsigned char  IDLength;
        unsigned char  CMapType;
        unsigned char  ImgType;
        unsigned char  CMapStartLo;
        unsigned char  CMapStartHi;
        unsigned char  CMapLengthLo;
        unsigned char  CMapLengthHi;
        unsigned char  CMapDepth;
        unsigned short XOffSet;
        unsigned short YOffSet;
        unsigned char  WidthLo;
        unsigned char  WidthHi;
        unsigned char  HeightLo;
        unsigned char  HeightHi;
        unsigned char  PixelDepth;
        unsigned char  ImageDescriptor;
    };
#pragma pack(pop)

    zCTextureFileFormatTGA();
    virtual ~zCTextureFileFormatTGA();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    int              LoadTexture(const zSTRING& fileName, zCTextureExchange* texture);
    int              LoadTexture(zFILE& file, zCTextureExchange* texture);
    int              SaveTexture(const zSTRING& fileName, zCTextureExchange* texture);
    int              SaveTexture(zFILE& file, zCTextureExchange* texture);
    const zSTRING&   GetFileExtension() const;
    int              CanSave() const;
    int              CanLoad() const;
    int              ReadTGAHeader(zFILE* file);
    int              ReadTGAColorMap(zFILE* file);
    int              ReadTGARLEPixel(zFILE* file, unsigned char* destination, int bytesPerPixel);
    int              ReadTGASpan(zFILE* file, unsigned char* destination, int pixelCount,
        int bytesPerPixel);
    int              ReadTGAData(zFILE* file);

    zTTgaHeader        tgaHeader;
    zTRnd_TextureFormat texFormat;
    zCTextureInfo      texInfo;
    unsigned char*     palette;
    unsigned char*     data;
    unsigned char*     dataBase;
    int                pitchXBytes;
    int                tgaRLE;
    int                tgaRLEflag;
    int                tgaRLEcount;
    int                tgaRLEsav[4];

    static zCClassDef* classDef;
};

class zCTextureFileFormatInternal : public zCTextureFileFormat {
public:

    zCTextureFileFormatInternal();
    virtual ~zCTextureFileFormatInternal();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    int              CanSave() const;
    int              CanLoad() const;
    const zSTRING&   GetFileExtension() const;
    int              ReadHeader(zFILE& file);
    int              ReadData(zFILE& file, zCTextureExchange* texture, int skipBytes);
    int              WriteHeader(zFILE& file, zCTextureExchange* texture);
    int              WriteData(zFILE& file, zCTextureExchange* texture);
    int              LoadTexture(const zSTRING& fileName, zCTextureExchange* texture);
    int              LoadTexture(zFILE& file, zCTextureExchange* texture);
    int              SaveTexture(const zSTRING& fileName, zCTextureExchange* texture);
    int              SaveTexture(zFILE& file, zCTextureExchange* texture);

    zCTextureInfo texInfo;
    int           numHigherThanRef;
    int           maxPixelDim;
    int           showSpyMessages;

    static zCClassDef* classDef;
};

struct zTTexPalette {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

class zCTextureExchange {
public:

    virtual ~zCTextureExchange();

    virtual int    Lock(int mode) = 0;
    virtual int    Unlock() = 0;
    virtual int    SetTextureInfo(const zCTextureInfo& info) = 0;
    virtual void*  GetPaletteBuffer() = 0;
    virtual int    GetTextureBuffer(int mipMap, void*& data, int& pitch) = 0;
    virtual zCTextureInfo GetTextureInfo() = 0;
    virtual int    CopyPaletteDataTo(void* destination) = 0;
    virtual int    CopyTextureDataTo(int mipMap, void* destination, int pitch) = 0;

    int         GetMemSizeBytes();
    zVEC4       GetRGBAAtTexel(float x, float y);
    zVEC4       GetRGBAAtPtr(unsigned char* data, zTTexPalette* palette,
        zTRnd_TextureFormat format);
    static int  CopyContents(zCTextureExchange* source, zCTextureExchange* destination);
};

class zCTexAniCtrl {
public:

    zCTexAniCtrl();
    void AdvanceAni(zCTexture* texture);

    int aniChannel;
    float actFrame;
    float aniFPS;
    unsigned long frameCtr;
    int bOneShotAni;

    static int masterFrameCtr;
};

class zCTexture : public zCResource, public zCTextureExchange {
public:

    enum zTTexFileDesiredBPP {
        zTEX_FILE_DESIRED_BPP_S3TC,
        zTEX_FILE_DESIRED_BPP_8,
        zTEX_FILE_DESIRED_BPP_16,
        zTEX_FILE_DESIRED_BPP_32,
        zTEX_FILE_DESIRED_BPP_COUNT
    };

    zCTexture();
    virtual ~zCTexture();
    static void operator delete(void* memory);

    zCClassDef* _GetClassDef() const;
    void        InitValues();
    int         HasAlpha();
    int         GetChromaKeyingEnabled() const;
    void        SetChromaKeyingEnabled(int enabled);
    int         IsLightmap() const;
    zCTexture*  GetAniTexture();
    void        AddAniTex(int channel, zCTexture* texture);
    void        PrecacheTexAniFrames(float priority);
    int         LoadResourceData();
    int         ReleaseResourceData();
    unsigned long GetResSizeBytes();
    void        GetCacheConfig(unsigned long& cacheOutTime, unsigned long& cacheSize);
    void        ForceConvertAndReloadTexture();
    int         CacheInNamed(const zSTRING* name);
    int         LoadAndConvertInternal(zFILE& file);
    int         SaveAndConvertInternal(zFILE& file);
    int         IsTextureTileBaseSlice() const;
    int         CreateTextureTile(zCTextureExchange* source);
    void        GetPixelSize(int& sizeX, int& sizeY);
    virtual void ReleaseData() = 0;

    static zCTexture* SearchName(zSTRING& name);
    static zCTexture* Load(zSTRING& name, int flags);
    static zCTexture* LoadSingle(zSTRING& name, int flags);
    static int       Exists(zSTRING& name);
    static void      AutoDetectRendererTexScale();
    static void      RefreshTexMaxSize(int maxSize);
    static int       ConvertTexture(const zSTRING& fileName);
    static void      ScanConvertTextures(zSTRING path);
    static zTRnd_TextureFormat CalcNextBestTexFormat(zTRnd_TextureFormat format);
    static void  CalcDesiredBPP(const zSTRING* fileName, zTTexFileDesiredBPP& desiredBPP,
        zTRnd_TextureFormat& format, int& forceConversion);

    zCTexture* nextAni[3];
    zCTexture* prevAni[3];
    int        actAniFrame[3];
    int        numAniFrames[3];
    unsigned char hasAlpha         : 1;
    unsigned char isAnimated       : 1;
    unsigned char changingRealtime : 1;
    unsigned char isTextureTile    : 1;

    static zCClassDef* classDef;
    static zSTRING zTEX_DEFAULT_TEXTURE_FILE;
    static zTTexScaleSize s_texScaleSize;
    static zTTexScaleBPP s_texScaleBPP;
    static unsigned long s_texSupportedFormats;
    static char s_useDefaultTex;
    static int s_globalLoadTextures;
    static unsigned long s_texMaxPixelSize;
    static zSTRING* s_texDesiredBPPSuffix;
    static zTRnd_TextureFormat s_texDesiredBPPDefault;
    static float s_texRefRAM;
    static zTTexFileDesiredBPP s_texRefBPP;
};

class zCTextureConvert : public zCTextureExchange {
public:

    virtual ~zCTextureConvert();

    int  LoadFromFileFormat(const zSTRING& fileName);
    int  SaveToFileFormat(const zSTRING& fileName);
    int  LoadFromFileFormat(zFILE& file, zCTextureFileFormat* format);
    int  SaveToFileFormat(zFILE& file, zCTextureFileFormat* format);
    int  SaveInternal(zFILE& file);
    int  LoadInternal(zFILE& file);
    void ConvertToNewFormat(const zTRnd_TextureFormat& format);
    void ConvertToNewSize(int sizeX, int sizeY);
    void GenerateMipMaps();

    static int  CalcNumMipMaps(unsigned long sizeX, unsigned long sizeY);
    static void CorrectAspectRatio(int& sizeX, int& sizeY);
    static void CorrectPow2(int& sizeX, int& sizeY);
};

class zCLightMap : public zCObject {
public:

    zCLightMap();
    virtual ~zCLightMap();

    static zCObject* _CreateNewInstance();
    zCClassDef*      _GetClassDef() const;
    void             SetTexture(zCTexture* texture);
    void             CalcLightmapOriginUpRight(const zVEC3& origin, const zVEC3& up,
        const zVEC3& right);

    zVEC3     lightmapOrigin;
    zVEC3     lightmapUVUp;
    zVEC3     lightmapUVRight;
    zCTexture* tex;

    static zCClassDef* classDef;
};

class zCTextureFileHandler : public zCScanDirFileHandler {
public:

    virtual ~zCTextureFileHandler() {}
    int HandleFile(const zSTRING& fileName, const char* directory, void* findData);
};

#endif
