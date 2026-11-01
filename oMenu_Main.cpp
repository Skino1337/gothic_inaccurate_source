#include "include/oMenu_Main.h"
#include "include/zMoving.h"
#include "include/zMenuItem.h"
#include "include/oGame.h"

zCTimer* zCMenu::timer = &ztimer;
zCMenu* zCMenu::active = NULL;

int zCMenu::HandleEvent(int)
{
    return 0;
}

void zCMenu::ScreenInit()
{
}

void zCMenu::ScreenDone()
{
}

void zCMenu::Leave()
{
}

void zCMenu::Render()
{
}

int zCMenu::ExecCommand(const zSTRING&)
{
    return 0;
}

zCMenu* zCMenu::GetActive()
{
    return active;
}

int oCMenu_Main::HandleEvent(int key)
{
    zCMenu::HandleEvent(key);
    updateTimer += (int)timer->frameTime;
    if (updateTimer > eventTimerUpdateMSec)
    {
        updateTimer = 0;
        eventOccured[4] = 1;
    }
    return 1;
}

void oCMenu_Main::ScreenInit()
{
    zCMenu::ScreenInit();
}

void oCMenu_Main::ScreenDone()
{
    zCMenu::ScreenDone();
}

void oCMenu_ChgKeys::InitText()
{
    for (int i = 0; i < m_listItems.numInArray; ++i)
    {
        zCMenuItem* item = m_listItems[i];
        if (!item || item->m_parOnChgSetOption.IsEmpty())
            continue;

        unsigned short keys[2] = { 0, 0 };
        zoptions->ReadRaw(zOPT_SEC_KEYS, item->m_parOnChgSetOption, keys, sizeof(keys));
        item->m_parUserString[0] = zCInput::GetNameByControlValue(keys[0]);
        item->m_parUserString[1] = zCInput::GetNameByControlValue(keys[1]);
        zSTRING text = item->m_parUserString[0];
        if (keys[1])
            text += " / " + item->m_parUserString[1];
        item->SetText(text, 0, 0);
    }
}

int oCMenu_ChgKeys::HandleEvent(int key)
{
    zCMenu::HandleEvent(key);
    zCMenuItem* selected = m_listItems[m_mainSel];
    if (key == 0xd3 && selected->m_parOnSelAction[0] == 7)
    {
        zSTRING itemName = selected->m_parOnSelAction_S[0].PickWord(2, " ", "\"");
        if (!itemName.IsEmpty())
        {
            zCMenuItem* item = zCMenuItem::GetByName(itemName);
            item->SetText("", 0, 0);
            item->Draw();
            item->m_parUserString[0].Clear();
            item->m_parUserString[1].Clear();
            unsigned short emptyKey = 0;
            zoptions->WriteRaw(zOPT_SEC_KEYS, item->m_parOnChgSetOption, &emptyKey, 2, 0);
            zinput->BindKeys(0);
        }
    }
    return 1;
}

void oCMenu_ChgKeys::Leave()
{
    zCMenu::Leave();
}

void oCMenu_ChgKeys::ScreenInit()
{
    if (m_screenInitDone)
        return;
    InitText();
    zCMenu::ScreenInit();
}

int oCMenu_ChgKeys::ExecCommand(const zSTRING& command)
{
    zSTRING operation = command.PickWord(1, " ", "\"");
    if (operation == "SETDEFAULT" || operation == "SETALTERNATIVE")
    {
        for (int i = 0; i < m_listItems.numInArray; ++i)
        {
            zCMenuItem* item = m_listItems[i];
            if (!item->m_parOnChgSetOption.IsEmpty())
                zoptions->RemoveEntry(zOPT_SEC_KEYS, item->m_parOnChgSetOption.ToChar());
        }
        zinput->BindKeys(operation == "SETALTERNATIVE");
        InitText();
        for (int i = 0; i < m_listItems.numInArray; ++i)
            m_listItems[i]->Draw();
        return 1;
    }

    if (operation != "RUN")
        return 0;

    zCMenuItem* item = zCMenuItem::GetByName(command.PickWord(2, " ", "\""));
    item->SetIsActive(1);
    unsigned short key;
    do
    {
        sysEvent();
        zinput->ProcessInputEvents();
        zCMenu::GetActive()->Render();
        key = zinput->GetKey(1, 1);
        if (zCInput::GetNameByControlValue(key).IsEmpty())
            key = 0;
    } while (key == 0x1c || !key || (key & 0x8000));
    zinput->ClearKeyBuffer();

    if (key != 1)
    {
        zSTRING keyName = zCInput::GetNameByControlValue(key);
        if (item->m_parUserString[0] == keyName || item->m_parUserString[1] == keyName)
        {
            if (item->m_parUserString[0].IsEmpty())
                item->m_parUserString[0] = keyName;
            else if (item->m_parUserString[1].IsEmpty())
                item->m_parUserString[1] = keyName;
            else
            {
                item->m_parUserString[0] = item->m_parUserString[1];
                item->m_parUserString[1] = keyName;
            }

            zSTRING text = item->m_parUserString[0];
            if (!item->m_parUserString[1].IsEmpty())
                text += " / " + item->m_parUserString[1];
            item->SetText(text, 0, 0);
            item->Draw();

            for (int i = 0; i < m_listItems.numInArray; ++i)
            {
                zCMenuItem* other = m_listItems[i];
                if (other == item)
                    continue;
                if (other->m_parUserString[0] == keyName || other->m_parUserString[1] == keyName)
                {
                    other->m_parUserString[0].Clear();
                    other->m_parUserString[1].Clear();
                    other->SetText("", 0, 0);
                    other->Draw();
                    unsigned short emptyKey = 0;
                    zoptions->WriteRaw(zOPT_SEC_KEYS, other->m_parOnChgSetOption,
                        &emptyKey, 2, 0);
                }
            }

            unsigned short keys[2] = { 0, 0 };
            int count = 0;
            if (!item->m_parUserString[0].IsEmpty())
                keys[count++] = zCInput::GetControlValueByName(item->m_parUserString[0]);
            if (!item->m_parUserString[1].IsEmpty())
                keys[count++] = zCInput::GetControlValueByName(item->m_parUserString[1]);
            zoptions->WriteRaw(zOPT_SEC_KEYS, item->m_parOnChgSetOption, keys,
                count ? count * 2 : 2, 0);
            zinput->BindKeys(0);
        }
    }
    item->SetIsActive(0);
    return 1;
}
