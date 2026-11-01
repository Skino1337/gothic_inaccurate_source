#include "include/zViewPrint_Font.h"

zCViewFont::zCViewFont(zCFont* font, zCOLOR& color, int alpha, int function)
    : FuncAlphaBlend(function), Font(font), Color(color), Alpha(alpha), EnabledBlend(1)
{
}

zCViewFont::zCViewFont(zCFont* font, zCOLOR& color)
    : FuncAlphaBlend(1), Font(font), Color(color), Alpha(255), EnabledBlend(0)
{
}

zCViewFont::zCViewFont(zCFont* font)
    : FuncAlphaBlend(1), Font(font), Color{ 255, 255, 255, 255 }, Alpha(255), EnabledBlend(0)
{
}

zCViewFont::zCViewFont()
    : FuncAlphaBlend(1), Font(0), Color{ 255, 255, 255, 255 }, Alpha(255), EnabledBlend(0)
{
}

zCViewFont::~zCViewFont()
{
}

void zCViewFont::SetFont(zSTRING& name)
{
    fontName = name;
    Font = zfontman->GetFont(zfontman->Load(name));
}

zCViewText2::zCViewText2()
    : EnabledColor(1), Enabled(1), EnabledTimer(0), Timer(0.0f), Color{ 255, 255, 255, 255 }
{
    PixelPosition = { 0, 0 };
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, float timer,
    zCFont* font, int alpha, int function)
    : EnabledColor(1), Enabled(1), EnabledTimer(1), PixelPosition(position), Timer(timer),
      Text(text), Color(color), ViewFont(font, color, alpha, function)
{
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, zCFont* font,
    int alpha, int function)
    : zCViewText2(text, position, color, 0.0f, font, alpha, function)
{
    EnabledTimer = 0;
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, float timer,
    zCFont* font)
    : zCViewText2(text, position, color, timer, font, 255, 1)
{
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color, zCFont* font)
    : zCViewText2(text, position, color, 0.0f, font, 255, 1)
{
    EnabledTimer = 0;
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, zCOLOR& color)
    : zCViewText2(text, position, color, 0.0f, 0, 255, 1)
{
    EnabledTimer = 0;
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, float timer, zCFont* font)
    : zCViewText2(text, position, ViewFont.Color, timer, font, 255, 1)
{
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, zCFont* font)
    : zCViewText2(text, position, ViewFont.Color, 0.0f, font, 255, 1)
{
    EnabledTimer = 0;
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position, float timer)
    : zCViewText2(text, position, ViewFont.Color, timer, 0, 255, 1)
{
}

zCViewText2::zCViewText2(zSTRING& text, zCPosition& position)
    : zCViewText2(text, position, ViewFont.Color, 0.0f, 0, 255, 1)
{
    EnabledTimer = 0;
}

zCViewText2::~zCViewText2()
{
}
