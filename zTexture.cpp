#include "include/zTexture.h"
#include "include/oGame.h"
#include <windows.h>

static int textureFormatState = -1;
static int textureState = -1;
static int textureRuntimeState = -1;
static zCClassDef zCTextureFileFormatClassDef("zCTextureFileFormat", "zCObject", NULL, 0, 0x24, 0, "");
zCClassDef* zCTextureFileFormat::classDef = &zCTextureFileFormatClassDef;
static zCClassDef zCTextureFileFormatTGAClassDef("zCTextureFileFormatTGA", "zCTextureFileFormat",
    NULL, 0, 0x84, 0, "");
zCClassDef* zCTextureFileFormatTGA::classDef = &zCTextureFileFormatTGAClassDef;
static zCClassDef zCTextureFileFormatInternalClassDef("zCTextureFileFormatInternal",
    "zCTextureFileFormat", zCTextureFileFormatInternal::_CreateNewInstance, 1, 0x4C, 0, "");
zCClassDef* zCTextureFileFormatInternal::classDef = &zCTextureFileFormatInternalClassDef;
zSTRING zCTexture::zTEX_DEFAULT_TEXTURE_FILE("DEFAULT.TGA");
static zSTRING textureBppSuffixes[4] = { "-C", "-8", "-16", "-32" };
// EXE: 0x0099B368
zSTRING* zCTexture::s_texDesiredBPPSuffix = textureBppSuffixes;
// EXE: 0x008A6AAC
zTTexScaleSize zCTexture::s_texScaleSize = static_cast<zTTexScaleSize>(2);
// EXE: 0x008A6AB0
zTTexScaleBPP zCTexture::s_texScaleBPP = static_cast<zTTexScaleBPP>(1);
// EXE: 0x0099B3B8
unsigned long zCTexture::s_texSupportedFormats;
// EXE: 0x008A6AA8
char zCTexture::s_useDefaultTex = 1;
// EXE: 0x0099B3BC
unsigned long zCTexture::s_texMaxPixelSize;
// EXE: 0x008A6ABC
zTRnd_TextureFormat zCTexture::s_texDesiredBPPDefault = static_cast<zTRnd_TextureFormat>(10);
// EXE: 0x008A6AB4
float zCTexture::s_texRefRAM = 16.0f;
// EXE: 0x008A6AB8
zCTexture::zTTexFileDesiredBPP zCTexture::s_texRefBPP =
    static_cast<zCTexture::zTTexFileDesiredBPP>(1);

zCTextureFileFormatTGA::zCTextureFileFormatTGA()
    : palette(0), data(0), dataBase(0), pitchXBytes(0), tgaRLE(0),
      tgaRLEflag(0), tgaRLEcount(0)
{
}

zCTextureFileFormatTGA::~zCTextureFileFormatTGA()
{
    palette = 0;
    data = 0;
    dataBase = 0;
}

// EXE: 0x005F0770
zCObject* zCTextureFileFormatTGA::_CreateNewInstance()
{
    zCTextureFileFormatTGA* format = new zCTextureFileFormatTGA;
    zCClassDef::ObjectCreated(format, classDef);
    return format;
}

void zCTextureFileFormatTGA::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTextureFileFormatTGA::_GetClassDef() const
{
    return classDef;
}

int zCTextureFileFormatTGA::SaveTexture(const zSTRING&, zCTextureExchange*)
{
    return 0;
}

int zCTextureFileFormatTGA::SaveTexture(zFILE&, zCTextureExchange*)
{
    return 0;
}

int zCTextureFileFormatTGA::CanSave() const
{
    return 0;
}

int zCTextureFileFormatTGA::CanLoad() const
{
    return 1;
}

zCTextureFileFormatInternal::zCTextureFileFormatInternal()
    : numHigherThanRef(0), maxPixelDim(65536), showSpyMessages(1)
{
}

zCTextureFileFormatInternal::~zCTextureFileFormatInternal()
{
}

// EXE: 0x005F0AE0
zCObject* zCTextureFileFormatInternal::_CreateNewInstance()
{
    zCTextureFileFormatInternal* format = new zCTextureFileFormatInternal;
    zCClassDef::ObjectCreated(format, classDef);
    return format;
}

void zCTextureFileFormatInternal::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCTextureFileFormatInternal::_GetClassDef() const
{
    return classDef;
}

int zCTextureFileFormatInternal::CanSave() const
{
    return 1;
}

int zCTextureFileFormatInternal::CanLoad() const
{
    return 1;
}

zCClassDef* zCTextureFileFormat::_GetClassDef() const
{
    return classDef;
}

zCTextureFileFormat::~zCTextureFileFormat()
{
}

const zSTRING& zCTextureFileFormatTGA::GetFileExtension() const
{
    static zSTRING extension(".TGA");
    return extension;
}

zCTextureExchange::~zCTextureExchange()
{
}

int zCTextureFileFormatTGA::ReadTGAHeader(zFILE* file)
{
    if (file->Read(&tgaHeader, sizeof(tgaHeader)) != sizeof(tgaHeader))
        return 0;
    if (tgaHeader.IDLength)
        file->Seek(file->Pos() + tgaHeader.IDLength);
    texInfo.numMipMap = 1;
    texInfo.sizeX = tgaHeader.WidthLo | (tgaHeader.WidthHi << 8);
    texInfo.sizeY = tgaHeader.HeightLo | (tgaHeader.HeightHi << 8);
    texInfo.refSizeX = texInfo.sizeX;
    texInfo.refSizeY = texInfo.sizeY;
    if (texInfo.sizeX <= 0 || texInfo.sizeY <= 0)
        return 0;
    const int alphaBits = tgaHeader.PixelDepth == 8 ? 0 : tgaHeader.ImageDescriptor & 0xF;
    const int imageType = tgaHeader.ImgType == 9 ? 1 :
        tgaHeader.ImgType == 10 ? 2 : tgaHeader.ImgType;
    if (imageType == 1) {
        const int paletteStart = tgaHeader.CMapStartLo | (tgaHeader.CMapStartHi << 8);
        const int paletteLength = tgaHeader.CMapLengthLo | (tgaHeader.CMapLengthHi << 8);
        if (tgaHeader.CMapType != 1 || paletteStart + paletteLength > 256)
            return 0;
        texFormat = zRND_TEX_FORMAT_PAL_8;
    } else if (imageType == 2) {
        if (tgaHeader.PixelDepth == 15 || tgaHeader.PixelDepth == 16)
            texFormat = alphaBits ? zRND_TEX_FORMAT_ARGB_1555 : zRND_TEX_FORMAT_RGB_565;
        else if (tgaHeader.PixelDepth == 24 && !alphaBits)
            texFormat = zRND_TEX_FORMAT_BGR_888;
        else if (tgaHeader.PixelDepth == 32)
            texFormat = zRND_TEX_FORMAT_BGRA_8888;
        else
            return 0;
    } else {
        return 0;
    }
    texInfo.texFormat = texFormat;
    return 1;
}

int zCTextureFileFormatTGA::ReadTGAColorMap(zFILE* file)
{
    if (!tgaHeader.CMapType)
        return 1;
    if (!palette)
        return 0;
    const int first = tgaHeader.CMapStartLo | (tgaHeader.CMapStartHi << 8);
    const int length = tgaHeader.CMapLengthLo | (tgaHeader.CMapLengthHi << 8);
    const int bytesPerColor = (tgaHeader.CMapDepth + 1) >> 3;
    if (bytesPerColor <= 0 || bytesPerColor > 4)
        return 0;
    if (tgaHeader.ImgType != 1 && tgaHeader.ImgType != 9) {
        file->Seek(file->Pos() + length * bytesPerColor);
        return 1;
    }
    if (first + length > 256)
        return 0;
    for (int i = 0; i < 256; ++i) {
        palette[i * 3] = 0;
        palette[i * 3 + 1] = 0;
        palette[i * 3 + 2] = 0;
        if (i < first || i >= first + length)
            continue;
        unsigned char color[4] = {};
        if (file->Read(color, bytesPerColor) != bytesPerColor)
            return 0;
        if (bytesPerColor == 1) {
            palette[i * 3] = color[0];
            palette[i * 3 + 1] = color[0];
            palette[i * 3 + 2] = color[0];
        } else if (bytesPerColor == 2) {
            const unsigned short value = color[0] | (color[1] << 8);
            palette[i * 3] = (value & 0x1F) << 3;
            palette[i * 3 + 1] = ((value >> 5) & 0x1F) << 3;
            palette[i * 3 + 2] = ((value >> 10) & 0x1F) << 3;
        } else {
            palette[i * 3] = color[0];
            palette[i * 3 + 1] = color[1];
            palette[i * 3 + 2] = color[2];
        }
    }
    return 1;
}

int zCTextureFileFormatTGA::ReadTGARLEPixel(zFILE* file, unsigned char* destination,
    int bytesPerPixel)
{
    if (!tgaRLEcount) {
        unsigned char packet;
        if (file->Read(&packet, 1) != 1)
            return 0;
        tgaRLEflag = packet & 0x80;
        tgaRLEcount = (packet & 0x7F) + 1;
        if (tgaRLEflag && file->Read(tgaRLEsav, bytesPerPixel) != bytesPerPixel)
            return 0;
    }
    --tgaRLEcount;
    if (tgaRLEflag) {
        const unsigned char* source = reinterpret_cast<const unsigned char*>(tgaRLEsav);
        for (int i = 0; i < bytesPerPixel; ++i)
            destination[i] = source[i];
        return 1;
    }
    return file->Read(destination, bytesPerPixel) == bytesPerPixel;
}

int zCTextureFileFormatTGA::ReadTGASpan(zFILE* file, unsigned char* destination,
    int pixelCount, int bytesPerPixel)
{
    if (!tgaRLE)
        return file->Read(destination, pixelCount * bytesPerPixel) == pixelCount * bytesPerPixel;
    for (int i = 0; i < pixelCount; ++i) {
        if (!ReadTGARLEPixel(file, destination + i * bytesPerPixel, bytesPerPixel))
            return 0;
    }
    return 1;
}

int zCTextureFileFormatTGA::ReadTGAData(zFILE* file)
{
    const int bytesPerPixel = (tgaHeader.PixelDepth + 1) >> 3;
    tgaRLE = tgaHeader.ImgType >= 9 && tgaHeader.ImgType <= 11;
    tgaRLEcount = 0;
    int rowStep = pitchXBytes;
    data = dataBase;
    if (!(tgaHeader.ImageDescriptor & 0x20)) {
        data += (texInfo.sizeY - 1) * pitchXBytes;
        rowStep = -rowStep;
    }
    if (!ReadTGAColorMap(file))
        return 0;
    for (int y = 0; y < texInfo.sizeY; ++y) {
        if (!ReadTGASpan(file, data, texInfo.sizeX, bytesPerPixel))
            return 0;
        data += rowStep;
    }
    return 1;
}

int zCTextureFileFormatTGA::LoadTexture(zFILE& file, zCTextureExchange* texture)
{
    if (!texture || !texture->Lock(2) || !ReadTGAHeader(&file) || !texture->SetTextureInfo(texInfo))
        return 0;
    palette = static_cast<unsigned char*>(texture->GetPaletteBuffer());
    texture->GetTextureBuffer(0, dataBase, pitchXBytes);
    data = dataBase;
    if (!dataBase || !ReadTGAData(&file))
        return 0;
    return texture->Unlock() == 0;
}

int zCTextureFileFormatTGA::LoadTexture(const zSTRING& fileName, zCTextureExchange* texture)
{
    zFILE_FILE file(fileName);
    if (!file.Exists() || file.Open(false))
        return 0;
    const int result = LoadTexture(file, texture);
    file.Close();
    return result;
}

const zSTRING& zCTextureFileFormatInternal::GetFileExtension() const
{
    static zSTRING extension(".TEX");
    return extension;
}

struct zTInternalTextureHeader {
    zCTextureInfo info;
    unsigned long cookie;
    unsigned long reserved;
};

static float TextureBytesPerPixel(zTRnd_TextureFormat format)
{
    if (format <= zRND_TEX_FORMAT_BGRA_8888)
        return 4.0f;
    if (format <= zRND_TEX_FORMAT_BGR_888)
        return 3.0f;
    if (format <= zRND_TEX_FORMAT_RGB_565)
        return 2.0f;
    if (format == zRND_TEX_FORMAT_PAL_8)
        return 1.0f;
    return format == zRND_TEX_FORMAT_DXT1 ? 0.5f : 1.0f;
}

int zCTextureFileFormatInternal::ReadHeader(zFILE& file)
{
    zTInternalTextureHeader header = {};
    if (file.Read(&header, sizeof(header)) != sizeof(header) || header.cookie)
        return 0;
    texInfo = header.info;
    return 1;
}

int zCTextureFileFormatInternal::ReadData(zFILE& file, zCTextureExchange* texture, int skipBytes)
{
    const float bytesPerPixel = TextureBytesPerPixel(texInfo.texFormat);
    const int scaledSizeX = numHigherThanRef < 0
        ? texInfo.refSizeX >> -numHigherThanRef
        : texInfo.refSizeX << numHigherThanRef;
    const int scaledSizeY = numHigherThanRef < 0
        ? texInfo.refSizeY >> -numHigherThanRef
        : texInfo.refSizeY << numHigherThanRef;
    const int maxSizeX = maxPixelDim < scaledSizeX ? maxPixelDim : scaledSizeX;
    const int maxSizeY = maxPixelDim < scaledSizeY ? maxPixelDim : scaledSizeY;
    int sizeX = texInfo.sizeX;
    int sizeY = texInfo.sizeY;
    int numMipMap = texInfo.numMipMap;
    int discardedBytes = 0;

    while (sizeX > maxSizeX || sizeY > maxSizeY) {
        if (numMipMap <= 1)
            break;
        --numMipMap;
        discardedBytes += static_cast<int>(sizeX * sizeY * bytesPerPixel);
        sizeX >>= 1;
        sizeY >>= 1;
        if (numMipMap <= 1)
            break;
    }

    texInfo.sizeX = sizeX;
    texInfo.sizeY = sizeY;
    texInfo.numMipMap = numMipMap;
    if (!texture->Lock(2))
        return 0;
    texture->SetTextureInfo(texInfo);
    if (texture->GetPaletteBuffer() && file.Read(texture->GetPaletteBuffer(), 768) != 768)
        return 0;
    for (int mip = texInfo.numMipMap - 1; mip >= 0; --mip) {
        void* buffer;
        int pitch;
        if (!texture->GetTextureBuffer(mip, buffer, pitch) || !buffer)
            return 0;
        int width = texInfo.sizeX >> mip;
        int height = texInfo.sizeY >> mip;
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
        const int rowBytes = static_cast<int>(width * bytesPerPixel);
        unsigned char* destination = static_cast<unsigned char*>(buffer);
        if (pitch == rowBytes) {
            if (file.Read(destination, rowBytes * height) != rowBytes * height)
                return 0;
        } else {
            for (int y = 0; y < height; ++y) {
                if (file.Read(destination + y * pitch, rowBytes) != rowBytes)
                    return 0;
            }
        }
    }
    if (skipBytes)
        file.Seek(file.Pos() + discardedBytes);
    return texture->Unlock() == 0;
}

int zCTextureFileFormatInternal::LoadTexture(zFILE& file, zCTextureExchange* texture)
{
    return texture && ReadHeader(file) && ReadData(file, texture, 0);
}

int zCTextureFileFormatInternal::LoadTexture(const zSTRING& fileName, zCTextureExchange* texture)
{
    zFILE* file = zfactory->CreateZFile(zoptions->GetDirString(20) + fileName);
    if (!file)
        return 0;
    file->Open(0);
    const int result = LoadTexture(*file, texture);
    file->Close();
    delete file;
    return result;
}

int zCTextureFileFormatInternal::WriteHeader(zFILE& file, zCTextureExchange* texture)
{
    texInfo = texture->GetTextureInfo();
    zTInternalTextureHeader header = {};
    header.info = texInfo;
    return file.Write(&header, sizeof(header)) == sizeof(header);
}

int zCTextureFileFormatInternal::WriteData(zFILE& file, zCTextureExchange* texture)
{
    if (texture->GetPaletteBuffer() && file.Write(texture->GetPaletteBuffer(), 768) != 768)
        return 0;
    const float bytesPerPixel = TextureBytesPerPixel(texInfo.texFormat);
    for (int mip = texInfo.numMipMap - 1; mip >= 0; --mip) {
        void* buffer;
        int pitch;
        texture->GetTextureBuffer(mip, buffer, pitch);
        if (!buffer)
            return 0;
        int width = texInfo.sizeX >> mip;
        int height = texInfo.sizeY >> mip;
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
        const int rowBytes = static_cast<int>(width * bytesPerPixel);
        const unsigned char* source = static_cast<const unsigned char*>(buffer);
        if (pitch == rowBytes) {
            if (file.Write(source, rowBytes * height) != rowBytes * height)
                return 0;
        } else {
            for (int y = 0; y < height; ++y) {
                if (file.Write(source + y * pitch, rowBytes) != rowBytes)
                    return 0;
            }
        }
    }
    return 1;
}

int zCTextureFileFormatInternal::SaveTexture(zFILE& file, zCTextureExchange* texture)
{
    if (!texture || !texture->Lock(1) || !WriteHeader(file, texture) || !WriteData(file, texture))
        return 0;
    return texture->Unlock() == 0;
}

int zCTextureFileFormatInternal::SaveTexture(const zSTRING& fileName, zCTextureExchange* texture)
{
    zFILE* file = zfactory->CreateZFile(zoptions->GetDirString(20) + fileName);
    if (!file)
        return 0;
    file->Create();
    const int result = SaveTexture(*file, texture);
    file->Close();
    delete file;
    return result;
}

static zCClassDef zCTextureClassDef("zCTexture", "zCResource", NULL, 0, 0x8C, 5, "");
zCClassDef* zCTexture::classDef = &zCTextureClassDef;

zCTexture::zCTexture()
{
    InitValues();
}

void zCTexture::InitValues()
{
    hasAlpha = 0;
    isAnimated = 0;
    changingRealtime = 0;
    isTextureTile = 0;
    for (int i = 0; i < 3; ++i) {
        nextAni[i] = 0;
        prevAni[i] = 0;
        actAniFrame[i] = 0;
        numAniFrames[i] = 0;
    }
}

zCTexture::~zCTexture()
{
    for (int i = 0; i < 3; ++i) {
        if (nextAni[i]) {
            nextAni[i]->CacheOut();
            nextAni[i]->Release();
            nextAni[i] = 0;
            prevAni[i] = 0;
        }
    }
}

zCClassDef* zCTexture::_GetClassDef() const
{
    return classDef;
}

void zCTexture::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

int zCTexture::HasAlpha()
{
    return hasAlpha;
}

int zCTexture::GetChromaKeyingEnabled() const
{
    return 0;
}

void zCTexture::SetChromaKeyingEnabled(int)
{
}

int zCTexture::s_globalLoadTextures = 1;

zCTexture* zCTexture::SearchName(zSTRING& name)
{
    name.Upper();
    return static_cast<zCTexture*>(classDef->SearchHashTable(name));
}

int zCTexture::IsLightmap() const
{
    const zSTRING& name = GetObjectName();
    if (name.Length() < 4)
        return 1;
    return name[name.Length() - 4] != '.';
}

zCTexture* zCTexture::GetAniTexture()
{
    zCTexture* result = this;
    if (isAnimated) {
        for (int channel = 0; channel < 3; ++channel) {
            if (!numAniFrames[channel])
                continue;
            for (int frame = 0; frame < actAniFrame[channel]; ++frame) {
                if (!result->nextAni[channel])
                    break;
                result = result->nextAni[channel];
            }
        }
    }
    return result;
}

void zCTexture::AddAniTex(int channel, zCTexture* texture)
{
    zCTexture* tail = this;
    while (tail->nextAni[channel])
        tail = tail->nextAni[channel];
    tail->nextAni[channel] = texture;
    texture->prevAni[channel] = tail;
    texture->nextAni[channel] = 0;
}

void zCTexture::PrecacheTexAniFrames(float priority)
{
    for (zCTexture* texture = this; texture; texture = texture->nextAni[0]) {
        if (texture->cacheState != zRES_CACHED_IN) {
            texture->TouchTimeStampLocal();
            zresMan->CacheIn(texture, priority);
        } else {
            texture->TouchTimeStamp();
        }
    }
}

zCTexture* zCTexture::Load(zSTRING& name, int flags)
{
    if (!name.Length() || !s_globalLoadTextures)
        return 0;
    zCTexture* texture = SearchName(name);
    if (texture) {
        texture->AddRef();
        return texture;
    }
    return LoadSingle(name, flags);
}

zCTexture* zCTexture::LoadSingle(zSTRING& name, int flags)
{
    if (!name.Length())
        return 0;
    zCTexture* texture = zrenderer->CreateTexture();
    if (!texture)
        return 0;
    texture->isTextureTile = (flags & 1) != 0;
    texture->SetObjectName(name.Upper());
    return texture;
}

int zCTexture::Exists(zSTRING& name)
{
    name.Upper();
    if (SearchName(name))
        return 1;

    zFILE* compiledFile = zfactory->CreateZFile(
        zoptions->GetDirString(20) + name + "-C.TEX");
    if (compiledFile->Exists()) {
        delete compiledFile;
        return 1;
    }
    delete compiledFile;

    zPATH sourcePath;
    return sourcePath.SearchFile(zoptions->GetDirString(12), name + ".*", 1) != 0;
}

static int textureScaleSize = 2;
static int textureScaleBPP;
static unsigned long textureMaxPixelSize = 65536;

void zCTexture::RefreshTexMaxSize(int maxSize)
{
    if (maxSize > 0)
        textureMaxPixelSize = maxSize;
    if (textureMaxPixelSize < 1)
        textureMaxPixelSize = 1;
    else if (textureMaxPixelSize > 65536)
        textureMaxPixelSize = 65536;
}

void zCTexture::AutoDetectRendererTexScale()
{
    textureScaleSize = 2;
    textureScaleBPP = 0;
    RefreshTexMaxSize(0);
}

int zCTextureConvert::LoadFromFileFormat(const zSTRING& fileName)
{
    for (int i = 0; i < zCClassDef::classDefList->numInArray; ++i) {
        zCClassDef* formatClass = zCClassDef::classDefList->GetSafe(i);
        if (!formatClass)
            continue;
        for (zCClassDef* baseClass = formatClass; baseClass; baseClass = baseClass->baseClassDef) {
            if (baseClass != zCTextureFileFormat::classDef)
                continue;
            if (formatClass != zCTextureFileFormat::classDef) {
                zCTextureFileFormat* format =
                    static_cast<zCTextureFileFormat*>(formatClass->CreateNewInstance());
                if (format->CanLoad() && fileName.Search(format->GetFileExtension()) != -1) {
                    const int result = format->LoadTexture(fileName, this);
                    format->Release();
                    return result;
                }
                format->Release();
            }
            break;
        }
    }
    return 0;
}

int zCTextureConvert::SaveToFileFormat(const zSTRING& fileName)
{
    for (int i = 0; i < zCClassDef::classDefList->numInArray; ++i) {
        zCClassDef* formatClass = zCClassDef::classDefList->GetSafe(i);
        if (!formatClass)
            continue;
        for (zCClassDef* baseClass = formatClass; baseClass; baseClass = baseClass->baseClassDef) {
            if (baseClass != zCTextureFileFormat::classDef)
                continue;
            if (formatClass != zCTextureFileFormat::classDef) {
                zCTextureFileFormat* format =
                    static_cast<zCTextureFileFormat*>(formatClass->CreateNewInstance());
                if (format->CanSave() && fileName.Search(format->GetFileExtension()) != -1) {
                    format->SaveTexture(fileName, this);
                    format->Release();
                    return 1;
                }
                format->Release();
            }
            break;
        }
    }
    return 0;
}

int zCTextureConvert::LoadFromFileFormat(zFILE& file, zCTextureFileFormat* format)
{
    return format->CanLoad() && format->LoadTexture(file, this);
}

int zCTextureConvert::SaveToFileFormat(zFILE& file, zCTextureFileFormat* format)
{
    return format->CanSave() && format->SaveTexture(file, this);
}

int zCTextureConvert::SaveInternal(zFILE& file)
{
    zCTextureFileFormatInternal format;
    return format.SaveTexture(file, this);
}

int zCTextureConvert::LoadInternal(zFILE& file)
{
    zCTextureFileFormatInternal format;
    return format.LoadTexture(file, this);
}

void zCTextureConvert::ConvertToNewFormat(const zTRnd_TextureFormat& format)
{
    zCTextureInfo info = GetTextureInfo();
    info.texFormat = format;
    SetTextureInfo(info);
}

void zCTextureConvert::ConvertToNewSize(int sizeX, int sizeY)
{
    CorrectPow2(sizeX, sizeY);
    CorrectAspectRatio(sizeX, sizeY);
    zCTextureInfo info = GetTextureInfo();
    info.sizeX = sizeX;
    info.sizeY = sizeY;
    info.numMipMap = CalcNumMipMaps(sizeX, sizeY);
    SetTextureInfo(info);
}

void zCTextureConvert::GenerateMipMaps()
{
    zCTextureInfo info = GetTextureInfo();
    info.numMipMap = CalcNumMipMaps(info.sizeX, info.sizeY);
    SetTextureInfo(info);
}

int zCTextureConvert::CalcNumMipMaps(unsigned long sizeX, unsigned long sizeY)
{
    int result = 1;
    if (sizeX > 4) {
        while (sizeY > 4 && sizeX > 4) {
            sizeX >>= 1;
            sizeY >>= 1;
            ++result;
        }
    }
    return result - 1;
}

void zCTextureConvert::CorrectAspectRatio(int& sizeX, int& sizeY)
{
    if (sizeX > sizeY && sizeX >= sizeY * 8)
        sizeX = sizeY * 8;
    else if (sizeY > sizeX && sizeY >= sizeX * 8)
        sizeY = sizeX * 8;
}

void zCTextureConvert::CorrectPow2(int& sizeX, int& sizeY)
{
    int value = sizeX;
    while (value & (value - 1))
        value &= value - 1;
    sizeX = value;
    value = sizeY;
    while (value & (value - 1))
        value &= value - 1;
    sizeY = value;
}

int zCTexture::LoadResourceData()
{
    return IsLightmap() ? 1 : CacheInNamed(&GetObjectName());
}

int zCTexture::ReleaseResourceData()
{
    ReleaseData();
    if (isAnimated) {
        int channel = 0;
        while (!prevAni[channel]) {
            ++channel;
            if (channel >= 3) {
                for (channel = 0; channel < 3; ++channel) {
                    for (zCTexture* texture = nextAni[channel]; texture; texture = texture->nextAni[channel]) {
                        texture->CacheOut();
                        texture->cacheState = (texture->cacheState & ~3) ^ 3;
                    }
                }
                break;
            }
        }
    }
    return 1;
}

unsigned long zCTexture::GetResSizeBytes()
{
    if (!zrenderer)
        return 0;
    const zCTextureInfo info = GetTextureInfo();
    const float bytesPerPixel = TextureBytesPerPixel(info.texFormat);
    unsigned long result = 0;
    int width = info.sizeX;
    int height = info.sizeY;
    for (int mip = 0; mip < info.numMipMap; ++mip) {
        result += static_cast<unsigned long>(width * height * bytesPerPixel);
        width >>= 1;
        height >>= 1;
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
    }
    if (info.texFormat == zRND_TEX_FORMAT_PAL_8)
        result += 768;
    return result;
}

void zCTexture::GetCacheConfig(unsigned long& cacheOutTime, unsigned long& cacheSize)
{
    static unsigned long textureCacheOutTime = 240000;
    static unsigned long textureCacheSize = 32000000;
    static int initialized;
    if (!initialized) {
        initialized = 1;
        textureCacheOutTime = zoptions->ReadDWord("ENGINE", "zTexCacheOutTimeMSec", 240000);
        textureCacheSize = zoptions->ReadDWord("ENGINE", "zTexCacheSizeMaxBytes", 32000000);

        MEMORYSTATUS memoryStatus = {};
        memoryStatus.dwLength = sizeof(memoryStatus);
        GlobalMemoryStatus(&memoryStatus);
        const unsigned long totalMemoryMB = (memoryStatus.dwTotalPhys + 0xFFFFF) >> 20;
        if (totalMemoryMB > 512) {
            textureCacheOutTime *= 2;
            textureCacheSize *= 3;
        } else if (totalMemoryMB > 256 ||
            !zrenderer->SupportsTextureFormatHardware(static_cast<zTRnd_TextureFormat>(10))) {
            textureCacheOutTime = static_cast<unsigned long>(textureCacheOutTime * 1.5f);
            textureCacheSize *= 2;
        }
    }
    cacheOutTime = textureCacheOutTime;
    cacheSize = textureCacheSize;
}

int zCTexture::ConvertTexture(const zSTRING& fileName)
{
    if (!fileName.Length())
        return 1;
    zCTextureConvert converter;
    if (!converter.LoadFromFileFormat(fileName))
        return 0;
    return converter.SaveToFileFormat(fileName + "-C.TEX");
}

void zCTexture::ForceConvertAndReloadTexture()
{
    CacheOut();
    ConvertTexture(GetObjectName());
    if (cacheState == zRES_CACHED_IN) {
        TouchTimeStamp();
        return;
    }
    TouchTimeStampLocal();
    zresMan->CacheIn(this, 0.5f);
}

zTRnd_TextureFormat zCTexture::CalcNextBestTexFormat(zTRnd_TextureFormat format)
{
    zTRnd_TextureFormat result = format;
    for (int i = 0; i < zRND_TEX_FORMAT_COUNT; ++i) {
        if (zrenderer->SupportsTextureFormat(result))
            break;
        result = static_cast<zTRnd_TextureFormat>((result + 1) % zRND_TEX_FORMAT_COUNT);
    }
    return result;
}

// EXE: 0x005F6340
void zCTexture::CalcDesiredBPP(const zSTRING* fileName, zTTexFileDesiredBPP& desiredBPP,
    zTRnd_TextureFormat& format, int& forceConversion)
{
    static const zTRnd_TextureFormat defaults[] = {
        zRND_TEX_FORMAT_DXT1, zRND_TEX_FORMAT_PAL_8,
        zRND_TEX_FORMAT_RGB_565, zRND_TEX_FORMAT_BGRA_8888
    };
    desiredBPP = zTEX_FILE_DESIRED_BPP_32;
    forceConversion = 0;
    if (fileName) {
        if (fileName->Search("8BIT") != -1)
            desiredBPP = zTEX_FILE_DESIRED_BPP_8;
        else if (fileName->Search("16BIT") != -1)
            desiredBPP = zTEX_FILE_DESIRED_BPP_16;
        else if (fileName->Search("32BIT") != -1)
            desiredBPP = zTEX_FILE_DESIRED_BPP_32;
    }
    format = CalcNextBestTexFormat(defaults[desiredBPP]);
    desiredBPP = static_cast<zTTexFileDesiredBPP>(format == zRND_TEX_FORMAT_DXT1 ? 0 :
        TextureBytesPerPixel(format) <= 1.0f ? 1 :
        TextureBytesPerPixel(format) <= 2.0f ? 2 : 3);
}

int zCTexture::LoadAndConvertInternal(zFILE& file)
{
    zCTextureFileFormatInternal format;
    if (!format.ReadHeader(file))
        return 0;
    format.texInfo.texFormat = CalcNextBestTexFormat(format.texInfo.texFormat);
    return format.ReadData(file, this, 0);
}

int zCTexture::SaveAndConvertInternal(zFILE& file)
{
    zCTextureFileFormatInternal format;
    return format.SaveTexture(file, this);
}

static zSTRING defaultTextureFile("DEFAULT.TGA");
static int useDefaultTexture = 1;

int zCTexture::CacheInNamed(const zSTRING* name)
{
    if (!name)
        return 0;
    zTTexFileDesiredBPP desiredBPP;
    int forceConversion;
    zTRnd_TextureFormat desiredFormat;
    CalcDesiredBPP(name, desiredBPP, desiredFormat, forceConversion);
    zFILE_FILE compiledFile(*name + "-C.TEX");
    if (compiledFile.Exists() && !compiledFile.Open(false)) {
        const int result = LoadAndConvertInternal(compiledFile);
        compiledFile.Close();
        if (result) {
            hasAlpha = GetTextureInfo().texFormat == zRND_TEX_FORMAT_BGRA_8888;
            return 1;
        }
    }
    zCTextureFileFormatTGA sourceFormat;
    if (sourceFormat.LoadTexture(*name, this)) {
        hasAlpha = GetTextureInfo().texFormat == zRND_TEX_FORMAT_BGRA_8888;
        return 1;
    }
    if (ConvertTexture(*name)) {
        zFILE_FILE convertedFile(*name + "-C.TEX");
        if (!convertedFile.Open(false)) {
            const int result = LoadAndConvertInternal(convertedFile);
            convertedFile.Close();
            if (result)
                return 1;
        }
    }
    if (!useDefaultTexture || *name == defaultTextureFile)
        return 0;
    return CacheInNamed(&defaultTextureFile);
}

int zCTexture::IsTextureTileBaseSlice() const
{
    if (!isTextureTile)
        return 0;
    for (int i = 0; i < 3; ++i) {
        if (prevAni[i])
            return 0;
    }
    return 1;
}

int zCTexture::CreateTextureTile(zCTextureExchange* source)
{
    if (!source || !textureMaxPixelSize)
        return 0;
    const zCTextureInfo sourceInfo = source->GetTextureInfo();
    int tilesX = sourceInfo.sizeX / textureMaxPixelSize;
    int tilesY = sourceInfo.sizeY / textureMaxPixelSize;
    if (tilesX < 1)
        tilesX = 1;
    if (tilesY < 1)
        tilesY = 1;
    if (tilesX == 1 && tilesY == 1)
        return zCTextureExchange::CopyContents(source, this);
    numAniFrames[0] = tilesX;
    numAniFrames[1] = tilesY;
    isAnimated = 1;
    for (int x = 0; x < tilesX; ++x) {
        for (int y = 0; y < tilesY; ++y) {
            zCTexture* tile = zrenderer->CreateTexture();
            if (!tile)
                return 0;
            zSTRING tileName;
            tileName.Sprintf("%s_%d_%d", GetObjectName().ToChar(), x, y);
            tile->SetObjectName(tileName.Upper());
            tile->isTextureTile = 1;
            tile->hasAlpha = hasAlpha;
            if (!x && !y)
                zCTextureExchange::CopyContents(source, tile);
            AddAniTex(0, tile);
            AddAniTex(1, tile);
        }
    }
    return 1;
}

void zCTexture::GetPixelSize(int& sizeX, int& sizeY)
{
    const zCTextureInfo info = GetTextureInfo();
    sizeX = info.sizeX;
    sizeY = info.sizeY;
}

int zCTextureFileHandler::HandleFile(const zSTRING& fileName, const char*, void*)
{
    if (fileName.Search(".TGA") != -1) {
        zCTexture::ConvertTexture(fileName);
        return 1;
    }
    if (fileName.Search(".PCX") != -1) {
        AddMessage("Attention: PCX-Files must be converted to TGA");
        return 0;
    }
    zerr.Message("B: SDIR: Unknown filetype. Can't handle file " + fileName);
    return 0;
}

void zCTexture::ScanConvertTextures(zSTRING path)
{
    zerr.Message("D: TEX: Searching texture directory for unconverted textures...");
    zCScanDir scanner;
    zCTextureFileHandler handler;
    handler.RegisterFileType(".tga");
    scanner.RegisterFileHandler(&handler, "TextureConvert");
    if (path.Length())
        scanner.Scan(zoptions->GetDirString(12) + path + "\\");
    else
        scanner.Scan(zoptions->GetDirString(12));
}

int zCTextureExchange::CopyContents(zCTextureExchange* source, zCTextureExchange* destination)
{
    int result = destination->Lock(2) && source->Lock(1);
    const zCTextureInfo info = source->GetTextureInfo();
    result = destination->SetTextureInfo(info) && result;
    result = source->CopyPaletteDataTo(destination->GetPaletteBuffer()) && result;
    for (int mip = 0; mip < info.numMipMap; ++mip) {
        void* buffer;
        int pitch;
        if (!destination->GetTextureBuffer(mip, buffer, pitch)) {
            result = 0;
            break;
        }
        result = source->CopyTextureDataTo(mip, buffer, pitch) && result;
    }
    result = source->Unlock() && result;
    result = destination->Unlock() && result;
    return result;
}

int zCTextureExchange::GetMemSizeBytes()
{
    if (!zrenderer)
        return 0;
    const zCTextureInfo info = GetTextureInfo();
    const float bytesPerPixel = TextureBytesPerPixel(info.texFormat);
    int result = 0;
    int width = info.sizeX;
    int height = info.sizeY;
    for (int mip = 0; mip < info.numMipMap; ++mip) {
        result += static_cast<int>(width * height * bytesPerPixel);
        width >>= 1;
        height >>= 1;
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
    }
    if (info.texFormat == zRND_TEX_FORMAT_PAL_8)
        result += 768;
    return result;
}

zVEC4 zCTextureExchange::GetRGBAAtPtr(unsigned char* data, zTTexPalette* palette,
    zTRnd_TextureFormat format)
{
    if (format > zRND_TEX_FORMAT_PAL_8)
        return zVEC4(zVEC3(0, 0, 0), 255.0f);
    if (format == zRND_TEX_FORMAT_PAL_8) {
        const zTTexPalette& color = palette[data[0]];
        return zVEC4(zVEC3(color.r, color.g, color.b), 255.0f);
    }
    if (format <= zRND_TEX_FORMAT_BGRA_8888) {
        static const int redOffsets[] = { 1, 3, 0, 2 };
        static const int greenOffsets[] = { 2, 2, 1, 1 };
        static const int blueOffsets[] = { 3, 1, 2, 0 };
        static const int alphaOffsets[] = { 0, 0, 3, 3 };
        return zVEC4(zVEC3(data[redOffsets[format]], data[greenOffsets[format]],
            data[blueOffsets[format]]), data[alphaOffsets[format]]);
    }
    if (format == zRND_TEX_FORMAT_RGB_888)
        return zVEC4(zVEC3(data[0], data[1], data[2]), 255.0f);
    if (format == zRND_TEX_FORMAT_BGR_888)
        return zVEC4(zVEC3(data[2], data[1], data[0]), 255.0f);
    const unsigned short value = data[0] | (data[1] << 8);
    if (format == zRND_TEX_FORMAT_ARGB_4444)
        return zVEC4(zVEC3((value >> 4) & 0xF0, value & 0xF0, (value & 0xF) << 4),
            (value >> 8) & 0xF0);
    if (format == zRND_TEX_FORMAT_ARGB_1555)
        return zVEC4(zVEC3((value >> 7) & 0xF8, (value >> 2) & 0xF8, (value & 0x1F) << 3),
            (value & 0x8000) ? 255.0f : 0);
    return zVEC4(zVEC3((value >> 8) & 0xF8, (value >> 3) & 0xFC, (value & 0x1F) << 3), 255.0f);
}

zVEC4 zCTextureExchange::GetRGBAAtTexel(float x, float y)
{
    const zCTextureInfo info = GetTextureInfo();
    if (x < 0)
        x = 0;
    else if (x > info.sizeX - 1)
        x = info.sizeX - 1.0f;
    if (y < 0)
        y = 0;
    else if (y > info.sizeY - 1)
        y = info.sizeY - 1.0f;
    const int x0 = static_cast<int>(x);
    const int y0 = static_cast<int>(y);
    const int x1 = x0 + 1 < info.sizeX ? x0 + 1 : x0;
    const int y1 = y0 + 1 < info.sizeY ? y0 + 1 : y0;
    const float blendX = x - x0;
    const float blendY = y - y0;
    if (!Lock(1))
        return zVEC4(zVEC3(0, 0, 0), 255.0f);
    void* buffer;
    int pitch;
    zVEC4 result(zVEC3(255, 0, 255), 255.0f);
    if (GetTextureBuffer(0, buffer, pitch) && buffer) {
        unsigned char* pixels = static_cast<unsigned char*>(buffer);
        zTTexPalette* palette = static_cast<zTTexPalette*>(GetPaletteBuffer());
        const int x0Offset = static_cast<int>(x0 * TextureBytesPerPixel(info.texFormat));
        const int x1Offset = static_cast<int>(x1 * TextureBytesPerPixel(info.texFormat));
        zVEC4 colors[4] = {
            GetRGBAAtPtr(pixels + y0 * pitch + x0Offset, palette, info.texFormat),
            GetRGBAAtPtr(pixels + y0 * pitch + x1Offset, palette, info.texFormat),
            GetRGBAAtPtr(pixels + y1 * pitch + x0Offset, palette, info.texFormat),
            GetRGBAAtPtr(pixels + y1 * pitch + x1Offset, palette, info.texFormat)
        };
        const float weights[] = {
            (1.0f - blendX) * (1.0f - blendY), blendX * (1.0f - blendY),
            (1.0f - blendX) * blendY, blendX * blendY
        };
        for (int channel = 0; channel < 4; ++channel) {
            result.n[channel] = colors[0].n[channel] * weights[0] +
                colors[1].n[channel] * weights[1] + colors[2].n[channel] * weights[2] +
                colors[3].n[channel] * weights[3];
        }
    }
    Unlock();
    return result;
}

static zCClassDef zCLightMapClassDef("zCLightMap", "zCObject", NULL, 0, 0x4C, 0, "");
zCClassDef* zCLightMap::classDef = &zCLightMapClassDef;

zCLightMap::zCLightMap()
    : tex(0)
{
}

zCLightMap::~zCLightMap()
{
    if (tex)
        tex->Release();
    tex = 0;
}

// EXE: 0x005F8DD0
zCObject* zCLightMap::_CreateNewInstance()
{
    zCLightMap* lightMap = new zCLightMap;
    zCClassDef::ObjectCreated(lightMap, classDef);
    return lightMap;
}

zCClassDef* zCLightMap::_GetClassDef() const
{
    return classDef;
}

void zCLightMap::SetTexture(zCTexture* texture)
{
    if (texture == tex)
        return;
    if (tex)
        tex->Release();
    tex = texture;
    if (tex)
        tex->AddRef();
}

void zCLightMap::CalcLightmapOriginUpRight(const zVEC3& origin, const zVEC3& up,
    const zVEC3& right)
{
    const float upLength2 = up.n[0] * up.n[0] + up.n[1] * up.n[1] + up.n[2] * up.n[2];
    const float rightLength2 = right.n[0] * right.n[0] + right.n[1] * right.n[1] +
        right.n[2] * right.n[2];
    lightmapUVUp = up * (1.0f / upLength2);
    lightmapUVRight = right * (1.0f / rightLength2);
    lightmapOrigin = origin;
}
