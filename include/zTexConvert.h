#ifndef ZTEXCONVERT_H
#define ZTEXCONVERT_H

#include "zTexture.h"

struct RGBPIXEL {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

class zCTexConGeneric : public zCTextureConvert {
public:

    zCTexConGeneric();
    virtual ~zCTexConGeneric();
    virtual int Lock(int mode);
    virtual int Unlock();
    virtual int SetTextureInfo(const zCTextureInfo& info);
    virtual void* GetPaletteBuffer();
    virtual int GetTextureBuffer(int mipMap, void*& data, int& pitch);
    virtual zCTextureInfo GetTextureInfo();
    virtual int CopyPaletteDataTo(void* destination);
    virtual int CopyTextureDataTo(int mipMap, void* destination, int pitch);
    virtual int HasAlpha();
    virtual int ConvertTextureFormat(const zCTextureInfo& info);
    virtual void SetDetailTextureMode(int mode);

    zCOLOR GetAverageColor();

    zCTextureInfo texinfo;
    char* tempptr;
    char pal[256 * 3];
    zCTextureFileFormatInternal* ffi;
    int alpha;
    zCOLOR average;
    int lockMode;
    int modified;
    int detailTextureMode;
};

void FreeTexConvert();
char* ResizeTexture(int* sourceDimensions, int targetX, int targetY, void* target,
    void* source, unsigned long format);
void tcConvertTexture(RGBPIXEL* source, unsigned char* palette, unsigned char* target, int numPixels);
void tcCreatePalette(RGBPIXEL* source, unsigned char* palette, int numPixels);

#endif
