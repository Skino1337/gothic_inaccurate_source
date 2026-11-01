#include "include/oMenuSavegame.h"
#include "include/zMoving.h"
#include <cstring>
#include <cstdlib>

static int savegameMenuState = -1;
static zSTRING savegameEmptyString;
static int savegameItemState = -1;
static zSTRING savegameListBoxLoadGame = "MENU_ITEM_LISTBOX_LOADGAME";
static zSTRING savegameListBoxSaveGame = "MENU_ITEM_LISTBOX_SAVEGAME";
static zSTRING savegameOptControls = "MENU_OPT_CONTROLS";
static zSTRING savegameOptControlsExtKeys = "MENU_OPT_CONTROLS_EXTKEYS";
static zSTRING savegameThumbpicItem = "MENUITEM_LOADSAVE_THUMBPIC";
static zSTRING savegameDateTimeItem = "MENUITEM_LOADSAVE_DATETIME_VALUE";
static zSTRING savegameGameTimeItem = "MENUITEM_LOADSAVE_GAMETIME_VALUE";
static zSTRING savegameLevelNameItem = "MENUITEM_LOADSAVE_LEVELNAME_VALUE";
static zSTRING savegamePlayTimeItem = "MENUITEM_LOADSAVE_PLAYTIME_VALUE";

oCMenuSavegame::oCMenuSavegame(const zSTRING& name, oTMenuSavegameMode saveMode)
    : oCMenu_Main(name), mode(saveMode), savegameManager(NULL), m_selSlot(-1),
      m_selSlotItemIndex(-1), m_thumbPic(NULL), m_item_PlayTime(NULL),
      m_item_DateTime(NULL), m_item_WorldName(NULL), m_item_GameTime(NULL),
      m_item_Thumbpic(NULL), m_item_Thumbpic2(NULL)
{
}

oCMenuSavegame::~oCMenuSavegame()
{
    m_item_PlayTime = NULL;
    m_item_DateTime = NULL;
    m_item_WorldName = NULL;
    m_item_GameTime = NULL;
    m_item_Thumbpic = NULL;
    m_item_Thumbpic2 = NULL;
    if (m_thumbPic) {
        m_thumbPic->Release();
        m_thumbPic = NULL;
    }
}

void oCMenuSavegame::Activate()
{
    zCMenu::Activate();
}

void oCMenuSavegame::HandleSlotChange(int)
{
    oCSavegameInfo* info = savegameManager->GetSavegame(m_selSlot);
    if (m_item_PlayTime && info) {
        zSTRING text;
        text.Sprintf("%dh %dmin", info->m_PlayTimeSeconds / 3600,
            info->m_PlayTimeSeconds / 60 % 60);
        m_item_PlayTime->SetText(text, 0, 0);
    }
    if (m_item_WorldName) {
        zSTRING text;
        if (info && !info->m_notCompatible)
            text = info->GetWorldName();
        text.UpperFirstLowerRest();
        if (info && !info->m_notCompatible && !text.IsEmpty()
            && (info->m_verMajor != 2 || info->m_verMinor != 6
                || info->m_verPoint != 0 || info->m_verInt != 0)) {
            text += " (" + zSTRING(info->m_verMajor) + "." + zSTRING(info->m_verMinor);
            if (info->m_verPoint >= 0)
                text += "." + zSTRING(info->m_verPoint);
            if (info->m_verInt >= 0)
                text += "." + zSTRING(info->m_verInt);
            text += ")";
        }
        m_item_WorldName->SetText(text, 0, 1);
    }
    if (m_item_GameTime) {
        zSTRING text;
        if (info && !info->m_notCompatible)
            text = info->GetGameTime();
        m_item_GameTime->SetText(text, 0, 1);
    }
    if (m_item_DateTime) {
        zSTRING text;
        if (info && !info->m_notCompatible)
            text = info->GetSaveTime();
        m_item_DateTime->SetText(text, 0, 1);
    }
    if (m_item_Thumbpic) {
        zCTexture* thumbnail = NULL;
        if (info && !info->m_notCompatible)
            thumbnail = info->m_ThumbTex;
        if (!thumbnail)
            info->ReloadResources();
        if (info && !info->m_notCompatible)
            thumbnail = info->m_ThumbTex;
        if (thumbnail)
            m_item_Thumbpic->InsertBack(thumbnail);
        else
            m_item_Thumbpic->InsertBack("");
    }
}

int oCMenuSavegame::HandleEvent(int key)
{
    int result = zCMenu::HandleEvent(key);
    const int oldSlot = m_selSlot;
    if (m_mainSel >= 0 && m_mainSel < m_listItems.numInArray) {
        m_selSlotItemIndex = m_mainSel;
        m_selSlot = GetMenuItemSlotNr(m_listItems.parray[m_mainSel]);
    }
    if (oldSlot != m_selSlot)
        HandleSlotChange(oldSlot);
    return result;
}

void oCMenuSavegame::ScreenInit()
{
    oCMenu_Main::ScreenInit();
    m_item_Thumbpic = zCMenuItem::GetByName("MENUITEM_THUMBPIC");
    m_item_DateTime = zCMenuItem::GetByName("MENUITEM_DATETIME");
    m_item_WorldName = zCMenuItem::GetByName("MENUITEM_WORLDNAME");
    m_item_GameTime = zCMenuItem::GetByName("MENUITEM_GAMETIME");
    m_item_PlayTime = zCMenuItem::GetByName("MENUITEM_PLAYTIME");
    if (m_item_Thumbpic && m_thumbPic) {
        int x1;
        int y1;
        int x2;
        int y2;
        m_item_Thumbpic->GetCoords(x1, y1, x2, y2);
        m_item_Thumbpic2 = new zCView(x1, y1, x2, y2, 2);
        m_item_Thumbpic2->InsertBack(m_thumbPic);
        screen->InsertItem(m_item_Thumbpic2, 0);
    }
    for (int i = 0; i < m_listItems.numInArray; ++i) {
        zCMenuItem* item = m_listItems.parray[i];
        const int slot = GetMenuItemSlotNr(item);
        if (slot < 0 || slot > 20)
            continue;
        oCSavegameInfo* info = savegameManager->GetSavegame(slot);
        zSTRING text = info ? info->GetName() : zSTRING("---");
        if (info && info->m_notCompatible)
            text = "xxx";
        if (text.IsEmpty())
            text = "???";
        if (slot)
            item->SetText(text, 0, 0);
        item->SetIsDisabled(0);
        item->SetIsSelectable(mode != SAVE || info != NULL);
    }
    if (m_mainSel >= 0 && m_mainSel < m_listItems.numInArray)
        m_selSlot = GetMenuItemSlotNr(m_listItems.parray[m_mainSel]);
    HandleSlotChange(-1);
}

void oCMenuSavegame::ScreenDone()
{
    for (int i = 0; i < m_listItems.numInArray; ++i) {
        const int slot = GetMenuItemSlotNr(m_listItems.parray[i]);
        if (slot < 0)
            continue;
        oCSavegameInfo* info = savegameManager->GetSavegame(slot);
        if (info)
            info->m_Name = m_listItems.parray[i]->GetText(0);
    }
    oCMenu_Main::ScreenDone();
    if (savegameManager)
        savegameManager->CleanResources();
    delete m_item_Thumbpic2;
    m_item_Thumbpic2 = NULL;
    if (m_item_Thumbpic) {
        --m_item_Thumbpic->m_iRefCtr;
        if (m_item_Thumbpic->m_iRefCtr <= 0 && !m_item_Thumbpic->registeredCPP)
            delete m_item_Thumbpic;
        m_item_Thumbpic = NULL;
    }
    zinput->ClearKeyBuffer();
}

int oCMenuSavegame::GetMenuItemSlotNr(zCMenuItem* item)
{
    if (!item)
        return -1;
    const char* marker = std::strstr(item->GetName().ToChar(), "_SLOT");
    if (!marker)
        return -1;
    const int slot = std::atoi(marker + 5);
    return slot >= 0 && slot <= 20 ? slot : -1;
}

int oCMenuSavegame::GetSelectedSlot()
{
    return m_selSlot;
}

void oCMenuSavegame::SetScreenshot(zCTextureConvert* converter)
{
    if (m_thumbPic) {
        m_thumbPic->Release();
        m_thumbPic = NULL;
    }
    if (converter) {
        zCTextureInfo info = converter->GetTextureInfo();
        info.texFormat = zCTexture::CalcNextBestTexFormat(zRND_TEX_FORMAT_RGB_565);
        info.sizeX = 256;
        info.sizeY = 256;
        info.numMipMap = 1;
        converter->SetTextureInfo(info);
        m_thumbPic = zrenderer->CreateTexture();
        zCTextureExchange::CopyContents(converter, m_thumbPic);
    }
    if (m_thumbPic)
        m_thumbPic->AddRef();
}
