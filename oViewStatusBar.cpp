#include "include/zSession.h"

oCViewStatusBar::oCViewStatusBar()
    : scale(1.0f), range_bar(NULL), value_bar(NULL)
{
}

oCViewStatusBar::oCViewStatusBar(int x1, int y1, int x2, int y2, int viewID)
    : zCViewStatusBar(x1, y1, x2, y2, viewID), scale(1.0f), range_bar(NULL), value_bar(NULL)
{
}

oCViewStatusBar::~oCViewStatusBar()
{
    delete value_bar;
    value_bar = NULL;
    delete range_bar;
    range_bar = NULL;
}

void oCViewStatusBar::Init(int x, int y, float scaleValue)
{
    scale = scaleValue;
    SetPos(x, y);
    SetSize(screen->anx(180), screen->any(20));
    zCViewStatusBar::Init();
    if (!range_bar)
        range_bar = new zCView(0, 0, 4000, 4000, 1);
    InsertItem(range_bar);
    if (!value_bar)
        value_bar = new zCView(0, 0, 2000, 2000, 2);
    range_bar->InsertItem(value_bar);
    SetMaxRange(0.0f, 100.0f);
    SetRange(0.0f, 100.0f);
    SetValue(25.0f);
}

void oCViewStatusBar::SetTextures(const zSTRING& view, const zSTRING& range,
    const zSTRING& value, const zSTRING&)
{
    texView = view;
    texRange = range;
    texValue = value;
    InsertBack(texView);
    SetTransparency(255);
    SetAlphaBlendFunc(2);
    range_bar->InsertBack(texRange);
    value_bar->InsertBack(texValue);
}

void oCViewStatusBar::SetMaxRange(float min, float max)
{
    zCViewStatusBar::SetMaxRange(min, max);
    SetRange(low, high);
}

void oCViewStatusBar::SetRange(float min, float max)
{
    zCViewStatusBar::SetRange(min, max);
    int width = 0;
    if (high > low && maxHigh > minLow)
        width = (int)((high - low) / (maxHigh - minLow) * (8192.0f - 2.0f * 350.0f));
    range_bar->SetPos(350, 1100);
    range_bar->SetSize(width, 8192 - 2 * 1100);
    SetPreview(previewValue);
    SetValue(currentValue);
}

void oCViewStatusBar::SetPreview(float value)
{
    zCViewStatusBar::SetPreview(value);
}

void oCViewStatusBar::SetValue(float value)
{
    zCViewStatusBar::SetValue(value);
    int width = 0;
    if (high > low)
        width = (int)(currentValue / (high - low) * 8192.0f);
    value_bar->SetPos(0, 0);
    value_bar->SetSize(width, 0x2000);
}
