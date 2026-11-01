#include "include/zSession.h"

zCViewStatusBar::zCViewStatusBar()
{
    Init();
}

zCViewStatusBar::zCViewStatusBar(int x1, int y1, int x2, int y2, int viewID)
    : zCView(x1, y1, x2, y2, viewID)
{
    Init();
}

zCViewStatusBar::~zCViewStatusBar()
{
}

void zCViewStatusBar::Init()
{
    minLow = 0.0f;
    maxHigh = 100.0f;
    low = 0.0f;
    high = 100.0f;
    previewValue = 0.0f;
    currentValue = 50.0f;
}

void zCViewStatusBar::SetMaxRange(float min, float max)
{
    if (min < 0.0f)
        min = 0.0f;
    if (max < 0.0f)
        max = 0.0f;
    if (min > max)
    {
        float swap = min;
        min = max;
        max = swap;
    }
    maxHigh = max;
    minLow = min;
    if (high > max)
        high = max;
    if (low < min)
        low = min;
}

void zCViewStatusBar::SetRange(float min, float max)
{
    if (min < 0.0f)
        min = 0.0f;
    if (max < 0.0f)
        max = 0.0f;
    if (min > max)
    {
        float swap = min;
        min = max;
        max = swap;
    }
    if (min < minLow)
        min = minLow;
    if (max > maxHigh)
        max = maxHigh;
    low = min;
    high = max;
}

void zCViewStatusBar::SetPreview(float value)
{
    if (value < minLow)
        value = minLow;
    if (value > maxHigh)
        value = maxHigh;
    previewValue = value;
}

void zCViewStatusBar::SetValue(float value)
{
    if (value < minLow)
        value = minLow;
    if (value > maxHigh)
        value = maxHigh;
    currentValue = value;
}
