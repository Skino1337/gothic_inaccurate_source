#include "include/zFonts.h"
#include "include/oGame.h"
#include "include/zFile.h"
#include <cstring>

zCTextureConvert* zCFont::texConv = NULL;

zCFontMan::zCFontMan()
{
}

zCFontMan::~zCFontMan()
{
    for (int i = 0; i < fontlist.numInArray; ++i)
        delete fontlist[i];
}

int zCFontMan::SearchFont(const zSTRING& name)
{
    zSTRING upperName = name.Upper();
    for (int i = 0; i < fontlist.numInArray; ++i)
    {
        if (fontlist[i]->GetFontName() == upperName)
            return i;
    }
    return -1;
}

int zCFontMan::Load(const zSTRING& name)
{
    zSTRING upperName = name.Upper();
    int index = SearchFont(upperName);
    if (index >= 0)
        return index;
    zCFont* font = new zCFont(upperName);
    font->LoadSource(upperName);
    fontlist.InsertEnd(font);
    return fontlist.numInArray - 1;
}

int zCFontMan::GetNumber()
{
    return fontlist.numInArray;
}

zCFont* zCFontMan::GetFont(int index)
{
    if (index >= fontlist.numInArray)
        return NULL;
    return fontlist[index];
}

int zCFont::CacheIn()
{
    if (!fontTex)
        return 0;
    if ((fontTex->cacheState & 3) == 3)
    {
        fontTex->TouchTimeStamp();
        return 1;
    }
    fontTex->TouchTimeStampLocal();
    fontTex->CacheIn(-1.0f);
    return 1;
}

int zCFont::LoadFontTexture(const zSTRING& fileName)
{
    zSTRING textureName = fileName;
    fontTex = zCTexture::Load(textureName);
    if (!fontTex)
        return 0;
    if (!LoadFontData())
    {
        texConv = zrenderer->CreateTextureConvert();
        zoptions->ChangeDir(12);
        zCTextureFileFormatTGA* format = new zCTextureFileFormatTGA;
        int converted = format->LoadTexture(fileName, texConv);
        delete format;
        if (!converted)
            zerr.Warning("U: FNT: Cannot convert Font " + fileName);
        else
            AnalyseLetters();
        delete texConv;
        texConv = NULL;
        SaveFontData();
        return converted;
    }
    return 1;
}

zCFont::zCFont(const zSTRING& fontName)
    : name(fontName), height(0), fontTex(NULL)
{
    std::memset(width, 0, sizeof(width));
    std::memset(fontuv1, 0, sizeof(fontuv1));
    std::memset(fontuv2, 0, sizeof(fontuv2));
    name.Upper();
    LoadFontTexture(name);
}

zCFont::~zCFont()
{
    if (fontTex)
    {
        fontTex->Release();
        fontTex = NULL;
    }
}

int zCFont::LoadSource(const zSTRING& fileName)
{
    name = fileName;
    name.Upper();
    return LoadFontTexture(name);
}

int zCFont::SaveFontData()
{
    zFILE* file = zfactory->CreateZFile(name);
    if (file->GetExt() != "TGA")
    {
        zerr.Warning("B: FONT: Name not correct: " + name);
        delete file;
        return 0;
    }
    file->SetExt("FNT");
    file->SetDir(zoptions->GetDirString(20));
    file->Create();
    file->Write("1\n");
    file->Write(name + "\n");
    file->Write(&height, sizeof(height));
    int letters = zFONT_MAX_LETTER;
    file->Write(&letters, sizeof(letters));
    file->Write(width, sizeof(width));
    file->Write(fontuv1, sizeof(fontuv1));
    file->Write(fontuv2, sizeof(fontuv2));
    file->Close();
    delete file;
    return 0;
}

int zCFont::LoadFontData()
{
    zFILE* file = zfactory->CreateZFile(name);
    if (file->GetExt() != "TGA")
    {
        zerr.Warning("B: FONT: Name not correct: " + name);
        delete file;
        return 0;
    }
    file->SetExt("FNT");
    file->SetDir(zoptions->GetDirString(20));
    int loaded = 0;
    if (file->Exists() && !file->Open(0))
    {
        zSTRING version;
        file->Read(version);
        if (version.ToInt() == 1)
        {
            file->Read(name);
            file->Read(&height, sizeof(height));
            int letters = 0;
            file->Read(&letters, sizeof(letters));
            if (letters == zFONT_MAX_LETTER)
            {
                file->Read(width, sizeof(width));
                file->Read(fontuv1, sizeof(fontuv1));
                file->Read(fontuv2, sizeof(fontuv2));
                loaded = 1;
            }
            else
                zerr.Warning("B: FONT: Wrong format of fontdata-file.");
        }
        file->Close();
    }
    delete file;
    return loaded;
}

zSTRING zCFont::GetFontName()
{
    return name;
}

int zCFont::GetFontY()
{
    return height;
}

int zCFont::GetFontX(const zSTRING& text)
{
    int result = 0;
    const char* current = text.ToChar();
    while (current && *current)
        result += width[(unsigned char)*current++] + 1;
    return result;
}

int zCFont::GetWidth(char character)
{
    return width[(unsigned char)character];
}

int zCFont::GetLetterDistance()
{
    return 1;
}

char zCFont::MapToAscii(char character)
{
    switch ((unsigned char)character)
    {
    case 0xC4:
        return (char)0x8E;
    case 0xD6:
        return (char)0x99;
    case 0xDC:
        return (char)0x9A;
    case 0xDF:
        return (char)0xE1;
    case 0xE4:
        return (char)0x84;
    case 0xF6:
        return (char)0x94;
    case 0xFC:
        return (char)0x81;
    default:
        return character;
    }
}

int zCFont::GetSizex()
{
    if (!texConv)
        return 0;
    return texConv->GetTextureInfo().sizeX;
}

int zCFont::GetSizey()
{
    if (!texConv)
        return 0;
    return texConv->GetTextureInfo().sizeY;
}

int zCFont::GetPixel(int x, int y)
{
    if (!texConv)
        return 0;
    if (!texConv->Lock(1))
    {
        zerr.Warning("C: zCFont :: GetPixel(): could not lock texture");
        return 0;
    }
    void* data = NULL;
    int pitch = 0;
    texConv->GetTextureBuffer(0, data, pitch);
    unsigned long pixel = ((unsigned long*)data)[y * (pitch >> 2) + x];
    texConv->Unlock();
    return pixel & 0xFFFFFF00;
}

int zCFont::GetPixelAlpha(int x, int y)
{
    if (!texConv)
        return 0;
    if (!texConv->Lock(1))
    {
        zerr.Warning("C: zCFont :: GetPixelAlpha(): could not lock texture");
        return 0;
    }
    void* data = NULL;
    int pitch = 0;
    texConv->GetTextureBuffer(0, data, pitch);
    unsigned long pixel = ((unsigned long*)data)[y * (pitch >> 2) + x];
    texConv->Unlock();
    return pixel & 0xFF;
}

int zCFont::NextPixelX(int x, int y, int& color)
{
    while (x < GetSizex())
    {
        color = GetPixel(x, y);
        if (color)
            return x;
        ++x;
    }
    return -1;
}

int zCFont::NextPixelY(int x, int y, int& color)
{
    while (y < GetSizey())
    {
        color = GetPixel(x, y);
        if (color)
            return y;
        ++y;
    }
    return -1;
}

int zCFont::NextPixelXAlpha(int x, int y, int& alpha)
{
    while (x < GetSizex())
    {
        alpha = GetPixelAlpha(x, y);
        if (alpha)
            return x;
        ++x;
    }
    return -1;
}

int zCFont::NextPixelYAlpha(int x, int y, int& alpha)
{
    while (y < GetSizey())
    {
        alpha = GetPixelAlpha(x, y);
        if (alpha)
            return y;
        ++y;
    }
    return -1;
}

int zCFont::NextPixelXCol(int x, int y, int color)
{
    while (x < GetSizex())
    {
        if (GetPixel(x, y) == color)
            return x;
        ++x;
    }
    return -1;
}

int zCFont::NextPixelYCol(int x, int y, int color)
{
    while (y < GetSizey())
    {
        if (GetPixel(x, y) == color)
            return y;
        ++y;
    }
    return -1;
}

int zCFont::AnalyseLetters()
{
    int textureWidth = GetSizex();
    int textureHeight = GetSizey();
    if (!textureWidth || !textureHeight)
        return 0;
    int borderColor;
    int left = NextPixelX(50, 50, borderColor);
    int top = NextPixelY(50, 50, borderColor);
    if (left < 0 || top < 0)
        return 0;
    int right = left + 1;
    while (right < textureWidth && GetPixel(right, top) == borderColor)
        ++right;
    int bottom = top + 1;
    while (bottom < textureHeight && GetPixel(left, bottom) == borderColor)
        ++bottom;
    int cellWidth = right - left;
    int cellHeight = bottom - top;
    if (cellWidth <= 0 || cellHeight <= 0)
        return 0;
    int columns = (textureWidth - left) / cellWidth;
    int rows = (textureHeight - top) / cellHeight;
    if (columns <= 0 || rows <= 0)
        return 0;
    height = 0;
    for (int letter = 0; letter < zFONT_MAX_LETTER; ++letter)
    {
        int cellX = left + (letter % columns) * cellWidth;
        int cellY = top + (letter / columns) * cellHeight;
        int minimumX = cellWidth;
        int minimumY = cellHeight;
        int maximumX = -1;
        int maximumY = -1;
        for (int y = 0; y < cellHeight && cellY + y < textureHeight; ++y)
        {
            for (int x = 0; x < cellWidth && cellX + x < textureWidth; ++x)
            {
                if (!GetPixel(cellX + x, cellY + y))
                    continue;
                if (x < minimumX)
                    minimumX = x;
                if (y < minimumY)
                    minimumY = y;
                if (x > maximumX)
                    maximumX = x;
                if (y > maximumY)
                    maximumY = y;
            }
        }
        if (maximumX < minimumX || maximumY < minimumY)
        {
            width[letter] = 0;
            fontuv1[letter].n[0] = 0.0f;
            fontuv1[letter].n[1] = 0.0f;
            fontuv2[letter].n[0] = 0.0f;
            fontuv2[letter].n[1] = 0.0f;
            continue;
        }
        width[letter] = maximumX - minimumX + 1;
        if (maximumY - minimumY + 1 > height)
            height = maximumY - minimumY + 1;
        fontuv1[letter].n[0] = float(cellX + minimumX) / textureWidth;
        fontuv1[letter].n[1] = float(cellY + minimumY) / textureHeight;
        fontuv2[letter].n[0] = float(cellX + maximumX + 1) / textureWidth;
        fontuv2[letter].n[1] = float(cellY + maximumY + 1) / textureHeight;
    }
    return 1;
}

int zCFont::GetFontData(unsigned char character, int& glyphWidth, zVEC2& uv1, zVEC2& uv2)
{
    if (character >= zFONT_MAX_LETTER)
        return 0;
    glyphWidth = width[character];
    uv1 = fontuv1[character];
    uv2 = fontuv2[character];
    return 1;
}

zCTexture* zCFont::GetFontTexture()
{
    return fontTex;
}
