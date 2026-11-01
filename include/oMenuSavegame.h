#ifndef OMENUSAVEGAME_H
#define OMENUSAVEGAME_H

#include "oGame.h"
#include "oSavegame.h"
#include "oMenu_Main.h"
#include "zMenuItem.h"
#include "zTexture.h"

class oCMenuSavegame : public oCMenu_Main {
public:

    enum oTMenuSavegameMode {
        SAVE,
        LOAD
    };

    oCMenuSavegame(const zSTRING& name, oTMenuSavegameMode mode);
    virtual ~oCMenuSavegame();
    int GetMenuItemSlotNr(zCMenuItem* item);
    void SetScreenshot(zCTextureConvert* converter);
    virtual int HandleEvent(int key);
    virtual void Activate();
    virtual void ScreenInit();
    virtual void ScreenDone();
    virtual void HandleSlotChange(int slot);
    virtual int GetSelectedSlot();

    oTMenuSavegameMode mode;
    oCSavegameManager* savegameManager;
    int m_selSlot;
    int m_selSlotItemIndex;
    zCTexture* m_thumbPic;
    zCMenuItem* m_item_PlayTime;
    zCMenuItem* m_item_DateTime;
    zCMenuItem* m_item_WorldName;
    zCMenuItem* m_item_GameTime;
    zCMenuItem* m_item_Thumbpic;
    zCView* m_item_Thumbpic2;
};

#endif
