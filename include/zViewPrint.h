#ifndef ZVIEWPRINT_H
#define ZVIEWPRINT_H

#include "zViewPrint_Font.h"

class zCViewPrint : public zCViewFX {
public:

    enum zEViewAlignment {
        VIEW_ALIGN_NONE,
        VIEW_ALIGN_MAX,
        VIEW_ALIGN_MIN,
        VIEW_ALIGN_CENTER
    };
    typedef zEViewAlignment zTViewAlign;

    zCViewPrint();
    virtual ~zCViewPrint();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void SizeHeightToContent(int keepPosition);
    void GetPixelBorder(zCPosition& topLeft, zCPosition& bottomRight);
    void ClearText();
    void Print(zSTRING& text);
    void PrintLine(zSTRING& text);
    void PrintLines(zSTRING& text);
    void AddText(zCViewText2* text);
    void RemoveText(zCViewText2* text);
    void RemoveAllTexts();
    virtual void Blit();
    virtual void BlitText();
    virtual void BlitTextLine(zCViewText2* text);
    virtual void BlitTextCharacters(zCViewText2* text, zCFont* font, zCOLOR& color);

    zCArray<zCViewText2*> ListTextLines;
    zCViewFont ViewFont;
    zCPosition PositionCursor;
    zCPosition OffsetTextPixel;
    zCPosition SizeMargin[2];

    static zCClassDef* classDef;
};

#endif
