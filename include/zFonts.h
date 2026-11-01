#ifndef ZFONTS_H
#define ZFONTS_H

#include "zModel.h"

class zCTexture;

class zCTextureInfo {
public:

    int texFormat;
    int sizeX;
    int sizeY;
    int numMipMap;
    int refSizeX;
    int refSizeY;
    unsigned long averageColor;
};

class zCTextureConvert {
public:

    zCTextureInfo GetTextureInfo();
    int Lock(int mode);
    int Unlock();
    int GetTextureBuffer(int mipMap, void*& data, int& pitch);
};

const int zFONT_MAX_LETTER = 256;

class zCFont {
public:

    zCFont(const zSTRING& name);
    ~zCFont();
    int CacheIn();
    int LoadFontTexture(const zSTRING& fileName);
    int LoadSource(const zSTRING& fileName);
    int SaveFontData();
    int LoadFontData();
    zSTRING GetFontName();
    int GetFontY();
    int GetFontX(const zSTRING& text);
    int GetWidth(char character);
    int GetLetterDistance();
    char MapToAscii(char character);
    int GetSizex();
    int GetSizey();
    int GetPixel(int x, int y);
    int GetPixelAlpha(int x, int y);
    int NextPixelX(int x, int y, int& color);
    int NextPixelY(int x, int y, int& color);
    int NextPixelXAlpha(int x, int y, int& alpha);
    int NextPixelYAlpha(int x, int y, int& alpha);
    int NextPixelXCol(int x, int y, int color);
    int NextPixelYCol(int x, int y, int color);
    int AnalyseLetters();
    int GetFontData(unsigned char character, int& width, zVEC2& uv1, zVEC2& uv2);
    zCTexture* GetFontTexture();

    zSTRING name;
    int height;
    zCTexture* fontTex;
    unsigned char width[zFONT_MAX_LETTER];
    zVEC2 fontuv1[zFONT_MAX_LETTER];
    zVEC2 fontuv2[zFONT_MAX_LETTER];

    static zCTextureConvert* texConv;
};

class zCFontMan {
public:

    zCFontMan();
    ~zCFontMan();
    int SearchFont(const zSTRING& name);
    int Load(const zSTRING& name);
    int GetNumber();
    zCFont* GetFont(int index);

    zCArray<zCFont*> fontlist;
};

#endif
