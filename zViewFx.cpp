#include "include/zViewFx.h"

extern zTIMER ztimer;

static int viewFxState = -1;
static zCClassDef zCViewFXClassDef("zCViewFX", "zCObject", zCViewFX::_CreateNewInstance,
    0, 0xAC, 0, "");
zCClassDef* zCViewFX::classDef = &zCViewFXClassDef;

zCObject* zCViewFX::_CreateNewInstance()
{
    zCViewFX* view = new zCViewFX;
    zCClassDef::ObjectCreated(view, classDef);
    return view;
}

void zCViewFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCViewFX::_GetClassDef() const
{
    return classDef;
}

zCViewFX::zCViewFX()
    : HasOpened(1), HasClosed(0), TimeOpen(0.0f), TimeClose(0.0f),
      DurationOpen(500.0f), DurationClose(500.0f), ModeOpen(2), ModeClose(2), baseAlpha(255.0f)
{
    TextureOffset[0].n[0] = 0.0f;
    TextureOffset[0].n[1] = 0.0f;
    TextureOffset[1].n[0] = 0.0f;
    TextureOffset[1].n[1] = 0.0f;
}

zCViewFX::~zCViewFX()
{
}

void zCViewFX::Init(zCViewObject* parent, int opened, unsigned long modeOpen,
    unsigned long modeClose, float durationOpen, float durationClose, zSTRING& texture)
{
    if (parent)
        parent->children.InsertEnd(this);
    TimeOpen = 0.0f;
    TimeClose = 0.0f;
    HasOpened = opened;
    HasClosed = !opened;
    ModeOpen = modeOpen;
    ModeClose = modeClose;
    DurationOpen = durationOpen;
    DurationClose = durationClose;
    if (texture.Length())
        SetTexture(texture);
}

void zCViewFX::Open()
{
    HasOpened = 0;
    HasClosed = 1;
    TimeClose = 0.0f;
    TimeOpen = ztimer.frameTimeFloat;
}

void zCViewFX::OpenSafe()
{
    float progress = 0.0f;
    if (TimeClose != 0.0f)
        progress = 1.0f - TimeClose;
    else if (HasOpened || TimeOpen != 0.0f)
        return;
    Open();
    TimeOpen = ztimer.frameTimeFloat / ztimer.factorMotion;
    if (progress != 0.0f)
        TimeOpen = progress;
}

void zCViewFX::OpenImmediately()
{
    HasOpened = 1;
    HasClosed = 0;
    TimeOpen = 0.0f;
    TimeClose = 0.0f;
}

void zCViewFX::Close()
{
    if (HasClosed || TimeClose > 0.0f)
        return;
    HasClosed = 0;
    HasOpened = 1;
    TimeOpen = 0.0f;
    TimeClose = ztimer.frameTimeFloat;
}

void zCViewFX::CloseSafe()
{
    if (TimeOpen > 0.0f)
        CloseImmediately();
    Close();
}

void zCViewFX::CloseImmediately()
{
    HasOpened = 0;
    HasClosed = 1;
    TimeOpen = 0.0f;
    TimeClose = 0.0f;
}

void zCViewFX::ApplyOpenCloseFX()
{
    if (TimeOpen > 0.0f)
        ApplyOpenFX();
    else if (TimeClose > 0.0f)
        ApplyCloseFX();
}

zVEC2& zVEC2::operator-=(const zVEC2& value)
{
    n[0] -= value.n[0];
    n[1] -= value.n[1];
    return *this;
}

zVEC2& zVEC2::operator*=(float value)
{
    n[0] *= value;
    n[1] *= value;
    return *this;
}

void zCViewFX::ApplyOpenFX()
{
    if (TimeOpen >= DurationOpen) {
        OpenImmediately();
        return;
    }
    if (ModeOpen & VIEW_FX_FADE)
        ApplyOpenFade();
    if (ModeOpen & VIEW_FX_ZOOM)
        ApplyOpenZoom();
    TimeOpen += ztimer.frameTimeFloat;
}

void zCViewFX::ApplyCloseFX()
{
    if (TimeClose >= DurationClose) {
        CloseImmediately();
        return;
    }
    if (ModeClose & VIEW_FX_FADE)
        ApplyCloseFade();
    if (ModeClose & VIEW_FX_ZOOM)
        ApplyCloseZoom();
    TimeClose += ztimer.frameTimeFloat;
}

void zCViewFX::ApplyOpenFade()
{
    TextureFuncAlpha = 2;
    TextureAlpha = static_cast<int>(baseAlpha * TimeOpen / DurationOpen);
}

void zCViewFX::ApplyOpenZoom()
{
    float scale = (1.0f - TimeOpen / DurationOpen) * 0.5f;
    float x = PixelSize.X * scale;
    float y = PixelSize.Y * scale;
    TextureOffset[0].n[0] += x;
    TextureOffset[0].n[1] += y;
    TextureOffset[1].n[0] -= x;
    TextureOffset[1].n[1] -= y;
}

void zCViewFX::ApplyCloseFade()
{
    TextureFuncAlpha = 2;
    TextureAlpha = static_cast<int>(baseAlpha * (1.0f - TimeClose / DurationClose));
}

void zCViewFX::ApplyCloseZoom()
{
    float scale = TimeClose / DurationClose * 0.5f;
    float x = PixelSize.X * scale;
    float y = PixelSize.Y * scale;
    TextureOffset[0].n[0] += x;
    TextureOffset[0].n[1] += y;
    TextureOffset[1].n[0] -= x;
    TextureOffset[1].n[1] -= y;
}

void zCViewFX::Blit()
{
    TextureOffset[0].n[0] = 0.0f;
    TextureOffset[0].n[1] = 0.0f;
    TextureOffset[1].n[0] = 0.0f;
    TextureOffset[1].n[1] = 0.0f;
    ApplyOpenCloseFX();
    if (HasOpened || TimeOpen > 0.0f || TimeClose > 0.0f)
        zCViewDraw::Blit();
}

void zCViewFX::EvaluateTexturePosition()
{
    zCViewDraw::EvaluateTexturePosition();
    TexturePosition[0] += TextureOffset[0];
    TexturePosition[1] += TextureOffset[1];
}
