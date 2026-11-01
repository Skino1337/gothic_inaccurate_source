#include "include/zRndD3d.h"
#include "include/zResource.h"
#include "include/zTexture.h"
#include <cstring>

zCTex_D3D::zCTex_D3D()
    : xtex_textureflag(0xdeadface), xtex_locked(0), xtex_miplocked(0), xtex_texinfo(),
      xtex_pPalettePtr(0), xtex_internalnumber(0), xtex_alphaTex(0), xtex_lastFrameUsed(0),
      xtex_buffer(0)
{
    std::memset(xtex_pddtex, 0, sizeof(xtex_pddtex));
    std::memset(xtex_pddtemporarytex, 0, sizeof(xtex_pddtemporarytex));
    std::memset(xtex_buffers, 0, sizeof(xtex_buffers));
    std::memset(xtex_pitches, 0, sizeof(xtex_pitches));
}

zCTex_D3D::~zCTex_D3D()
{
    CacheOut();
}

void zCTex_D3D::ReleaseData()
{
    if (xtex_textureflag != 0xdeadface) {
        zerr->Message("X: zCTex_D3D::ReleaseData: uninitialized pointer called");
        return;
    }
    RemoveTexture();
    for (int i = 0; i < 12; ++i) {
        delete[] xtex_buffers[i];
        xtex_buffers[i] = 0;
        xtex_pddtex[i] = 0;
        xtex_pddtemporarytex[i] = 0;
        xtex_pitches[i] = 0;
    }
    delete[] xtex_pPalettePtr;
    xtex_pPalettePtr = 0;
    xtex_buffer = 0;
    xtex_internalnumber = -1;
}

int zCTex_D3D::Lock(int)
{
    if ((cacheState & 3) != 3)
        zresMan->CacheIn(this, -1.0f);
    else
        TouchTimeStamp();
    xtex_locked = 1;
    return 1;
}

int zCTex_D3D::Unlock()
{
    xtex_miplocked = 0;
    xtex_locked = 0;
    cacheState = 3;
    return 1;
}

zCTextureInfo zCTex_D3D::GetTextureInfo()
{
    if ((cacheState & 3) != 3)
        zresMan->CacheIn(this, -1.0f);
    else
        TouchTimeStamp();
    return xtex_texinfo;
}

int zCTex_D3D::SetTextureInfo(const zCTextureInfo& info)
{
    if (xtex_textureflag != 0xdeadface) {
        zerr->Message("X: zCTex_D3D::SetTextureInfo: uninitialized pointer called");
        return 0;
    }
    xtex_texinfo = info;
    if (info.sizeX && info.sizeY)
        return 1;
    zerr->Message("X: CacheIn: WARNING: texture has no size....abort action!!!");
    return 0;
}

void zCTex_D3D::GetPixelSize(int& sizeX, int& sizeY)
{
    if (xtex_textureflag != 0xdeadface) {
        zerr->Message("X: zCTex_D3D::GetPixelSize: uninitialized pointer called");
        return;
    }
    sizeX = xtex_texinfo.sizeX;
    sizeY = xtex_texinfo.sizeY;
}

int zCTex_D3D::CopyTextureDataTo(int mipMap, void* destination, int pitch)
{
    if (xtex_textureflag != 0xdeadface || !xtex_locked || !xtex_buffers[mipMap])
        return 0;
    int width = xtex_texinfo.sizeX >> mipMap;
    int height = xtex_texinfo.sizeY >> mipMap;
    int bytesPerPixel = static_cast<int>(texFormatInfoList[xtex_texinfo.texFormat].bytesPerPixel);
    int rowSize = width * bytesPerPixel;
    const unsigned char* source = xtex_buffers[mipMap];
    unsigned char* target = static_cast<unsigned char*>(destination);
    for (int y = 0; y < height; ++y) {
        std::memcpy(target, source, rowSize);
        source += xtex_pitches[mipMap];
        target += pitch;
    }
    xtex_miplocked |= 1 << mipMap;
    return 1;
}

int zCTex_D3D::CopyPaletteDataTo(void* destination)
{
    if (xtex_textureflag != 0xdeadface || !destination || !xtex_pPalettePtr)
        return 0;
    std::memcpy(destination, xtex_pPalettePtr, 0x300);
    return 1;
}

void* zCTex_D3D::GetPaletteBuffer()
{
    if (xtex_textureflag != 0xdeadface || xtex_texinfo.texFormat != zRND_TEX_FORMAT_PAL_8)
        return 0;
    if (!xtex_pPalettePtr)
        xtex_pPalettePtr = new unsigned char[0x300];
    return xtex_pPalettePtr;
}

int zCTex_D3D::GetTextureBuffer(int mipMap, void*& data, int& pitch)
{
    data = 0;
    pitch = 0;
    if (xtex_textureflag != 0xdeadface || !xtex_locked)
        return 0;
    if ((xtex_texinfo.texFormat == zRND_TEX_FORMAT_DXT1 || xtex_texinfo.texFormat == zRND_TEX_FORMAT_DXT3)
        && ((xtex_texinfo.sizeX >> mipMap) < 4 || (xtex_texinfo.sizeY >> mipMap) < 4))
        return 0;
    if (!xtex_buffers[mipMap]) {
        int width = xtex_texinfo.sizeX >> mipMap;
        int height = xtex_texinfo.sizeY >> mipMap;
        int bytesPerPixel = static_cast<int>(texFormatInfoList[xtex_texinfo.texFormat].bytesPerPixel);
        xtex_pitches[mipMap] = width * bytesPerPixel;
        xtex_buffers[mipMap] = new unsigned char[xtex_pitches[mipMap] * height];
    }
    data = xtex_buffers[mipMap];
    pitch = xtex_pitches[mipMap];
    xtex_buffer = data;
    xtex_miplocked |= 1 << mipMap;
    return 1;
}

int zCTex_D3D::XTEX_BuildSurfaces(int)
{
    xtex_alphaTex = xtex_texinfo.texFormat == zRND_TEX_FORMAT_ARGB_4444
        || xtex_texinfo.texFormat == zRND_TEX_FORMAT_ARGB_1555
        || xtex_texinfo.texFormat == zRND_TEX_FORMAT_DXT2
        || xtex_texinfo.texFormat == zRND_TEX_FORMAT_DXT3
        || xtex_texinfo.texFormat == zRND_TEX_FORMAT_DXT4
        || xtex_texinfo.texFormat == zRND_TEX_FORMAT_DXT5;
    int numLevels = xtex_texinfo.numMipMap + 1;
    for (int i = 0; i < numLevels; ++i) {
        if (!xtex_buffers[i]) {
            int width = xtex_texinfo.sizeX >> i;
            int height = xtex_texinfo.sizeY >> i;
            int bytesPerPixel = static_cast<int>(texFormatInfoList[xtex_texinfo.texFormat].bytesPerPixel);
            xtex_pitches[i] = width * bytesPerPixel;
            xtex_buffers[i] = new unsigned char[xtex_pitches[i] * height];
        }
        xtex_pddtex[i] = reinterpret_cast<IDirectDrawSurface7*>(xtex_buffers[i]);
    }
    if (xtex_texinfo.texFormat == zRND_TEX_FORMAT_PAL_8 && !xtex_pPalettePtr)
        xtex_pPalettePtr = new unsigned char[0x300];
    return 1;
}

void zCTex_D3D::InsertTexture(int stage, int)
{
    if (xtex_textureflag != 0xdeadface) {
        zerr->Message("X: zCTex_D3D::InsertTexture: uninitialized pointer called");
        return;
    }
    zCRnd_D3D* renderer = static_cast<zCRnd_D3D*>(zrenderer);
    if (!xtex_texinfo.sizeX || !xtex_texinfo.sizeY || !xtex_pddtex[0]) {
        renderer->XD3D_SetTexture(stage, 0);
        renderer->xd3d_ptexture[stage] = 0;
        return;
    }
    if (renderer->XD3D_SetTexture(stage, xtex_pddtex[0]))
        renderer->xd3d_ptexture[stage] = this;
    static unsigned long frameNumber;
    if (xtex_lastFrameUsed != frameNumber) {
        ++renderer->xd3d_statistics.numTexturesUsed;
        xtex_lastFrameUsed = frameNumber;
    }
}

int zCTex_D3D::PreloadTexture()
{
    return xtex_pddtex[0] != 0;
}

int zCTex_D3D::SetTexture(int stage)
{
    zCRnd_D3D* renderer = static_cast<zCRnd_D3D*>(zrenderer);
    if (!xtex_texinfo.sizeX || !xtex_texinfo.sizeY || !xtex_pddtex[0]) {
        if (!renderer->XD3D_SetTexture(stage, 0))
            return 0;
        renderer->xd3d_ptexture[stage] = 0;
        return 1;
    }
    if (!renderer->XD3D_SetTexture(stage, xtex_pddtex[0]))
        return 0;
    renderer->xd3d_ptexture[stage] = this;
    return 1;
}

int zCTex_D3D::RemoveTexture()
{
    if (xtex_textureflag != 0xdeadface) {
        zerr->Message("X: zCTex_D3D::RemoveTexture: uninitialized pointer called");
        return 0;
    }
    if (!xtex_pddtex[0])
        return 0;
    for (int i = 0; i < 12; ++i)
        xtex_pddtex[i] = 0;
    delete[] xtex_pPalettePtr;
    xtex_pPalettePtr = 0;
    return 1;
}

int zCTex_D3D::RescueTexture(zD3D_savedTexture* saved)
{
    cacheState &= ~4;
    saved->texCon = zrenderer->CreateTextureConvert();
    return CopyContents(this, saved->texCon) != 0;
}

int zCTex_D3D::RebuildTexture(zD3D_savedTexture* saved)
{
    zCTextureInfo info = saved->texCon->GetTextureInfo();
    if (!zrenderer->SupportsTextureFormat(info.texFormat)) {
        zTRnd_TextureFormat format = zCTexture::CalcNextBestTexFormat(info.texFormat);
        saved->texCon->ConvertToNewFormat(format);
    }
    if (CopyContents(saved->texCon, this)) {
        cacheState = (cacheState & ~4) | 4;
        return 1;
    }
    delete saved->texCon;
    saved->texCon = 0;
    return 0;
}
