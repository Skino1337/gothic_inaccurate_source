#include "include/zViewBase.h"

// EXE: 0x008D2A9C
static int viewState = -1;

// EXE: 0x0083BB40
const unsigned short zCViewBase::ViewVirtualWidth = 8192;
// EXE: 0x0083BB42
const unsigned short zCViewBase::ViewVirtualHeight = 8192;

// EXE: 0x0068DEE0
zCViewBase::zCViewBase()
{
}

// EXE: 0x0068DF70
zCViewBase::~zCViewBase()
{
}

// EXE: 0x0068DEF0
int zCViewBase::anx(int x)
{
    return 0;
}

// EXE: 0x0068DF00
int zCViewBase::any(int y)
{
    return 0;
}

// EXE: 0x0068DF10
int zCViewBase::nax(int x)
{
    return 0;
}

// EXE: 0x0068DF20
int zCViewBase::nay(int y)
{
    return 0;
}

// EXE: 0x0068DF30
int zCViewBase::ClipLine(int& x1, int& y1, int& x2, int& y2)
{
    return 0;
}

// EXE: 0x0068DF40
void zCViewBase::Line(int x1, int y1, int x2, int y2, const zCOLOR& color)
{
}

// EXE: 0x0068DF50
void zCViewBase::GetViewport(int& x1, int& y1, int& x2, int& y2)
{
}

// EXE: 0x0068DF80
void zCViewBase::FillZBuffer()
{
    // EXE: 0x00AAD12C
    static zCTexture* fillTexture = 0;
    if (!fillTexture) {
        zSTRING textureName = "default.tga";
        fillTexture = zCTexture::Load(textureName, 0);
    }
    int x;
    int y;
    int width;
    int height;
    GetViewport(x, y, width, height);
    zTRnd_ZBufferCmp oldCompare = zrenderer->GetZBufferCompare();
    zTRnd_AlphaBlendSource oldSource = zrenderer->GetAlphaBlendSource();
    zTRnd_AlphaBlendFunc oldFunc = zrenderer->GetAlphaBlendFunc();
    float oldFactor = zrenderer->GetAlphaBlendFactor();
    int oldWrite = zrenderer->GetZBufferWriteEnabled();
    zrenderer->SetZBufferCompare(zRND_ZBUFFER_CMP_ALWAYS);
    zrenderer->SetZBufferWriteEnabled(1);
    zrenderer->SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
    zrenderer->SetAlphaBlendSource(zRND_ALPHA_SOURCE_CONSTANT);
    zrenderer->SetAlphaBlendFactor(0.0f);
    float z = zCCamera::activeCam ? zCCamera::activeCam->farClipZ - 1.0f : 65534.0f;
    zVEC2 topLeft(x, y);
    zVEC2 bottomRight(x + width, y + height);
    zVEC2 uvTopLeft(0.0f, 0.0f);
    zVEC2 uvBottomRight(1.0f, 1.0f);
    zrenderer->DrawTile(fillTexture, topLeft, bottomRight, z, uvTopLeft, uvBottomRight, 0);
    zrenderer->SetZBufferCompare(oldCompare);
    zrenderer->SetZBufferWriteEnabled(oldWrite);
    zrenderer->SetAlphaBlendSource(oldSource);
    zrenderer->SetAlphaBlendFactor(oldFactor);
    zrenderer->SetAlphaBlendFunc(oldFunc);
}

// EXE: 0x0068DF60
int zCViewBase::GetCode(int x, int y)
{
    return 0;
}
