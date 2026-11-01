#include "include/zViewDraw.h"

static int viewDrawState = -1;
static zCClassDef zCViewDrawClassDef("zCViewDraw", "zCObject", zCViewDraw::_CreateNewInstance,
    0, 0x7C, 0, "");
zCClassDef* zCViewDraw::classDef = &zCViewDrawClassDef;
static zCViewDraw* viewScreen = 0;

zCObject* zCViewDraw::_CreateNewInstance()
{
    zCViewDraw* view = new zCViewDraw;
    zCClassDef::ObjectCreated(view, classDef);
    return view;
}

void zCViewDraw::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCViewDraw::_GetClassDef() const
{
    return classDef;
}

zCViewDraw::zCViewDraw()
    : TextureFuncAlpha(1), Texture(0), TextureColor{ 255, 255, 255, 255 }, TextureAlpha(255)
{
    TexturePosition[0].n[0] = 0.0f;
    TexturePosition[0].n[1] = 0.0f;
    TexturePosition[1].n[0] = 0.0f;
    TexturePosition[1].n[1] = 0.0f;
}

zCViewDraw::~zCViewDraw()
{
    if (Texture)
        Texture->Release();
    Texture = 0;
}

void zCViewDraw::RemoveChild(zSTRING& name)
{
    for (int i = children.numInArray - 1; i >= 0; --i) {
        if (children.parray[i] && children.parray[i]->objectName == name) {
            zCViewObject* child = children.parray[i];
            children.Remove(child);
            delete child;
        }
    }
}

void zCViewDraw::SetTextureAlphaBlendFunc(const int& function)
{
    TextureFuncAlpha = function;
}

void zCViewDraw::SetTexture(const zSTRING& name)
{
    if (!name.Length())
        return;
    if (Texture) {
        Texture->Release();
        Texture = 0;
    }
    zSTRING loadName = name;
    Texture = zCTexture::Load(loadName, 1);
    textureName = name;
}

void zCViewDraw::SetTexture(zCTexture* texture)
{
    if (!texture)
        return;
    if (Texture)
        Texture->Release();
    Texture = texture;
    Texture->AddRef();
}

void zCViewDraw::SetTextureColor(const zCOLOR& color)
{
    TextureColor = color;
}

void zCViewDraw::SetTextureAlpha(int alpha)
{
    TextureAlpha = alpha;
}

void zCViewDraw::TextureCacheIn(float priority)
{
    if (!Texture)
        return;
    if ((Texture->cacheState & 3) == 3) {
        Texture->TouchTimeStamp();
        return;
    }
    Texture->TouchTimeStampLocal();
    zresMan->CacheIn(Texture, priority);
}

void zCViewDraw::Render()
{
    int oldFilter = zrenderer->GetBilerpFilterEnabled();
    int oldWrite = zrenderer->GetZBufferWriteEnabled();
    zTRnd_ZBufferCmp oldCompare = zrenderer->GetZBufferCompare();
    zTRnd_AlphaBlendFunc oldFunc = zrenderer->GetAlphaBlendFunc();
    int alpha = TextureAlpha;
    Blit();
    zrenderer->SetAlphaBlendFunc(oldFunc);
    zrenderer->SetZBufferCompare(oldCompare);
    zrenderer->SetZBufferWriteEnabled(oldWrite);
    zrenderer->SetBilerpFilterEnabled(oldFilter);
    TextureAlpha = alpha;
    DrawChildren();
}

void zCViewDraw::Draw()
{
    Render();
}

void zCViewDraw::DrawChildren()
{
    for (int i = children.numInArray - 1; i >= 0; --i) {
        zCViewDraw* child = dynamic_cast<zCViewDraw*>(children.parray[i]);
        if (child)
            child->Draw();
    }
}

void zCViewDraw::Blit()
{
    zrenderer->SetAlphaBlendFunc(static_cast<zTRnd_AlphaBlendFunc>(TextureFuncAlpha));
    zrenderer->SetZBufferCompare(zRND_ZBUFFER_CMP_ALWAYS);
    zrenderer->SetZBufferWriteEnabled(0);
    zrenderer->SetBilerpFilterEnabled(1);
    if (Texture)
        BlitTexture();
}

void zCViewDraw::BlitTexture()
{
    EvaluateTexturePosition();
    TextureColor.a = TextureAlpha;
    float z = zCCamera::activeCam ? zCCamera::activeCam->nearClipZ + 1.0f : 1.0f;
    zVEC2 uvTopLeft(0.0f, 0.0f);
    zVEC2 uvBottomRight(1.0f, 1.0f);
    zrenderer->DrawTile(Texture, TexturePosition[0], TexturePosition[1], z,
        uvTopLeft, uvBottomRight, TextureColor.dword);
}

void zCViewDraw::EvaluateTexturePosition()
{
    TexturePosition[0].n[0] = PixelPosition.X;
    TexturePosition[0].n[1] = PixelPosition.Y;
    TexturePosition[1].n[0] = PixelPosition.X + PixelSize.X;
    TexturePosition[1].n[1] = PixelPosition.Y + PixelSize.Y;
}

zCViewDraw& zCViewDraw::GetScreen()
{
    if (!viewScreen) {
        viewScreen = new zCViewDraw;
        zCClassDef::ObjectCreated(viewScreen, classDef);
        viewScreen->PixelSize = { 8192, 8192 };
    }
    return *viewScreen;
}

void zCViewDraw::CleanupScreen()
{
    delete viewScreen;
    viewScreen = 0;
}
