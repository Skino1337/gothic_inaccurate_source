#include "include/oMenu_Status.h"
#include "include/oGame.h"
#include "include/oNpcMessages.h"
#include "include/zMoving.h"

oCStatusScreen::oCStatusScreen()
    : m_pStatusMenu(new oCMenu_Status("MENU_STATUS"))
{
}

oCStatusScreen::~oCStatusScreen()
{
    if (m_pStatusMenu)
    {
        m_pStatusMenu->Release();
        m_pStatusMenu = NULL;
    }
}

void oCStatusScreen::Show()
{
    zinput->ResetRepeatKey(1);
    if (m_pStatusMenu)
    {
        if (ogame)
            ogame->Pause(0);
        m_pStatusMenu->InitForDisplay();
        m_pStatusMenu->Run();
        if (ogame)
            ogame->Unpause();
    }
    zinput->ResetRepeatKey(1);
}

oCLogScreen::oCLogScreen()
    : m_pLogMenu(new oCMenu_Log("MENU_LOG"))
{
}

oCLogScreen::~oCLogScreen()
{
    if (m_pLogMenu)
    {
        m_pLogMenu->Release();
        m_pLogMenu = NULL;
    }
}

void oCLogScreen::Show()
{
    zinput->ResetRepeatKey(1);
    if (m_pLogMenu)
    {
        if (ogame)
            ogame->Pause(0);
        m_pLogMenu->Run();
        if (ogame)
            ogame->Unpause();
    }
    zinput->ResetRepeatKey(1);
}

void oCMapScreen::Show(int)
{
    zerr->Report(zERR_TYPE_INFO, 0, "B: STAT: Entering Map-Screen ...", 3, 1, 0x2A3, 0, 0);

    if (ogame && oCNpc::player)
    {
        oCWorld* world = dynamic_cast<oCWorld*>(ogame->GetWorld());
        if (world)
        {
            zSTRING mapName = "ITWR_MAP_" + world->GetWorldName();
            if (oCNpc::player->IsInInv(mapName, 1))
            {
                oCMsgManipulate* message = new oCMsgManipulate(5, mapName, -1);
                oCNpc::player->GetEM(0)->OnMessage(message, oCNpc::player);
            }
        }
    }

    zerr->Report(zERR_TYPE_INFO, 0, "", 3, 2, 0x2B6, 0, 0);
}
