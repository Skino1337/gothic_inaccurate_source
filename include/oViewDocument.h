#ifndef OVIEWDOCUMENT_H
#define OVIEWDOCUMENT_H

#include "oViewDialogInventory.h"

struct zCPosition {
    zCPosition& operator-=(zCPosition& other);
    zCPosition& operator+=(zCPosition& other);

    int X;
    int Y;
};

class oCViewDocument : public zCViewDialog {
public:

    oCViewDocument(zSTRING& texture);
    oCViewDocument();
    virtual ~oCViewDocument();
    oCViewDocument* GetPage(int page);
    void SetPages(int pages);
    void SetMargins(int page, zCPosition& topLeft, zCPosition& bottomRight, int adjustText);
    virtual oCViewDocument* SetPage(int page, zSTRING& texture, int fullSize);
    void SetFont(int page, zSTRING& font);
    void PrintLine(int page, zSTRING& text);
    void PrintLines(int page, zSTRING& text);
    void SetTexture(zSTRING& texture, int fullSize);
    void AlignPageToParent();
    void Center();
    void SizeToContent();
    void SizeToParent();
    void ScaleToParent();

    int Pages;
    zCArray<oCViewDocument*> pagesList;
    zSTRING textureName;
    zSTRING fontName;
    zCArray<zSTRING> textLines;
    zCPosition marginTopLeft;
    zCPosition marginBottomRight;
    zCPosition pixelPosition;
    zCPosition pixelSize;
    zCPosition parentPosition;
    zCPosition parentSize;
    int ID;
};

#endif
