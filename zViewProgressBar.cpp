#include "include/zSession.h"
#include "include/zObject.h"

zCViewProgressBar::zCViewProgressBar()
    : bar(NULL), minStep(1), percent(0), start_time(0)
{
    rangeList.data = NULL;
    rangeList.next = NULL;
}

zCViewProgressBar::zCViewProgressBar(int x1, int y1, int x2, int y2, int viewID)
    : zCView(x1, y1, x2, y2, viewID), bar(NULL), minStep(1), percent(0), start_time(0)
{
    rangeList.data = NULL;
    rangeList.next = NULL;
}

zCViewProgressBar::~zCViewProgressBar()
{
    delete bar;
    rangeList.DeleteListDatas();
}

void zCViewProgressBar::Init()
{
    minStep = 1;
    bar = NULL;
    percent = 0;
    text = "";
}

float zCViewProgressBar::CalcPercentOf(float value, float min, float max)
{
    return (max - min) * value * 0.01f + min;
}

void zCViewProgressBar::SetPercent(int value, zSTRING valueText)
{
    if (value < 0)
        value = 0;
    else if (value > 100)
        value = 100;
    float mapped = value;
    if (rangeList.next)
    {
        zTProgressRange* range = rangeList.next->data;
        mapped = CalcPercentOf(value, range->min, range->max);
    }
    int oldPercent = percent;
    if (oldPercent && oldPercent != 100 && minStep > mapped - oldPercent)
        return;
    percent = (int)mapped;
    if (percent < 0)
        percent = 0;
    else if (percent > 100)
        percent = 100;
    zerr->Report(zERR_TYPE_INFO, 0, "B: ---------- " + zSTRING(percent) + "% ----------",
        0, 0, 142, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zViewProgressbar.cpp", NULL);
    if (!valueText.IsEmpty())
        text = valueText;
    HandleChange();
}

void zCViewProgressBar::HandleChange()
{
    ClrPrintwin();
    Print(0, 0, zSTRING((long)percent) + "%");
    Draw();
}

void zCViewProgressBar::Draw()
{
    zrenderer->BeginFrame();
    screen->Render();
    zrenderer->EndFrame();
    zrenderer->Vid_Blit(0, NULL, NULL);
}

void zCViewProgressBar::SetRange(int minimum, int maximum)
{
    float min = minimum;
    float max = maximum;
    if (rangeList.next)
    {
        zTProgressRange* current = rangeList.next->data;
        min = CalcPercentOf(minimum, current->min, current->max);
        max = CalcPercentOf(maximum, current->min, current->max);
    }
    zTProgressRange* range = new zTProgressRange;
    range->iteration = rangeList.GetNumInList();
    range->min = min;
    range->max = max;
    rangeList.Insert(range);
}

void zCViewProgressBar::ResetRange()
{
    if (!rangeList.next)
        return;
    zTProgressRange* range = rangeList.next->data;
    rangeList.Remove(range);
    delete range;
}
