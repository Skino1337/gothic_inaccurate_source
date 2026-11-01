#ifndef ZVIEWDIALOGCHOICE_H
#define ZVIEWDIALOGCHOICE_H

#include "oViewDialogInventory.h"
#include "zViewPrint_Font.h"
#include "zSky.h"

class zCViewDialogChoice : public zCViewDialog {
public:

    zCViewDialogChoice();
    virtual ~zCViewDialogChoice();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual int HandleEvent(int key);
    virtual void StartSelection();
    virtual void StopSelection();
    virtual void BlitText();
    void ScrollUp();
    void ScrollDown();
    int IsSelectedOutsideAbove();
    int IsSelectedOutsideBelow();
    void ShowSelected();
    void SelectPrevious();
    void SelectNext();
    void Select(int choice);
    zCViewText2* GetSelection();
    zSTRING GetSelectedText();
    void HighlightSelected();
    void AddChoice(zSTRING& text, int numbered);
    void RemoveChoice(zSTRING& text);
    void RemoveChoice(int choice);
    void RemoveAllChoices();

    zCOLOR ColorSelected;
    zCOLOR ColorGrayed;
    int ChoiceSelected;
    int Choices;
    int LineStart;
    int OffsetTextPixelY;
    zCArray<zCViewText2*> choiceList;

    static zCClassDef* classDef;
};

#endif
