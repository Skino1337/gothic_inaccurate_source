#include "include/oViewDocument.h"

zCPosition& zCPosition::operator-=(zCPosition& other)
{
    X -= other.X;
    Y -= other.Y;
    return *this;
}

oCViewDocument::oCViewDocument(zSTRING& texture)
    : Pages(0), textureName(texture), ID(0)
{
    Init(&zCViewDraw::GetScreen(), 0, 0, 0, 0.0f, 0.0f, texture);
    zSTRING defaultFont = "font_default.tga";
    ViewFont.SetFont(defaultFont);
    fontName = defaultFont;
    marginTopLeft = { 10, 10 };
    marginBottomRight = { 20, 10 };
    SizeMargin[0] = marginTopLeft;
    SizeMargin[1] = marginBottomRight;
    if (PositionCursor.X < marginTopLeft.X)
        PositionCursor.X = marginTopLeft.X;
    if (PositionCursor.Y < marginTopLeft.Y)
        PositionCursor.Y = marginTopLeft.Y;
    pixelPosition = { 0, 0 };
    pixelSize = { 8192, 8192 };
    parentPosition = { 0, 0 };
    parentSize = { 8192, 8192 };
    SizeToContent();
    Center();
    OpenImmediately();
    SetEnableHandleEvent(1);
    SetHandleEventTop();
    oCNpc* player = ogame->GetSelfPlayerVob();
    if (player) {
        player->SetMovLock(1);
        if (player->inventory2.IsOpen())
            player->CloseInventory();
    }
}

oCViewDocument::oCViewDocument()
    : Pages(0), ID(0)
{
    marginTopLeft = { 10, 10 };
    marginBottomRight = { 20, 10 };
    pixelPosition = { 0, 0 };
    pixelSize = { 8192, 8192 };
    parentPosition = { 0, 0 };
    parentSize = { 8192, 8192 };
}

oCViewDocument::~oCViewDocument()
{
    SetPages(0);
}

oCViewDocument* oCViewDocument::GetPage(int page)
{
    for (int i = 0; i < pagesList.numInArray; ++i) {
        if (pagesList.parray[i]->ID == page)
            return pagesList.parray[i];
    }
    return 0;
}

void oCViewDocument::SetPages(int pages)
{
    RemoveAllChildren();
    pagesList.numInArray = 0;
    Pages = pages;
    for (int i = 0; i < pages; ++i) {
        oCViewDocument* page = new oCViewDocument;
        page->ID = pages - i - 1;
        page->parentPosition = pixelPosition;
        page->parentSize = pixelSize;
        zSTRING texture;
        page->Init(this, 1, 2, 2, 500.0f, 500.0f, texture);
        page->Open();
        pagesList.InsertEnd(page);
    }
}

void oCViewDocument::SetMargins(int page, zCPosition& topLeft, zCPosition& bottomRight,
    int adjustText)
{
    if (page >= 0) {
        oCViewDocument* document = GetPage(page);
        if (document && adjustText) {
            document->marginTopLeft = topLeft;
            document->marginBottomRight = bottomRight;
            document->SizeMargin[0] = topLeft;
            document->SizeMargin[1] = bottomRight;
            if (document->PositionCursor.X < topLeft.X)
                document->PositionCursor.X = topLeft.X;
            if (document->PositionCursor.Y < topLeft.Y)
                document->PositionCursor.Y = topLeft.Y;
        }
        return;
    }
    for (int i = 0; i < pagesList.numInArray; ++i) {
        if (adjustText) {
            oCViewDocument* document = pagesList.parray[i];
            document->marginTopLeft = topLeft;
            document->marginBottomRight = bottomRight;
            document->SizeMargin[0] = topLeft;
            document->SizeMargin[1] = bottomRight;
            if (document->PositionCursor.X < topLeft.X)
                document->PositionCursor.X = topLeft.X;
            if (document->PositionCursor.Y < topLeft.Y)
                document->PositionCursor.Y = topLeft.Y;
        }
    }
}

oCViewDocument* oCViewDocument::SetPage(int page, zSTRING& texture, int fullSize)
{
    if (page >= 0) {
        oCViewDocument* document = GetPage(page);
        if (!document)
            return 0;
        document->SetTexture(texture, fullSize);
        return document;
    }
    oCViewDocument* result = 0;
    for (int i = 0; i < pagesList.numInArray; ++i) {
        pagesList.parray[i]->SetTexture(texture, fullSize);
        result = pagesList.parray[i];
    }
    return result;
}

void oCViewDocument::SetFont(int page, zSTRING& font)
{
    if (page >= 0) {
        oCViewDocument* document = GetPage(page);
        if (document) {
            document->fontName = font;
            document->ViewFont.SetFont(font);
        }
        return;
    }
    for (int i = 0; i < pagesList.numInArray; ++i) {
        pagesList.parray[i]->fontName = font;
        pagesList.parray[i]->ViewFont.SetFont(font);
    }
}

void oCViewDocument::PrintLine(int page, zSTRING& text)
{
    if (page >= 0) {
        oCViewDocument* document = GetPage(page);
        if (document) {
            document->textLines.InsertEnd(text);
            document->zCViewPrint::PrintLine(text);
        }
        return;
    }
    for (int i = 0; i < pagesList.numInArray; ++i) {
        pagesList.parray[i]->textLines.InsertEnd(text);
        pagesList.parray[i]->zCViewPrint::PrintLine(text);
    }
}

void oCViewDocument::PrintLines(int page, zSTRING& text)
{
    if (page >= 0) {
        oCViewDocument* document = GetPage(page);
        if (document)
            document->zCViewPrint::PrintLines(text);
        return;
    }
    for (int i = 0; i < pagesList.numInArray; ++i)
        pagesList.parray[i]->zCViewPrint::PrintLines(text);
}

void oCViewDocument::SetTexture(zSTRING& texture, int fullSize)
{
    textureName = texture;
    zCViewDraw::SetTexture(texture);
    if (fullSize)
        SizeToParent();
    else
        SizeToContent();
    AlignPageToParent();
}

void oCViewDocument::AlignPageToParent()
{
    oCViewDocument* parent = dynamic_cast<oCViewDocument*>(ViewParent);
    if (!parent || parent->Pages < 1)
        return;
    if (parent->Pages == 1) {
        Center();
        return;
    }
    zCPosition position;
    position.X = parent->PixelPosition.X + parent->PixelSize.X / 2
        - parent->Pages * PixelSize.X / 2 + ID * PixelSize.X;
    position.Y = parent->PixelPosition.Y + parent->PixelSize.Y / 2
        - parent->Pages * PixelSize.Y / 2 + PixelSize.Y / 2;
    SetPixelPosition(position);
}

void oCViewDocument::Center()
{
    if (!ViewParent) {
        zCPosition position = { 0, 0 };
        SetVirtualPosition(position);
        return;
    }
    zCPosition position;
    position.X = ViewParent->PixelPosition.X + (ViewParent->PixelSize.X - PixelSize.X) / 2;
    position.Y = ViewParent->PixelPosition.Y + (ViewParent->PixelSize.Y - PixelSize.Y) / 2;
    SetPixelPosition(position);
}

void oCViewDocument::SizeToContent()
{
    if (Texture && ViewParent) {
        zCTextureInfo info = Texture->GetTextureInfo();
        if (info.refSizeX > 0 && info.refSizeX < ViewParent->PixelSize.X
            && info.refSizeY > 0 && info.refSizeY < ViewParent->PixelSize.Y) {
            zCPosition size = { info.refSizeX, info.refSizeY };
            SetPixelSize(size);
            return;
        }
    }
    zCPosition position = { 0, 0 };
    SetVirtualPosition(position);
    zCPosition size = { 8192, 8192 };
    SetVirtualSize(size);
}

void oCViewDocument::SizeToParent()
{
    pixelPosition = { 0, 0 };
    pixelSize = { 8192, 8192 };
    SetVirtualPosition(pixelPosition);
    SetVirtualSize(pixelSize);
}

void oCViewDocument::ScaleToParent()
{
    SizeToParent();
}
