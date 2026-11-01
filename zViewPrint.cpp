#include "include/zViewPrint.h"

extern zTIMER ztimer;

static int viewPrintState = -1;
static int viewPrintRuntimeState = -1;
static zCClassDef zCViewPrintClassDef("zCViewPrint", "zCObject", zCViewPrint::_CreateNewInstance,
    0, 0xEC, 0, "");
zCClassDef* zCViewPrint::classDef = &zCViewPrintClassDef;
static int capsLockEnabled = 0;

zCObject* zCViewPrint::_CreateNewInstance()
{
    zCViewPrint* view = new zCViewPrint;
    zCClassDef::ObjectCreated(view, classDef);
    return view;
}

void zCViewPrint::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCViewPrint::zCViewPrint()
{
    PositionCursor = { 0, 0 };
    OffsetTextPixel = { 0, 0 };
    SizeMargin[0] = { 0, 0 };
    SizeMargin[1] = { 0, 0 };
}

zCClassDef* zCViewPrint::_GetClassDef() const
{
    return classDef;
}

zCViewPrint::~zCViewPrint()
{
    RemoveAllTexts();
}

void zCViewPrint::SizeHeightToContent(int keepPosition)
{
    if (!ViewFont.Font)
        return;
    int oldY = PixelPosition.Y;
    zCPosition size = PixelSize;
    size.Y = ListTextLines.numInArray * ViewFont.Font->GetFontY()
        + SizeMargin[0].Y + SizeMargin[1].Y + 1;
    SetPixelSize(size);
    if (keepPosition) {
        zCPosition position = { PixelPosition.X, oldY };
        SetPixelPosition(position);
    } else {
        zCPosition position = { PixelPosition.X, oldY + size.Y - PixelSize.Y };
        SetPixelPosition(position);
    }
}

void zCViewPrint::Blit()
{
    zCViewFX::Blit();
    if (HasOpened && TimeClose <= 0.0f)
        BlitText();
}

void zCViewPrint::BlitText()
{
    for (int i = ListTextLines.numInArray - 1; i >= 0; --i) {
        zCViewText2* text = ListTextLines.parray[i];
        if (!text)
            continue;
        if (!text->EnabledTimer) {
            BlitTextLine(text);
            continue;
        }
        text->Timer -= ztimer.frameTimeFloat;
        if (text->Timer > 0.0f)
            BlitTextLine(text);
        else
            RemoveText(text);
    }
}

void zCViewPrint::BlitTextLine(zCViewText2* text)
{
    zCFont* font = text->ViewFont.Font ? text->ViewFont.Font : ViewFont.Font;
    zCOLOR color = text->EnabledColor ? text->ViewFont.Color : ViewFont.Color;
    BlitTextCharacters(text, font, color);
}

void zCViewPrint::BlitTextCharacters(zCViewText2* text, zCFont* font, zCOLOR& color)
{
    zCTexture* texture = font->GetFontTexture();
    int x = text->PixelPosition.X + PixelPosition.X + OffsetTextPixel.X;
    int y = text->PixelPosition.Y + PixelPosition.Y + OffsetTextPixel.Y;
    int fontHeight = font->GetFontY();
    int letterDistance = font->GetLetterDistance();
    int spaceWidth = font->GetWidth(' ');
    float z = zCCamera::activeCam ? zCCamera::activeCam->nearClipZ + 1.0f : 1.0f;
    for (int i = 0; i < text->Text.Length(); ++i) {
        unsigned char character = text->Text[i];
        if (character <= ' ') {
            x += spaceWidth;
            continue;
        }
        int width;
        zVEC2 uvTopLeft;
        zVEC2 uvBottomRight;
        font->GetFontData(character, width, uvTopLeft, uvBottomRight);
        if (x >= PixelPosition.X + PixelSize.X || y >= PixelPosition.Y + PixelSize.Y)
            break;
        if (x + width - 1 >= PixelPosition.X && y + fontHeight - 1 >= PixelPosition.Y) {
            zVEC2 topLeft(x, y);
            zVEC2 bottomRight(x + width - 1, y + fontHeight - 1);
            zrenderer->DrawTile(texture, topLeft, bottomRight, z,
                uvTopLeft, uvBottomRight, color.dword);
        }
        x += width + letterDistance;
    }
}

void zCViewPrint::GetPixelBorder(zCPosition& topLeft, zCPosition& bottomRight)
{
    topLeft = PixelPosition;
    bottomRight = PixelSize;
    bottomRight += PixelPosition;
    topLeft += SizeMargin[0];
    bottomRight -= SizeMargin[1];
}

void zCViewPrint::ClearText()
{
    RemoveAllTexts();
    PositionCursor = SizeMargin[0];
}

void zCViewPrint::Print(zSTRING& text)
{
    if (!ViewFont.Font)
        return;
    zCViewText2* line = new zCViewText2(text, PositionCursor, ViewFont.Color,
        ViewFont.Font, ViewFont.Alpha, zRND_ALPHA_FUNC_NONE);
    AddText(line);
    PositionCursor.X += ViewFont.Font->GetFontX(text);
}

void zCViewPrint::PrintLine(zSTRING& text)
{
    if (!ViewFont.Font)
        return;
    zCViewText2* line = new zCViewText2(text, PositionCursor, ViewFont.Color,
        ViewFont.Font, ViewFont.Alpha, zRND_ALPHA_FUNC_NONE);
    AddText(line);
    PositionCursor.X = SizeMargin[0].X;
    PositionCursor.Y += ViewFont.Font->GetFontY();
}

void zCViewPrint::PrintLines(zSTRING& text)
{
    if (!ViewFont.Font)
        return;
    zCPosition topLeft;
    zCPosition bottomRight;
    GetPixelBorder(topLeft, bottomRight);
    zSTRING separators = zSTR_SKIP + "\n";
    zSTRING current;
    for (int index = 1;; ++index) {
        zSTRING word = text.PickWord(index, separators, "");
        if (word.IsEmpty())
            break;
        if (word == "\n") {
            PrintLine(current);
            current = "";
            continue;
        }
        if (!current.IsEmpty()
            && PositionCursor.X + ViewFont.Font->GetFontX(word) >= bottomRight.X) {
            PrintLine(current);
            current = word;
        } else {
            current += word;
        }
    }
    if (!current.IsEmpty())
        PrintLine(current);
}

void zCViewPrint::AddText(zCViewText2* text)
{
    if (text)
        ListTextLines.InsertEnd(text);
}

void zCViewPrint::RemoveText(zCViewText2* text)
{
    if (!text)
        return;
    ListTextLines.Remove(text);
    delete text;
}

void zCViewPrint::RemoveAllTexts()
{
    for (int i = ListTextLines.numInArray - 1; i >= 0; --i)
        delete ListTextLines.parray[i];
    ListTextLines.numInArray = 0;
}

int CapsLock()
{
    unsigned char keyState[256];
    GetKeyboardState(keyState);
    return keyState[VK_CAPITAL] & 1;
}

void SetCapsLock(int enabled)
{
    if ((CapsLock() != 0) == (enabled != 0))
        return;
    keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY, 0);
    keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

void ToggleCapsLock()
{
    SetCapsLock(!CapsLock());
}
