#include "include/zTexConvert.h"
#include <cstring>

static char* conversionBuffer;

void FreeTexConvert()
{
    delete[] conversionBuffer;
    conversionBuffer = 0;
}

char* ResizeTexture(int* sourceDimensions, int targetX, int targetY, void* target,
    void* source, unsigned long format)
{
    int sourceX = sourceDimensions[0];
    int sourceY = sourceDimensions[1];
    int bytesPerPixel = static_cast<int>(texFormatInfoList[format].bytesPerPixel);
    float stepX = static_cast<float>(sourceX) / targetX;
    float stepY = static_cast<float>(sourceY) / targetY;
    unsigned char* sourceBytes = static_cast<unsigned char*>(source);
    unsigned char* targetBytes = static_cast<unsigned char*>(target);
    for (int y = 0; y < targetY; ++y) {
        int sourceRow = static_cast<int>(y * stepY) * sourceX;
        for (int x = 0; x < targetX; ++x) {
            int sourcePixel = (sourceRow + static_cast<int>(x * stepX)) * bytesPerPixel;
            std::memcpy(targetBytes, sourceBytes + sourcePixel, bytesPerPixel);
            targetBytes += bytesPerPixel;
        }
    }
    return static_cast<char*>(target);
}

static void DecodeTexel(const unsigned char* source, zTRnd_TextureFormat format,
    const unsigned char* palette, unsigned char& red, unsigned char& green, unsigned char& blue,
    unsigned char& alpha)
{
    alpha = 255;
    switch (format) {
    case zRND_TEX_FORMAT_ARGB_8888:
        alpha = source[0];
        red = source[1];
        green = source[2];
        blue = source[3];
        break;
    case zRND_TEX_FORMAT_ABGR_8888:
        alpha = source[0];
        blue = source[1];
        green = source[2];
        red = source[3];
        break;
    case zRND_TEX_FORMAT_RGBA_8888:
        red = source[0];
        green = source[1];
        blue = source[2];
        alpha = source[3];
        break;
    case zRND_TEX_FORMAT_BGRA_8888:
        blue = source[0];
        green = source[1];
        red = source[2];
        alpha = source[3];
        break;
    case zRND_TEX_FORMAT_RGB_888:
        red = source[0];
        green = source[1];
        blue = source[2];
        break;
    case zRND_TEX_FORMAT_BGR_888:
        blue = source[0];
        green = source[1];
        red = source[2];
        break;
    case zRND_TEX_FORMAT_ARGB_4444: {
        unsigned short value = *reinterpret_cast<const unsigned short*>(source);
        alpha = (value >> 8) & 0xf0;
        red = (value >> 4) & 0xf0;
        green = value & 0xf0;
        blue = (value << 4) & 0xf0;
        alpha |= alpha >> 4;
        red |= red >> 4;
        green |= green >> 4;
        blue |= blue >> 4;
        break;
    }
    case zRND_TEX_FORMAT_ARGB_1555: {
        unsigned short value = *reinterpret_cast<const unsigned short*>(source);
        alpha = value & 0x8000 ? 255 : 0;
        red = (value >> 7) & 0xf8;
        green = (value >> 2) & 0xf8;
        blue = (value << 3) & 0xf8;
        red |= red >> 5;
        green |= green >> 5;
        blue |= blue >> 5;
        break;
    }
    case zRND_TEX_FORMAT_RGB_565: {
        unsigned short value = *reinterpret_cast<const unsigned short*>(source);
        red = (value >> 8) & 0xf8;
        green = (value >> 3) & 0xfc;
        blue = (value << 3) & 0xf8;
        red |= red >> 5;
        green |= green >> 6;
        blue |= blue >> 5;
        break;
    }
    case zRND_TEX_FORMAT_PAL_8: {
        int index = *source * 3;
        red = palette[index];
        green = palette[index + 1];
        blue = palette[index + 2];
        break;
    }
    default:
        red = source[0];
        green = source[0];
        blue = source[0];
        break;
    }
}

static int EncodeTexel(unsigned char* target, zTRnd_TextureFormat format,
    unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
    switch (format) {
    case zRND_TEX_FORMAT_ARGB_8888:
        target[0] = alpha;
        target[1] = red;
        target[2] = green;
        target[3] = blue;
        return 4;
    case zRND_TEX_FORMAT_ABGR_8888:
        target[0] = alpha;
        target[1] = blue;
        target[2] = green;
        target[3] = red;
        return 4;
    case zRND_TEX_FORMAT_RGBA_8888:
        target[0] = red;
        target[1] = green;
        target[2] = blue;
        target[3] = alpha;
        return 4;
    case zRND_TEX_FORMAT_BGRA_8888:
        target[0] = blue;
        target[1] = green;
        target[2] = red;
        target[3] = alpha;
        return 4;
    case zRND_TEX_FORMAT_RGB_888:
        target[0] = red;
        target[1] = green;
        target[2] = blue;
        return 3;
    case zRND_TEX_FORMAT_BGR_888:
        target[0] = blue;
        target[1] = green;
        target[2] = red;
        return 3;
    case zRND_TEX_FORMAT_ARGB_4444:
        *reinterpret_cast<unsigned short*>(target) = (alpha & 0xf0) << 8
            | (red & 0xf0) << 4 | (green & 0xf0) | blue >> 4;
        return 2;
    case zRND_TEX_FORMAT_ARGB_1555:
        *reinterpret_cast<unsigned short*>(target) = (alpha ? 0x8000 : 0)
            | (red & 0xf8) << 7 | (green & 0xf8) << 2 | blue >> 3;
        return 2;
    case zRND_TEX_FORMAT_RGB_565:
        *reinterpret_cast<unsigned short*>(target) = (red & 0xf8) << 8
            | (green & 0xfc) << 3 | blue >> 3;
        return 2;
    default:
        target[0] = red;
        return 1;
    }
}

zCTexConGeneric::zCTexConGeneric()
    : texinfo(), tempptr(0), ffi(0), alpha(0), average(0xffffffff), lockMode(0), modified(0),
      detailTextureMode(0)
{
    std::memset(pal, 0, sizeof(pal));
}

zCTexConGeneric::~zCTexConGeneric()
{
    delete[] tempptr;
    tempptr = 0;
    texinfo = zCTextureInfo();
}

zCTextureConvert::~zCTextureConvert()
{
}

int zCTexConGeneric::Lock(int mode)
{
    lockMode = mode;
    return 1;
}

int zCTexConGeneric::Unlock()
{
    if ((lockMode & 2) && modified && texinfo.texFormat != zRND_TEX_FORMAT_DXT3
        && texinfo.texFormat != zRND_TEX_FORMAT_DXT5) {
        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        float alpha = 0.0f;
        for (int y = 0; y < texinfo.sizeY; ++y) {
            for (int x = 0; x < texinfo.sizeX; ++x) {
                zVEC4 color = GetRGBAAtTexel(static_cast<float>(x), static_cast<float>(y));
                red += color.n[0];
                green += color.n[1];
                blue += color.n[2];
                alpha += color.n[3];
            }
        }
        float scale = 1.0f / (texinfo.sizeX * texinfo.sizeY);
        unsigned long r = static_cast<unsigned long>(red * scale);
        unsigned long g = static_cast<unsigned long>(green * scale);
        unsigned long b = static_cast<unsigned long>(blue * scale);
        unsigned long a = static_cast<unsigned long>(alpha * scale);
        texinfo.averageColor = (a << 24) | (r << 16) | (g << 8) | b;
        average = texinfo.averageColor;
    }
    modified = 0;
    return 1;
}

int zCTexConGeneric::SetTextureInfo(const zCTextureInfo& info)
{
    texinfo = info;
    if (info.sizeX * info.sizeY > 0 && !tempptr) {
        int bytes = static_cast<int>(info.sizeX * info.sizeY
            * texFormatInfoList[info.texFormat].bytesPerPixel);
        int total = 0;
        for (int i = 0; i <= info.numMipMap && bytes; ++i) {
            total += bytes;
            bytes >>= 2;
        }
        tempptr = new char[total];
        if (!tempptr)
            return 0;
    }
    return 1;
}

zCTextureInfo zCTexConGeneric::GetTextureInfo()
{
    return texinfo;
}

void* zCTexConGeneric::GetPaletteBuffer()
{
    if (lockMode & 2)
        modified = 1;
    return texinfo.texFormat == zRND_TEX_FORMAT_PAL_8 ? pal : 0;
}

int zCTexConGeneric::GetTextureBuffer(int mipMap, void*& data, int& pitch)
{
    data = 0;
    pitch = 0;
    if (lockMode & 2)
        modified = 1;
    if (texinfo.sizeX * texinfo.sizeY <= 0 || !tempptr || mipMap > texinfo.numMipMap)
        return 0;
    int bytes = static_cast<int>(texinfo.sizeX * texinfo.sizeY
        * texFormatInfoList[texinfo.texFormat].bytesPerPixel);
    int offset = 0;
    for (int i = 0; i < mipMap && bytes; ++i) {
        offset += bytes;
        bytes >>= 2;
    }
    data = tempptr + offset;
    pitch = static_cast<int>((texinfo.sizeX >> mipMap)
        * texFormatInfoList[texinfo.texFormat].bytesPerPixel);
    return 1;
}

int zCTexConGeneric::CopyTextureDataTo(int mipMap, void* destination, int pitch)
{
    void* source;
    int sourcePitch;
    if (!destination || !GetTextureBuffer(mipMap, source, sourcePitch))
        return 0;
    int height = texinfo.sizeY >> mipMap;
    for (int y = 0; y < height; ++y)
        std::memcpy(static_cast<unsigned char*>(destination) + y * pitch,
            static_cast<unsigned char*>(source) + y * sourcePitch, sourcePitch);
    return 1;
}

int zCTexConGeneric::CopyPaletteDataTo(void* destination)
{
    if (!destination)
        return 0;
    std::memcpy(destination, pal, sizeof(pal));
    return 1;
}

zCOLOR zCTexConGeneric::GetAverageColor()
{
    return texinfo.averageColor;
}

int zCTexConGeneric::HasAlpha()
{
    switch (texinfo.texFormat) {
    case zRND_TEX_FORMAT_ARGB_8888:
    case zRND_TEX_FORMAT_ABGR_8888:
    case zRND_TEX_FORMAT_RGBA_8888:
    case zRND_TEX_FORMAT_BGRA_8888:
    case zRND_TEX_FORMAT_ARGB_4444:
    case zRND_TEX_FORMAT_ARGB_1555:
    case zRND_TEX_FORMAT_DXT3:
        alpha = 1;
        break;
    default:
        alpha = 0;
        break;
    }
    return alpha;
}

void zCTexConGeneric::SetDetailTextureMode(int mode)
{
    detailTextureMode = mode;
}

int zCTexConGeneric::ConvertTextureFormat(const zCTextureInfo& info)
{
    if (!tempptr)
        return 0;
    if (info.texFormat == texinfo.texFormat && info.numMipMap == texinfo.numMipMap
        && info.sizeX == texinfo.sizeX && info.sizeY == texinfo.sizeY)
        return 1;
    int sourceBpp = static_cast<int>(texFormatInfoList[texinfo.texFormat].bytesPerPixel);
    int targetBpp = static_cast<int>(texFormatInfoList[info.texFormat].bytesPerPixel);
    if (sourceBpp < 1)
        sourceBpp = 1;
    if (targetBpp < 1)
        targetBpp = 1;
    int total = 0;
    for (int level = 0; level <= info.numMipMap; ++level)
        total += (info.sizeX >> level) * (info.sizeY >> level) * targetBpp;
    char* converted = new char[total];
    char* target = converted;
    char* source = tempptr;
    for (int level = 0; level <= info.numMipMap; ++level) {
        int sourceX = texinfo.sizeX >> level;
        int sourceY = texinfo.sizeY >> level;
        int targetX = info.sizeX >> level;
        int targetY = info.sizeY >> level;
        for (int y = 0; y < targetY; ++y) {
            int sourceRow = sourceY == targetY ? y : y * sourceY / targetY;
            for (int x = 0; x < targetX; ++x) {
                int sourceColumn = sourceX == targetX ? x : x * sourceX / targetX;
                const unsigned char* sourcePixel = reinterpret_cast<unsigned char*>(source)
                    + (sourceRow * sourceX + sourceColumn) * sourceBpp;
                unsigned char red;
                unsigned char green;
                unsigned char blue;
                unsigned char alpha;
                DecodeTexel(sourcePixel, texinfo.texFormat,
                    reinterpret_cast<unsigned char*>(pal), red, green, blue, alpha);
                target += EncodeTexel(reinterpret_cast<unsigned char*>(target), info.texFormat,
                    red, green, blue, alpha);
            }
        }
        source += sourceX * sourceY * sourceBpp;
    }
    delete[] tempptr;
    tempptr = converted;
    texinfo = info;
    alpha = HasAlpha();
    return 1;
}

void tcConvertTexture(RGBPIXEL* source, unsigned char* palette, unsigned char* target, int numPixels)
{
    for (int pixel = 0; pixel < numPixels; ++pixel) {
        int bestDistance = 0x7fffffff;
        int bestIndex = 0;
        for (int index = 0; index < 256; ++index) {
            int red = source[pixel].r - palette[index * 3];
            int green = source[pixel].g - palette[index * 3 + 1];
            int blue = source[pixel].b - palette[index * 3 + 2];
            int distance = red * red + green * green + blue * blue;
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = index;
            }
        }
        target[pixel] = bestIndex;
    }
}

void tcCreatePalette(RGBPIXEL* source, unsigned char* palette, int numPixels)
{
    struct zTPaletteBin {
        unsigned long count;
        unsigned long red;
        unsigned long green;
        unsigned long blue;
    };
    zTPaletteBin* bins = new zTPaletteBin[32768]();
    for (int i = 0; i < numPixels; ++i) {
        int index = (source[i].r >> 3) << 10 | (source[i].g >> 3) << 5 | source[i].b >> 3;
        ++bins[index].count;
        bins[index].red += source[i].r;
        bins[index].green += source[i].g;
        bins[index].blue += source[i].b;
    }
    for (int entry = 0; entry < 256; ++entry) {
        int best = 0;
        for (int i = 1; i < 32768; ++i) {
            if (bins[i].count > bins[best].count)
                best = i;
        }
        unsigned long count = bins[best].count;
        if (count) {
            palette[entry * 3] = bins[best].red / count;
            palette[entry * 3 + 1] = bins[best].green / count;
            palette[entry * 3 + 2] = bins[best].blue / count;
        } else {
            palette[entry * 3] = 0;
            palette[entry * 3 + 1] = 0;
            palette[entry * 3 + 2] = 0;
        }
        bins[best].count = 0;
    }
    delete[] bins;
}
