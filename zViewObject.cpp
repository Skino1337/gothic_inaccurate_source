#include "include/zViewDraw.h"

static int viewObjectState = -1;
static zCClassDef zCViewObjectClassDef("zCViewObject", "zCObject", zCViewObject::_CreateNewInstance,
    0, 0x5C, 0, "");
zCClassDef* zCViewObject::classDef = &zCViewObjectClassDef;

zCObject* zCViewObject::_CreateNewInstance()
{
    zCViewObject* view = new zCViewObject;
    zCClassDef::ObjectCreated(view, classDef);
    return view;
}

void zCViewObject::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCViewObject::zCViewObject()
    : ID(0), ViewParent(0)
{
    VirtualPosition = { 0, 0 };
    VirtualSize = { 0, 0 };
    PixelPosition = { 0, 0 };
    PixelSize = { 0, 0 };
}

zCClassDef* zCViewObject::_GetClassDef() const
{
    return classDef;
}

int zCViewObject::Compare(zCViewObject* first, zCViewObject* second)
{
    return first >= second;
}

zCViewObject::~zCViewObject()
{
    RemoveAllChildren();
}

void zCViewObject::AddChild(zCViewObject* child)
{
    if (child) {
        children.InsertEnd(child);
        child->ViewParent = this;
        ++child->refCtr;
    }
}

void zCViewObject::AddChild(zCViewObject& child)
{
    AddChild(&child);
}

void zCViewObject::AddChildren(zCListSort<zCViewObject>& list)
{
    for (zCListSort<zCViewObject>* node = list.next; node; node = node->next)
        AddChild(node->data);
}

void zCViewObject::RemoveChild(zCViewObject* child)
{
    if (!child)
        return;
    children.Remove(child);
    child->ViewParent = 0;
    --child->refCtr;
    if (child->refCtr <= 0)
        delete child;
}

void zCViewObject::RemoveChild(zCViewObject& child)
{
    RemoveChild(&child);
}

void zCViewObject::RemoveAllChildren()
{
    while (children.numInArray > 0)
        RemoveChild(children.parray[children.numInArray - 1]);
}

const zCViewObject* zCViewObject::GetChild(unsigned long id)
{
    for (int i = children.numInArray - 1; i >= 0; --i) {
        if (children.parray[i] && children.parray[i]->ID == id)
            return children.parray[i];
    }
    return 0;
}

void zCViewObject::SetVirtualPosition(zCPosition& position)
{
    VirtualPosition = position;
    UpdatePixelPosition();
}

void zCViewObject::SetPixelPosition(zCPosition& position)
{
    PixelPosition = position;
    UpdatePixelPositionChildren();
}

void zCViewObject::SetVirtualSize(zCPosition& size)
{
    VirtualSize = size;
    if (VirtualSize.X <= 0)
        VirtualSize.X = 1;
    if (VirtualSize.Y <= 0)
        VirtualSize.Y = 1;
    UpdatePixelSize();
    if (PixelSize.X <= 0)
        PixelSize.X = 1;
    if (PixelSize.Y <= 0)
        PixelSize.Y = 1;
}

zCPosition& zCPosition::operator+=(zCPosition& other)
{
    X += other.X;
    Y += other.Y;
    return *this;
}

void zCViewObject::SetPixelSize(zCPosition& size)
{
    PixelSize = size;
    VirtualSize.X = PixelSize.X ? 8192 * PixelSize.X / PixelSize.X : 0;
    VirtualSize.Y = PixelSize.Y ? 8192 * PixelSize.Y / PixelSize.Y : 0;
    UpdatePixelDimensionsChildren();
}

void zCViewObject::GetViewport(int& x1, int& y1, int& x2, int& y2)
{
    x1 = PixelPosition.X;
    y1 = PixelPosition.Y;
    x2 = PixelSize.X;
    y2 = PixelSize.Y;
}

void zCViewObject::EvaluatePixelPosition(zCPosition& position)
{
    zCViewObject* parent = this;
    while (parent->ViewParent) {
        position.X += parent->ViewParent->PixelSize.X * parent->VirtualPosition.X / 8192;
        position.Y += parent->ViewParent->PixelSize.Y * parent->VirtualPosition.Y / 8192;
        parent = parent->ViewParent;
    }
}

void zCViewObject::EvaluatePixelSize(zCPosition& size)
{
    size = { 0, 0 };
    if (ViewParent) {
        size.X = ViewParent->PixelSize.X * VirtualSize.X / 8192;
        size.Y = ViewParent->PixelSize.Y * VirtualSize.Y / 8192;
        return;
    }
    size.X = VirtualSize.X * zrenderer->vid_xdim / 8192;
    size.Y = VirtualSize.Y * zrenderer->vid_ydim / 8192;
}

void zCViewObject::UpdatePixelPosition()
{
    PixelPosition = { 0, 0 };
    if (ViewParent) {
        PixelPosition.X = ViewParent->PixelSize.X * VirtualPosition.X / 8192;
        PixelPosition.Y = ViewParent->PixelSize.Y * VirtualPosition.Y / 8192;
        ViewParent->EvaluatePixelPosition(PixelPosition);
    }
    UpdatePixelPositionChildren();
}

void zCViewObject::UpdatePixelSize()
{
    EvaluatePixelSize(PixelSize);
    UpdatePixelSizeChildren();
}

void zCViewObject::UpdatePixelPositionChildren()
{
    for (int i = children.numInArray - 1; i >= 0; --i)
        children.parray[i]->UpdatePixelPosition();
}

void zCViewObject::UpdatePixelSizeChildren()
{
    for (int i = children.numInArray - 1; i >= 0; --i)
        children.parray[i]->UpdatePixelSize();
}

void zCViewObject::UpdatePixelDimensionsChildren()
{
    UpdatePixelPositionChildren();
    UpdatePixelSizeChildren();
}

zCPosition zCViewObject::ConvertPositionVirtualToPixel(zCPosition& position)
{
    return ConvertSizeVirtualToPixel(position);
}

zCPosition zCViewObject::ConvertSizeVirtualToPixel(zCPosition& size)
{
    zCPosition result = {
        PixelSize.X * size.X / 8192,
        PixelSize.Y * size.Y / 8192
    };
    return result;
}

zCPosition zCViewObject::ConvertPositionPixelToVirtual(zCPosition& position)
{
    return ConvertSizePixelToVirtual(position);
}

zCPosition zCViewObject::ConvertSizePixelToVirtual(zCPosition& size)
{
    zCPosition result = { size.X, size.Y };
    if (PixelSize.X)
        result.X = 8192 * size.X / PixelSize.X;
    if (PixelSize.Y)
        result.Y = 8192 * size.Y / PixelSize.Y;
    return result;
}

zCPosition zCViewObject::ConvertToPixel(zCPosition& position)
{
    zCPosition result = ConvertPositionVirtualToPixel(position);
    result += PixelPosition;
    return result;
}

zCPosition zCViewObject::ConvertToPixelLocal(zCPosition& position)
{
    return ConvertPositionVirtualToPixel(position);
}

zCPosition zCViewObject::ConvertToVirtualLocal(zCPosition& position)
{
    return ConvertPositionPixelToVirtual(position);
}

int zCViewObject::nax(int x)
{
    return PixelSize.X * x / 8192;
}

int zCViewObject::nay(int y)
{
    return PixelSize.Y * y / 8192;
}

int zCViewObject::anx(int x)
{
    return PixelSize.X ? 8192 * x / PixelSize.X : 0;
}

int zCViewObject::any(int y)
{
    return PixelSize.Y ? 8192 * y / PixelSize.Y : 0;
}

int zCViewObject::ClipLine(int& x1, int& y1, int& x2, int& y2)
{
    int code1 = GetCode(x1, y1);
    int code2 = GetCode(x2, y2);
    while (true) {
        if (!(code1 | code2))
            return 1;
        if (code1 & code2)
            return 0;
        int code = code1 ? code1 : code2;
        int x = 0;
        int y = 0;
        if (code & 8) {
            x = x1 + (x2 - x1) * (PixelSize.Y - 1 - y1) / (y2 - y1);
            y = PixelSize.Y - 1;
        } else if (code & 4) {
            x = x1 + (x2 - x1) * (0 - y1) / (y2 - y1);
            y = 0;
        } else if (code & 2) {
            y = y1 + (y2 - y1) * (PixelSize.X - 1 - x1) / (x2 - x1);
            x = PixelSize.X - 1;
        } else {
            y = y1 + (y2 - y1) * (0 - x1) / (x2 - x1);
            x = 0;
        }
        if (code == code1) {
            x1 = x;
            y1 = y;
            code1 = GetCode(x1, y1);
        } else {
            x2 = x;
            y2 = y;
            code2 = GetCode(x2, y2);
        }
    }
}

void zCViewObject::Line(int x1, int y1, int x2, int y2, const zCOLOR& color)
{
    x1 = screen->nax(x1);
    y1 = screen->nay(y1);
    x2 = screen->nax(x2);
    y2 = screen->nay(y2);
    if (ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLine(x1, y1, x2, y2, color.dword);
}

int zCViewObject::GetCode(int x, int y)
{
    int result = y > PixelSize.Y - 1 ? 8 : y >= 0 ? 0 : 4;
    if (x > PixelSize.X - 1)
        result |= 2;
    else if (x < 0)
        result |= 1;
    return result;
}
