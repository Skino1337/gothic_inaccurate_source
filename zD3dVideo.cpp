#include "include/zRndD3d.h"
#include "include/zTexture.h"
#include <cmath>
#include <cstring>

static int dxDeviceModeNum[] = { 4 };
static zTRnd_VidModeInfo dxDeviceMode[64] = {
    { 640, 480, 16, 0 },
    { 800, 600, 16, 0 },
    { 1024, 768, 32, 0 },
    { 1280, 1024, 32, 0 }
};

void zCRnd_D3D::Vid_Blit(int, tagRECT*, tagRECT*)
{
    if (xd3d_tryDevnum)
        xd3d_tryDevnum = 0;
    xd3d_backlocked = 0;
    xd3d_tryDevnum = 1;
}

int zCRnd_D3D::Vid_Lock(zTRndSurfaceDesc& surface)
{
    if (xd3d_alphaBlendImmed)
        return 0;
    int bytesPerPixel = vid_bpp / 8;
    xd3d_frontPitch = vid_xdim * bytesPerPixel;
    if (!xd3d_frontPixels)
        xd3d_frontPixels = new unsigned char[xd3d_frontPitch * vid_ydim];
    surface.pSurface = xd3d_frontPixels;
    surface.bytePitch = xd3d_frontPitch;
    surface.pixelWidth = vid_xdim;
    surface.pixelHeight = vid_ydim;
    surface.bitRGBCount = vid_bpp;
    surface.bitRMask = vid_bpp == 16 ? 0xf800 : 0x00ff0000;
    surface.bitGMask = vid_bpp == 16 ? 0x07e0 : 0x0000ff00;
    surface.bitBMask = vid_bpp == 16 ? 0x001f : 0x000000ff;
    xd3d_pfrontBuffer = reinterpret_cast<IDirectDrawSurface7*>(xd3d_frontPixels);
    xd3d_alphaBlendImmed = 1;
    return 1;
}

int zCRnd_D3D::Vid_Unlock()
{
    if (!xd3d_alphaBlendImmed)
        return 0;
    xd3d_alphaBlendImmed = 0;
    return 1;
}

int zCRnd_D3D::Vid_GetFrontBufferCopy(zCTextureConvert& texture)
{
    zTRndSurfaceDesc surface;
    if (!Vid_Lock(surface))
        return 0;
    zCTextureInfo info = {};
    info.sizeX = surface.pixelWidth;
    info.sizeY = surface.pixelHeight;
    info.refSizeX = info.sizeX;
    info.refSizeY = info.sizeY;
    info.numMipMap = 0;
    info.texFormat = surface.bitRGBCount == 16 ? zRND_TEX_FORMAT_RGB_565
        : (surface.bitRGBCount == 24 ? zRND_TEX_FORMAT_BGR_888 : zRND_TEX_FORMAT_ARGB_8888);
    if (!texture.Lock(2) || !texture.SetTextureInfo(info)) {
        Vid_Unlock();
        return 0;
    }
    void* destination;
    int pitch;
    int result = texture.GetTextureBuffer(0, destination, pitch);
    if (result) {
        int rowSize = surface.pixelWidth * (surface.bitRGBCount / 8);
        const unsigned char* source = static_cast<const unsigned char*>(surface.pSurface);
        unsigned char* target = static_cast<unsigned char*>(destination);
        for (unsigned long y = 0; y < surface.pixelHeight; ++y) {
            std::memcpy(target, source, rowSize);
            source += surface.bytePitch;
            target += pitch;
        }
    }
    texture.Unlock();
    Vid_Unlock();
    return result;
}

void zCRnd_D3D::Vid_Clear(zCOLOR& color, int flags)
{
    if (!xd3d_frontPixels || flags == 2)
        return;
    int pixelSize = vid_bpp / 8;
    unsigned long value = color;
    for (int y = 0; y < vid_ydim; ++y) {
        unsigned char* pixel = xd3d_frontPixels + y * xd3d_frontPitch;
        for (int x = 0; x < vid_xdim; ++x) {
            std::memcpy(pixel, &value, pixelSize);
            pixel += pixelSize;
        }
    }
}

int zCRnd_D3D::Vid_GetNumDevices()
{
    return xd3d_actStatus.devmax;
}

int zCRnd_D3D::Vid_GetActiveDeviceNr()
{
    return xd3d_actStatus.devnum;
}

int zCRnd_D3D::Vid_SetDevice(int device)
{
    if (device == xd3d_actStatus.devnum)
        return 1;
    if (device < 0 || device >= xd3d_actStatus.devmax)
        return 0;
    xd3d_actStatus.devnum = device;
    return 1;
}

int zCRnd_D3D::Vid_GetDeviceInfo(zTRnd_DeviceInfo& info, int device)
{
    if (device < 0 || device >= xd3d_actStatus.devmax)
        return 0;
    info.deviceName = "Direct3D device";
    return 1;
}

int zCRnd_D3D::Vid_GetNumModes()
{
    return dxDeviceModeNum[xd3d_actStatus.devnum];
}

int zCRnd_D3D::Vid_GetModeInfo(zTRnd_VidModeInfo& info, int mode)
{
    if (mode < 0 || mode >= dxDeviceModeNum[xd3d_actStatus.devnum])
        return 0;
    info = dxDeviceMode[mode];
    info.fullscreenOnly = !xd3d_Capabilities.window;
    return 1;
}

int zCRnd_D3D::Vid_GetActiveModeNr()
{
    return xd3d_actStatus.modeNr;
}

int zCRnd_D3D::Vid_SetMode(int mode, HWND__** window)
{
    if (window)
        xd3d_newWinHandle = *window;
    int device = xd3d_actStatus.devnum;
    if (mode < 0 || mode >= dxDeviceModeNum[device])
        mode = dxDeviceModeNum[device] - 1;
    const zTRnd_VidModeInfo& info = dxDeviceMode[mode];
    if (!XD3D_SetMode(info.xdim, info.ydim, info.bpp))
        return 0;
    xd3d_actStatus.modeNr = mode;
    return 1;
}

int zCRnd_D3D::XD3D_SetMode(int xdim, int ydim, int bpp)
{
    vid_xdim = xdim;
    vid_ydim = ydim;
    vid_bpp = bpp;
    if (xd3d_scrStatus == xdim && xd3d_scrWidth == ydim && xd3d_scrHeight == bpp)
        return 1;
    xd3d_scrStatus = xdim;
    xd3d_scrWidth = ydim;
    xd3d_scrHeight = bpp;
    XD3D_CreateWin(xdim, ydim, bpp);
    if (!XD3D_SetDevice(xd3d_actStatus.devnum, xdim, ydim, bpp, xd3d_deviceChange))
        return 0;
    xd3d_scrMode = zRND_SCRMODE_WINDOWED;
    return 1;
}

void zCRnd_D3D::Vid_SetScreenMode(zTRnd_ScreenMode mode)
{
    if (mode == zRND_SCRMODE_HIDE)
        mode = zRND_SCRMODE_WINDOWED;
    if (mode == xd3d_deviceChange)
        return;
    if (XD3D_SetDevice(xd3d_actStatus.devnum, xd3d_scrStatus, xd3d_scrWidth,
            xd3d_scrHeight, mode)) {
        xd3d_deviceChange = mode;
        xd3d_scrMode = mode;
    }
    SetSurfaceLost(0);
}

zTRnd_ScreenMode zCRnd_D3D::Vid_GetScreenMode()
{
    return static_cast<zTRnd_ScreenMode>(xd3d_deviceChange);
}

void zCRnd_D3D::Vid_SetGammaCorrection(float contrast, float gamma, float brightness)
{
    if (!xd3d_Capabilities.gamma)
        return;
    xd3d_actStatus.gammavalue = contrast;
    if (contrast < 0.1f)
        contrast = 0.1f;
    else if (contrast > 0.9f)
        contrast = 0.9f;
    if (gamma < 0.1f)
        gamma = 0.1f;
    else if (gamma > 0.9f)
        gamma = 0.9f;
    if (brightness < 0.1f)
        brightness = 0.1f;
    else if (brightness > 0.9f)
        brightness = 0.9f;
    static unsigned short gammaRamp[3][256];
    float offset = (1.0f - gamma) * 0.4f + 0.3f;
    float scale = contrast * 0.8f + 0.1f;
    float bright = brightness * 0.4f + 0.3f;
    float denominator = 1.0f / (1.0f - offset);
    float exponent = 1.0f / (scale * scale * 3.0f + 0.25f);
    for (int i = 0; i < 256; ++i) {
        float value = i / 255.0f * denominator * offset + 0.5f - denominator * offset * 0.5f;
        if (value < 0.0f)
            value = 0.0f;
        else if (value > 1.0f)
            value = 1.0f;
        value = std::pow(value, exponent) + bright + bright - 1.0f;
        if (value < 0.0f)
            value = 0.0f;
        else if (value > 1.0f)
            value = 1.0f;
        unsigned short entry = static_cast<unsigned short>(value * 65535.0f);
        gammaRamp[0][i] = entry;
        gammaRamp[1][i] = entry;
        gammaRamp[2][i] = entry;
    }
}

float zCRnd_D3D::Vid_GetGammaCorrection()
{
    return xd3d_actStatus.gammavalue;
}
