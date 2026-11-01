#include "include/zGrafix.h"
#include <cstdio>
#include <cstring>

zCGfx* zCGfx::Gfx_List = NULL;
int zCGfx::gfxcount = 0;
int VideoRSize = 5;
int VideoGSize = 6;
int VideoBSize = 5;

static int readByteCount = 0;
static unsigned char* readByteCursor = NULL;
static unsigned char readByteCache[5000];

static int GfxBytesPerPixel()
{
    int result = VideoRSize + VideoGSize + VideoBSize;
    if (zrenderer->vid_bpp == 32)
        result += 8;
    return result;
}

zCGfx::zCGfx(const zSTRING& fileName)
    : data(NULL), next(NULL)
{
    zSTRING upperName = fileName.Upper();
    for (zCGfx* entry = Gfx_List; entry; entry = entry->next)
    {
        if (*entry->data->id == upperName)
        {
            data = entry->data;
            ++data->count;
            break;
        }
    }
    if (!data)
    {
        data = LoadGfx(upperName);
        if (!data)
            return;
        data->count = 1;
        data->id = new zSTRING(upperName);
        data->bpp = GfxBytesPerPixel();
    }
    next = Gfx_List;
    Gfx_List = this;
}

zCGfx::zCGfx(int width, int height)
    : data(NULL), next(NULL)
{
    ++gfxcount;
    data = new TGfx_Gfx;
    data->id = new zSTRING(gfxcount);
    data->bpp = GfxBytesPerPixel();
    data->sx = width;
    data->sy = height;
    data->count = 1;
    data->scr = new unsigned char[(zrenderer->vid_bpp >> 3) * width * height];
    std::memset(data->scr, 0, (zrenderer->vid_bpp >> 3) * width * height);
}

zCGfx::~zCGfx()
{
    if (data && --data->count <= 0)
    {
        delete data->id;
        delete[] data->scr;
        delete data;
    }
    zCGfx** link = &Gfx_List;
    while (*link && *link != this)
        link = &(*link)->next;
    if (*link)
        *link = next;
    data = NULL;
    next = NULL;
}

void zCGfx::Reload()
{
    int bpp = GfxBytesPerPixel();
    for (zCGfx* entry = Gfx_List; entry; entry = entry->next)
    {
        TGfx_Gfx* current = entry->data;
        if (current->bpp == bpp)
            continue;
        TGfx_Gfx* loaded = entry->LoadGfx(*current->id);
        delete[] current->scr;
        current->bpp = bpp;
        current->sx = loaded->sx;
        current->sy = loaded->sy;
        current->scr = loaded->scr;
        delete loaded;
    }
}

zCOLOR::zCOLOR(int value)
    : dword(value)
{
}

zCOLOR::zCOLOR(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
    : b(blue), g(green), r(red), a(alpha)
{
}

int rnd(float value)
{
    return int(value + (value < 0.0f ? -0.5f : 0.5f));
}

void Gfx_ChangeBPP(TGfx_Gfx* gfx)
{
}

TGfx_Gfx* zCGfx::LoadRaw(const zSTRING& fileName, int width, int height, int bpp)
{
    TGfx_Gfx* result = NULL;
    zFILE::DirectFileAccess(true);
    FILE* file = std::fopen(fileName.ToChar(), "rb");
    if (file)
    {
        result = new TGfx_Gfx;
        result->id = NULL;
        result->scr = new unsigned char[width * height];
        std::fread(result->scr, (bpp >> 3) * width * height, 1, file);
        std::fclose(file);
        result->sx = width;
        result->sy = height;
        result->bpp = zrenderer->vid_bpp;
        result->count = 0;
    }
    zFILE::DirectFileAccess(false);
    return result;
}

unsigned char zCGfx::Readbyte(zFILE* file)
{
    if (readByteCount < 0 || readByteCount >= 5000 || !readByteCursor)
    {
        file->Read(readByteCache, sizeof(readByteCache));
        readByteCount = 0;
        readByteCursor = readByteCache;
    }
    ++readByteCount;
    return *readByteCursor++;
}

int VideoRPos = 10;
int VideoGPos = 5;
int VideoBPos = 0;
unsigned char* Gfx_pal = NULL;

static TGfx_Gfx* CreateEmptyGfx()
{
    TGfx_Gfx* gfx = new TGfx_Gfx;
    gfx->id = NULL;
    gfx->bpp = 0;
    gfx->sx = 0;
    gfx->sy = 0;
    gfx->count = 0;
    gfx->scr = NULL;
    return gfx;
}

static void WriteGfxPixel(unsigned char*& target, int value)
{
    int bytes = zrenderer->vid_bpp >> 3;
    std::memcpy(target, &value, bytes);
    target += bytes;
}

TGfx_Gfx* zCGfx::LoadPcx(const zSTRING& fileName)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    if (!file->Exists())
    {
        delete file;
        return CreateEmptyGfx();
    }
    file->Open(0);
    file->Seek(8);
    unsigned short width = 0;
    unsigned short height = 0;
    file->Read(&width, sizeof(width));
    file->Read(&height, sizeof(height));
    TGfx_Gfx* result = CreateEmptyGfx();
    result->sx = width + 1;
    result->sy = height + 1;
    result->bpp = zrenderer->vid_bpp;
    int bytes = result->bpp >> 3;
    result->scr = new unsigned char[result->sx * result->sy * bytes];
    unsigned char palette[768];
    file->Seek(file->Size() - 768);
    file->Read(palette, sizeof(palette));
    Gfx_Setpal(palette);
    file->Seek(128);
    readByteCount = -1;
    readByteCursor = NULL;
    unsigned char* target = result->scr;
    int remaining = result->sx * result->sy;
    while (remaining > 0)
    {
        int run = 1;
        int index = Readbyte(file);
        if (index >= 0xC0)
        {
            run = index & 0x3F;
            index = Readbyte(file);
        }
        int pixel = index;
        if (result->bpp > 8 && index)
        {
            unsigned char* color = palette + index * 3;
            pixel = Gfx_Col(color[0], color[1], color[2]);
        }
        for (int i = 0; i < run && remaining > 0; ++i)
        {
            WriteGfxPixel(target, pixel);
            --remaining;
        }
    }
    file->Close();
    delete file;
    return result;
}

static int ReadTgaPixel(zCGfx* gfx, zFILE* file, int depth)
{
    if (depth == 16)
    {
        int low = gfx->Readbyte(file);
        int high = gfx->Readbyte(file);
        int value = low | high << 8;
        int red = (value >> 10) & 31;
        int green = (value >> 5) & 31;
        int blue = value & 31;
        return Gfx_Col(red << 3, green << 3, blue << 3);
    }
    int blue = gfx->Readbyte(file);
    int green = gfx->Readbyte(file);
    int red = gfx->Readbyte(file);
    if (depth == 32)
        gfx->Readbyte(file);
    return Gfx_Col(red, green, blue);
}

TGfx_Gfx* zCGfx::LoadTga(const zSTRING& fileName)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    TGfx_Gfx* result = CreateEmptyGfx();
    if (!file->Exists())
    {
        delete file;
        return result;
    }
    file->Open(0);
    unsigned char header[18];
    file->Read(header, sizeof(header));
    if (header[0])
        file->Seek(file->Pos() + header[0]);
    result->sx = header[12] | header[13] << 8;
    result->sy = header[14] | header[15] << 8;
    result->bpp = zrenderer->vid_bpp;
    int depth = header[16];
    int type = header[2];
    int bytes = result->bpp >> 3;
    result->scr = new unsigned char[result->sx * result->sy * bytes];
    std::memset(result->scr, 0, result->sx * result->sy * bytes);
    unsigned char palette[768];
    if (type == 1)
        file->Read(palette, sizeof(palette));
    int paletted = type == 1 && header[1] && depth == 8;
    if (!paletted && (header[1] || type != 2 && type != 10
        || depth != 16 && depth != 24 && depth != 32))
        zerr.Warning(fileName + "\nTGA-format not supported !");
    else
    {
        readByteCount = -1;
        readByteCursor = NULL;
        int* pixels = new int[result->sx * result->sy];
        int count = result->sx * result->sy;
        int current = 0;
        if (type == 1)
        {
            while (current < count)
            {
                int index = Readbyte(file);
                pixels[current++] = Gfx_Col(palette[index * 3], palette[index * 3 + 1],
                    palette[index * 3 + 2]);
            }
        }
        else if (type == 2)
        {
            while (current < count)
                pixels[current++] = ReadTgaPixel(this, file, depth);
        }
        else
        {
            while (current < count)
            {
                int packet = Readbyte(file);
                int run = (packet & 0x7F) + 1;
                if (packet & 0x80)
                {
                    int pixel = ReadTgaPixel(this, file, depth);
                    for (int i = 0; i < run && current < count; ++i)
                        pixels[current++] = pixel;
                }
                else
                {
                    for (int i = 0; i < run && current < count; ++i)
                        pixels[current++] = ReadTgaPixel(this, file, depth);
                }
            }
        }
        unsigned char* target = result->scr;
        for (int y = result->sy - 1; y >= 0; --y)
        {
            for (int x = 0; x < result->sx; ++x)
                WriteGfxPixel(target, pixels[y * result->sx + x]);
        }
        delete[] pixels;
    }
    file->Close();
    delete file;
    return result;
}

void zCGfx::SaveRaw(const zSTRING& fileName)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    file->Create();
    file->Write(data->scr, data->sx * data->sy * 2);
    file->Close();
    delete file;
}

void zCGfx::Save(const zSTRING& fileName)
{
    if (data->bpp != 32)
    {
        zerr.Warning("U:zGrafix: Save TGA only in 32bpp mode !");
        return;
    }
    unsigned char header[18] = {};
    header[2] = 2;
    header[12] = data->sx & 0xFF;
    header[13] = data->sx >> 8;
    header[14] = data->sy & 0xFF;
    header[15] = data->sy >> 8;
    header[16] = 32;
    zFILE* file = zfactory->CreateZFile(fileName);
    file->Create();
    file->Write(header, sizeof(header));
    for (int y = data->sy - 1; y >= 0; --y)
        file->Write(data->scr + y * data->sx * 4, data->sx * 4);
    file->Close();
    delete file;
}

TGfx_Gfx* zCGfx::LoadGfx(const zSTRING& fileName)
{
    if (fileName.Search(".PCX") >= 0)
        return LoadPcx(fileName);
    if (fileName.Search(".TEX") >= 0)
        return LoadRaw(fileName, 64, 64, 8);
    if (fileName.Search(".TGA") >= 0)
        return LoadTga(fileName);
    zerr.Message("U:(zGrafix) Failed.");
    return NULL;
}

int Gfx_Col(zCOLOR color)
{
    return Gfx_Col(color.r, color.g, color.b);
}

int Gfx_Col(int red, int green, int blue)
{
    if (zrenderer->vid_bpp == 8 && Gfx_pal)
    {
        int best = 0;
        int bestDistance = 0xFFFF;
        for (int i = 10; i < 246; ++i)
        {
            int distance = 0;
            int delta = red - Gfx_pal[i * 3];
            distance += delta < 0 ? -delta : delta;
            delta = green - Gfx_pal[i * 3 + 1];
            distance += delta < 0 ? -delta : delta;
            delta = blue - Gfx_pal[i * 3 + 2];
            distance += delta < 0 ? -delta : delta;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = i;
            }
        }
        return best;
    }
    return (red << VideoRSize >> 8 << VideoRPos) +
        (green << VideoGSize >> 8 << VideoGPos) +
        (blue << VideoBSize >> 8 << VideoBPos);
}

int zCGfx::GetSizex()
{
    return data ? data->sx : 0;
}

int zCGfx::GetSizey()
{
    return data ? data->sy : 0;
}

unsigned char* zCGfx::GetDataPtr()
{
    return data ? data->scr : NULL;
}

int zCGfx::GetPixel(int x, int y)
{
    if (!data)
        return 0;
    return ((unsigned short*)data->scr)[data->sx * y + x];
}

static unsigned short Gfx_Pal8to16[256];
static unsigned long Pal8to24[256];

void Gfx_Setpal(unsigned char* palette)
{
    for (int i = 0; i < 256; ++i)
    {
        Gfx_Pal8to16[i] = ((palette[i * 3] << 5 >> 8 << 5) +
            (palette[i * 3 + 1] << 5 >> 8) << 5) +
            (palette[i * 3 + 2] << 5 >> 8);
        Pal8to24[i] = (palette[i * 3] << 16) |
            (palette[i * 3 + 1] << 8) | palette[i * 3 + 2];
    }
    Gfx_pal = palette;
}

void Gfx_Loadpal(const char* fileName)
{
    if (!Gfx_pal)
        Gfx_pal = new unsigned char[1024];
    zFILE::DirectFileAccess(true);
    FILE* file = std::fopen(fileName, "rb");
    if (file)
    {
        std::fseek(file, 0, SEEK_END);
        long size = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);
        std::memset(Gfx_pal, 0, 1024);
        if (size == 768)
        {
            for (int i = 0; i < 256; ++i)
            {
                for (int channel = 0; channel < 3; ++channel)
                {
                    unsigned char value = 0;
                    std::fread(&value, 1, 1, file);
                    Gfx_pal[i * 4 + channel] = (value * 255 + 31) / 63;
                }
            }
        }
        std::fclose(file);
        Gfx_Setpal(Gfx_pal);
    }
    zFILE::DirectFileAccess(false);
}
