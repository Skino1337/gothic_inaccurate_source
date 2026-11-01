#ifndef ZGRAFIX_H
#define ZGRAFIX_H

#include "zRenderer.h"
#include "zFile.h"

struct TGfx_Gfx {
    zSTRING* id;
    int bpp;
    int sx;
    int sy;
    int count;
    unsigned char* scr;
};

class zCGfx {
public:

    zCGfx(const zSTRING& fileName);
    zCGfx(int width, int height);
    ~zCGfx();
    TGfx_Gfx* LoadRaw(const zSTRING& fileName, int width, int height, int bpp);
    unsigned char Readbyte(zFILE* file);
    TGfx_Gfx* LoadPcx(const zSTRING& fileName);
    TGfx_Gfx* LoadTga(const zSTRING& fileName);
    void SaveRaw(const zSTRING& fileName);
    void Save(const zSTRING& fileName);
    TGfx_Gfx* LoadGfx(const zSTRING& fileName);
    int GetSizex();
    int GetSizey();
    unsigned char* GetDataPtr();
    int GetPixel(int x, int y);
    static void Reload();

    TGfx_Gfx* data;
    zCGfx* next;

    static zCGfx* Gfx_List;
    static int gfxcount;
};

extern int VideoRSize;
extern int VideoGSize;
extern int VideoBSize;
extern int VideoRPos;
extern int VideoGPos;
extern int VideoBPos;
extern unsigned char* Gfx_pal;

int rnd(float value);
int Gfx_Col(zCOLOR color);
int Gfx_Col(int red, int green, int blue);
void Gfx_Setpal(unsigned char* palette);
void Gfx_Loadpal(const char* fileName);
void Gfx_ChangeBPP(TGfx_Gfx* gfx);

#endif
