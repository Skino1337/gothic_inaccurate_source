#ifndef ZVIEWPRINT_FONT_H
#define ZVIEWPRINT_FONT_H

#include "zViewFx.h"

class zCFont;

class zCViewFont {
public:

    zCViewFont(zCFont* font, zCOLOR& color, int alpha, int function);
    zCViewFont(zCFont* font, zCOLOR& color);
    zCViewFont(zCFont* font);
    zCViewFont();
    ~zCViewFont();
    void SetFont(zSTRING& name);

    int FuncAlphaBlend;
    zCFont* Font;
    zCOLOR Color;
    int Alpha;
    int EnabledBlend;
    zSTRING fontName;
};

class zCViewText2 {
public:

    zCViewText2();
    zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, float timer,
        zCFont* font, int alpha, int function);
    zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, zCFont* font,
        int alpha, int function);
    zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, float timer, zCFont* font);
    zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, zCFont* font);
    zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color);
    zCViewText2(zSTRING& text, zCPosition& position, float timer, zCFont* font);
    zCViewText2(zSTRING& text, zCPosition& position, zCFont* font);
    zCViewText2(zSTRING& text, zCPosition& position, float timer);
    zCViewText2(zSTRING& text, zCPosition& position);
    ~zCViewText2();

    int EnabledColor;
    int Enabled;
    int EnabledTimer;
    zCPosition PixelPosition;
    float Timer;
    zSTRING Text;
    zCOLOR Color;
    zCViewFont ViewFont;
};

#endif
