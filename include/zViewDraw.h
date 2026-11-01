#ifndef ZVIEWDRAW_H
#define ZVIEWDRAW_H

#include "oViewDocument.h"
#include "zModel.h"

class zCViewObject : public zCObject {
public:

    zCViewObject();
    virtual ~zCViewObject();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    static int Compare(zCViewObject* first, zCViewObject* second);
    void AddChild(zCViewObject* child);
    void AddChild(zCViewObject& child);
    void AddChildren(zCListSort<zCViewObject>& list);
    void RemoveChild(zCViewObject* child);
    void RemoveChild(zCViewObject& child);
    void RemoveAllChildren();
    const zCViewObject* GetChild(unsigned long id);
    void SetVirtualPosition(zCPosition& position);
    void SetPixelPosition(zCPosition& position);
    void SetVirtualSize(zCPosition& size);
    void SetPixelSize(zCPosition& size);
    void EvaluatePixelPosition(zCPosition& position);
    void EvaluatePixelSize(zCPosition& size);
    void UpdatePixelPosition();
    void UpdatePixelSize();
    void UpdatePixelPositionChildren();
    void UpdatePixelSizeChildren();
    void UpdatePixelDimensionsChildren();
    zCPosition ConvertPositionVirtualToPixel(zCPosition& position);
    zCPosition ConvertSizeVirtualToPixel(zCPosition& size);
    zCPosition ConvertPositionPixelToVirtual(zCPosition& position);
    zCPosition ConvertSizePixelToVirtual(zCPosition& size);
    zCPosition ConvertToPixel(zCPosition& position);
    zCPosition ConvertToPixelLocal(zCPosition& position);
    zCPosition ConvertToVirtualLocal(zCPosition& position);
    virtual int anx(int x);
    virtual int any(int y);
    virtual int nax(int x);
    virtual int nay(int y);
    virtual int ClipLine(int& x1, int& y1, int& x2, int& y2);
    virtual void Line(int x1, int y1, int x2, int y2, const zCOLOR& color);
    virtual void GetViewport(int& x1, int& y1, int& x2, int& y2);
    virtual int GetCode(int x, int y);

    zCPosition VirtualPosition;
    zCPosition VirtualSize;
    zCPosition PixelPosition;
    zCPosition PixelSize;
    unsigned long ID;
    zCViewObject* ViewParent;
    zCArray<zCViewObject*> children;
    zSTRING objectName;

    static zCClassDef* classDef;
};

class zCViewDraw : public zCViewObject {
public:

    zCViewDraw();
    virtual ~zCViewDraw();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void RemoveChild(zSTRING& name);
    void SetTextureAlphaBlendFunc(const int& function);
    void SetTexture(const zSTRING& name);
    void SetTexture(zCTexture* texture);
    void SetTextureColor(const zCOLOR& color);
    void SetTextureAlpha(int alpha);
    void TextureCacheIn(float priority);
    void Render();
    void Draw();
    void DrawChildren();
    virtual void Blit();
    virtual void BlitTexture();
    virtual void EvaluateTexturePosition();
    static zCViewDraw& GetScreen();
    static void CleanupScreen();

    int TextureFuncAlpha;
    zCTexture* Texture;
    zCOLOR TextureColor;
    int TextureAlpha;
    zVEC2 TexturePosition[2];
    zSTRING textureName;

    static zCClassDef* classDef;
};

#endif
