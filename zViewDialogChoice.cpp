#include "include/zViewDialogChoice.h"

extern zTIMER ztimer;

static int viewChoiceState = -1;
static zCClassDef zCViewDialogChoiceClassDef("zCViewDialogChoice", "zCObject",
    zCViewDialogChoice::_CreateNewInstance, 0, 0x10C, 0, "");
zCClassDef* zCViewDialogChoice::classDef = &zCViewDialogChoiceClassDef;

zCObject* zCViewDialogChoice::_CreateNewInstance()
{
    zCViewDialogChoice* dialog = new zCViewDialogChoice;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

zCClassDef* zCViewDialogChoice::_GetClassDef() const
{
    return classDef;
}

zCViewDialogChoice::zCViewDialogChoice()
    : ColorSelected{ 255, 255, 255, 255 }, ColorGrayed{ 159, 159, 159, 255 },
      ChoiceSelected(-1), Choices(0), LineStart(0), OffsetTextPixelY(0)
{
}

zCViewDialogChoice::~zCViewDialogChoice()
{
    RemoveAllChoices();
}

int zCViewDialogChoice::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (zinput->IsBinded(3, key) || key == 0x20a) {
        SelectPrevious();
        return 1;
    }
    if (zinput->IsBinded(4, key) || key == 0x20b) {
        SelectNext();
        return 1;
    }
    if (zinput->IsBinded(5, key) || key == 0x1c || key == 0x20c) {
        if (!TimeOpen && DurationOpen != 0.0f)
            zCViewFX::CloseSafe();
        return 1;
    }
    if (zinput->IsBinded(0xf, key) || zinput->IsBinded(8, key))
        return 1;
    if (key >= 2 && key <= 0x0b) {
        int choice = key == 0x0b ? 0 : key - 1;
        if (choice < Choices)
            Select(choice);
        return 1;
    }
    return 0;
}

void zCViewDialogChoice::StartSelection()
{
    HasClosed = 1;
    IsActivated = 1;
    IsDone = 0;
    OpenSafe();
    SetHandleEventTop();
    SetEnableHandleEvent(1);
}

void zCViewDialogChoice::StopSelection()
{
    IsDone = 0;
    IsActivated = 0;
    zCViewFX::CloseSafe();
    SetEnableHandleEvent(0);
}

void zCViewDialogChoice::BlitText()
{
    for (int i = LineStart; i < ListTextLines.numInArray; ++i) {
        zCViewText2* line = ListTextLines.parray[i];
        if (!line)
            continue;
        if (!line->EnabledTimer) {
            BlitTextLine(line);
            continue;
        }
        float timer = line->Timer - ztimer.frameTimeFloat / ztimer.factorMotion;
        if (timer > 0.0f) {
            line->Timer = timer;
            BlitTextLine(line);
        } else {
            choiceList.Remove(line);
            RemoveText(line);
        }
    }
}

void zCViewDialogChoice::ScrollUp()
{
    if (LineStart <= 0)
        return;
    zCViewText2* line = ListTextLines.parray[LineStart];
    if (line && line->ViewFont.Font) {
        OffsetTextPixel.Y += line->ViewFont.Font->GetFontY();
        OffsetTextPixelY = OffsetTextPixel.Y;
    }
    --LineStart;
}

void zCViewDialogChoice::ScrollDown()
{
    if (LineStart >= Choices)
        return;
    zCViewText2* line = ListTextLines.parray[LineStart];
    if (line && line->ViewFont.Font) {
        OffsetTextPixel.Y -= line->ViewFont.Font->GetFontY();
        OffsetTextPixelY = OffsetTextPixel.Y;
    }
    ++LineStart;
}

int zCViewDialogChoice::IsSelectedOutsideAbove()
{
    return ChoiceSelected < LineStart;
}

int zCViewDialogChoice::IsSelectedOutsideBelow()
{
    zCPosition topLeft = { 0, 0 };
    zCPosition bottomRight = { 0, 0 };
    GetPixelBorder(topLeft, bottomRight);
    int height = topLeft.X;
    for (int i = LineStart; i < Choices; ++i) {
        zCViewText2* line = ListTextLines.parray[i];
        if (line) {
            zCFont* font = line->ViewFont.Font ? line->ViewFont.Font : ViewFont.Font;
            if (font)
                height += font->GetFontY();
            if (i == ChoiceSelected)
                break;
        }
    }
    return height > bottomRight.X;
}

void zCViewDialogChoice::ShowSelected()
{
    while (ChoiceSelected < LineStart)
        ScrollUp();
    while (IsSelectedOutsideBelow())
        ScrollDown();
}

void zCViewDialogChoice::SelectPrevious()
{
    if (!Choices)
        return;
    ChoiceSelected = ChoiceSelected <= 0 ? Choices - 1 : ChoiceSelected - 1;
    HighlightSelected();
}

void zCViewDialogChoice::SelectNext()
{
    if (!Choices)
        return;
    ChoiceSelected = ChoiceSelected >= Choices - 1 ? 0 : ChoiceSelected + 1;
    HighlightSelected();
}

void zCViewDialogChoice::Select(int choice)
{
    if (choice < 0 || choice >= Choices)
        return;
    ChoiceSelected = choice;
    HighlightSelected();
}

zCViewText2* zCViewDialogChoice::GetSelection()
{
    return choiceList.parray[ChoiceSelected];
}

zSTRING zCViewDialogChoice::GetSelectedText()
{
    zCViewText2* selection = GetSelection();
    return selection ? selection->Text : zSTRING("");
}

void zCViewDialogChoice::HighlightSelected()
{
    for (int i = 0; i < Choices; ++i) {
        zCViewText2* line = choiceList.parray[i];
        if (line) {
            line->Color = i == ChoiceSelected ? ColorSelected : ColorGrayed;
            line->Enabled = 1;
        }
    }
    ShowSelected();
}

void zCViewDialogChoice::AddChoice(zSTRING& text, int numbered)
{
    zSTRING lineText = numbered ? zSTRING(Choices) + zSTRING(". ") + text : text;
    int lineCount = ListTextLines.numInArray;
    zCViewPrint::PrintLine(lineText);
    if (ListTextLines.numInArray > lineCount)
        choiceList.InsertEnd(ListTextLines.parray[ListTextLines.numInArray - 1]);
    ++Choices;
    ChoiceSelected = 0;
    HighlightSelected();
}

void zCViewDialogChoice::RemoveChoice(zSTRING& text)
{
}

void zCViewDialogChoice::RemoveChoice(int choice)
{
}

void zCViewDialogChoice::RemoveAllChoices()
{
    RemoveAllTexts();
    choiceList.numInArray = 0;
    Choices = 0;
    ChoiceSelected = -1;
    LineStart = 0;
    OffsetTextPixelY = 0;
}
