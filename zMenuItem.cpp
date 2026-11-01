#include "include/zMenuItem.h"
#include "include/zMenu.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zFonts.h"
#include "include/zMoving.h"
#include <string.h>

static int menuItemState = -1;
static zSTRING menuItemHorizontalRule = "<hr>";
static zSTRING menuItemSeparator = "---";
static zSTRING menuItemOTexture = "o.tga";
static zSTRING menuItemUTexture = "u.tga";
zCArraySort<zCMenuItem*> zCMenuItem::itemList;
zCMenuItem* zCMenuItem::tmp;

int zCMenuItem::CompareNames(const void* first, const void* second)
{
    const zCMenuItem* firstItem = *static_cast<zCMenuItem* const*>(first);
    const zCMenuItem* secondItem = *static_cast<zCMenuItem* const*>(second);
    int result = strcmp(firstItem->id.ToChar(), secondItem->id.ToChar());
    return result < 0 ? -1 : result > 0 ? 1 : 0;
}

void zCMenuItem::Startup()
{
    itemList.Compare = CompareNames;
    tmp = new zCMenuItem();
}

void zCMenuItem::Shutdown()
{
    while (itemList.numInArray) {
        zCMenuItem* item = itemList.array[0];
        item->registeredCPP = 0;
        item->Unregister();
        item->Release();
    }

    if (tmp)
        tmp->Release();
    tmp = NULL;
}

zCMenuItem* zCMenuItem::GetByName(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();

    for (int i = 0; i < itemList.numInArray; i++) {
        if (itemList.array[i]->id == upperName) {
            itemList.array[i]->m_iRefCtr++;
            return itemList.array[i];
        }
    }

    return NULL;
}

zCMenuItem* zCMenuItem::GetSelItem()
{
    for (int i = 0; i < MAX_SEL_ACTIONS; i++) {
        if (!m_parOnSelAction[i])
            return NULL;
        if (m_parOnSelAction[i] == SEL_ACTION_STARTITEM)
            return GetByName(m_parOnSelAction_S[i]);
    }

    return NULL;
}

zCMenuItem* zCMenuItem::Create(const zSTRING& name)
{
    if (name.IsEmpty())
        return NULL;

    zCMenuItem* item = GetByName(name);
    if (item)
        return item;

    zSTRING upperName = name;
    upperName.Upper();

    if (!zCMenu::GetParser()->GetSymbol(upperName))
        return NULL;

    zCMenu::GetParser()->CreateInstance(upperName, &tmp->m_parFontName);

    switch (tmp->m_parType) {
    case MENU_ITEM_TEXT:
        item = new zCMenuItemText(upperName);
        break;
    case MENU_ITEM_SLIDER:
        item = new zCMenuItemSlider(upperName);
        break;
    case MENU_ITEM_INPUT:
        item = new zCMenuItemInput(upperName);
        break;
    case MENU_ITEM_CHOICEBOX:
        item = new zCMenuItemChoice(upperName);
        break;
    case MENU_ITEM_BUTTON:
        item = new zCMenuItemButton(upperName);
        break;
    case MENU_ITEM_LISTBOX:
        item = new zCMenuItemList(upperName);
        break;
    default:
        return NULL;
    }

    item->InitView();
    item->InitMenuItem();
    return item;
}

void zCMenuItem::Release()
{
    --m_iRefCtr;
    if (m_iRefCtr <= 0 && !registeredCPP)
        delete this;
}

void zCMenuItem::SetMinValue(float value)
{
}

void zCMenuItem::SetMaxValue(float value)
{
}

int zCMenuItem::HasBeenCanceled()
{
    return 0;
}

void zCMenuItemInput::Enter()
{
    canceled = 0;
}

void zCMenuItemInput::Leave()
{
}

int zCMenuItemInput::HandleEvent(int key)
{
    return 1;
}

int zCMenuItemInput::HasBeenCanceled()
{
    return canceled;
}

void zCMenuItemSlider::SetIsActive(int value)
{
    zCMenuItem::SetIsActive(value);
    SetEnableHandleEvent(value);
}

int zCMenuItemSlider::HandleEvent(int key)
{
    return 1;
}

zCMenuItem::zCMenuItem()
{
    Init();
}

zCMenuItem::zCMenuItem(const zSTRING& name)
{
    Init();
    id = name;
    id.Upper();
    SetByScript(id);
    registeredCPP = 1;
}

zCMenuItem::~zCMenuItem()
{
    m_pFont = NULL;
    m_pFontHi = NULL;
    m_pFontSel = NULL;
    m_pFontDis = NULL;
    delete m_pInnerWindow;
    m_pInnerWindow = NULL;
    m_listLines.DeleteList();
}

void zCMenuItem::Init()
{
    for (int i = 0; i < MENU_EVENT_MAX; i++)
        m_parOnEventAction[i] = 0;

    for (int i = 0; i < MAX_SEL_ACTIONS; i++) {
        m_parOnSelAction[i] = 0;
        m_parOnSelAction_S[i].Clear();
    }

    inserted = 0;
    changed = 1;
    active = 0;
    open = 0;
    close = 0;
    opened = 0;
    closed = 0;
    disabled = 0;
    orgWin = NULL;
    fxTimer = 0.0f;
    openDelayTimer = 0.0f;
    activeTimer = 0.0f;
    registeredCPP = 0;
    m_parAlpha = 254;
    m_parType = MENU_ITEM_UNDEF;
    m_parOpenDuration = -1.0f;
    m_parSizeStartScale = 1.0f;
    m_parItemFlags = 0;
    m_parOpenDelayTime = 0.0f;
    m_parPosX = 0;
    m_parPosY = 0;
    m_parDimX = -1;
    m_parDimY = -1;
    m_parFrameSizeX = 0;
    m_parFrameSizeY = 0;
    m_bVisible = 1;
    m_bDontRender = 0;
    m_iRefCtr = 1;
    m_pInnerWindow = NULL;
    m_pFont = NULL;
    m_pFontHi = NULL;
    m_pFontSel = NULL;
    m_pFontDis = NULL;
    m_bViewInitialized = 0;
    m_bLeaveItem = 0;
    firstTimeInserted = 1;
    m_parFontName.Clear();
    m_parBackPic.Clear();
    m_parAlphaMode.Clear();
    m_parOnChgSetOption.Clear();
    m_parOnChgSetOptionSection.Clear();
    id.Clear();

    for (int i = 0; i < MAX_USERSTRINGS; i++)
        m_parText[i].Clear();

    for (int i = 0; i < MAX_USERVARS; i++) {
        m_parUserFloat[i] = 0.0f;
        m_parUserString[i].Clear();
    }

    m_parHideIfOptionSet.Clear();
    m_parHideIfOptionSectionSet.Clear();
    m_parHideOnValue = 0;
}

void zCMenuItem::SetByScript(zSTRING& script)
{
    id = script;
    id.Upper();

    if (zCMenu::GetParser()->GetSymbol(id))
        zCMenu::GetParser()->CreateInstance(id, this);

    m_parAlphaMode.Upper();
    SetAlphaBlendFunc(zrenderer->AlphaBlendFuncStringToType(m_parAlphaMode));
    m_pFont = zfontman->GetFont(zfontman->Load(m_parFontName));
    m_pFontHi = m_pFont;
    m_pFontSel = m_pFont;
    m_pFontDis = m_pFont;
    m_parBackPic.Upper();
    Register();

    int lastText = -1;
    for (int i = 0; i < MAX_USERSTRINGS; i++) {
        if (!m_parText[i].IsEmpty())
            lastText = i;
    }

    for (int i = 0; i <= lastText; i++)
        m_listLines.InsertEnd(m_parText[i]);
}

void zCMenuItem::InsertInWin(zCView* window)
{
    if (owner)
        RemoveFromWin();
    if (!m_bVisible)
        return;

    orgWin = window;
    window->InsertItem(this);
    SetPos(m_parPosX, m_parPosY);
    SetTransparency(m_parAlpha);
    SetFont(m_parFontName);
    SetSize(GetDimX(m_parDimX), GetDimY(m_parDimY));

    if (!m_parBackPic.IsEmpty())
        InsertBack(m_parBackPic);

    m_pInnerWindow = new zCView(m_parFrameSizeX, m_parFrameSizeY,
        8192 - m_parFrameSizeX, 8192 - m_parFrameSizeY);
    InsertItem(m_pInnerWindow);
    m_pInnerWindow->SetPos(m_parFrameSizeX, m_parFrameSizeY);
    m_pInnerWindow->SetSize((4096 - m_parFrameSizeX) * 2, (4096 - m_parFrameSizeY) * 2);
    m_pInnerWindow->SetFont(m_parFontName);
    m_pInnerWindow->SetFontColor(fontColor);
    m_pInnerWindow->SetAlphaBlendFunc(zrenderer->AlphaBlendFuncStringToType(m_parAlphaMode));
    if (m_parItemFlags & 0x10)
        m_pInnerWindow->Setflags(0x200);
    else
        m_pInnerWindow->ClrFlags(0x200);
    inserted = 1;
    UpdateFX();
    SetSize(GetDimX(m_parDimX), GetDimY(m_parDimY));
    ClrPrintwin();
    DrawFront();
}

void zCMenuItem::RemoveFromWin()
{
    delete m_pInnerWindow;
    m_pInnerWindow = NULL;

    if (orgWin)
        orgWin->RemoveItem(this);

    active = 0;
    open = 0;
    close = 0;
    opened = 0;
    closed = 0;
    activeTimer = 0.0f;
    fxTimer = 0.0f;
    openDelayTimer = 0.0f;
    inserted = 0;
}

int zCMenuItem::GetDimX(int value)
{
    if (value >= 0)
        return value;

    zCView* view = m_pInnerWindow ? m_pInnerWindow : this;
    zCFont* font = view->GetFont();

    if (!font || !owner)
        return 0;

    return owner->anx(font->GetFontX(GetText(0)) + nax(m_parFrameSizeX));
}

int zCMenuItem::GetDimY(int value)
{
    if (value >= 0)
        return value;

    zCView* view = m_pInnerWindow ? m_pInnerWindow : this;
    zCFont* font = view->GetFont();

    if (!font || !owner)
        return 0;

    return owner->any(font->GetFontY() + nay(m_parFrameSizeY));
}

void zCMenuItem::InitMenuItem()
{
    SetText(GetText(0), 0, 0);
}

void zCMenuItem::InitView()
{
    if (m_bViewInitialized)
        return;

    if (m_parDimX == -1 || m_parDimY == -1)
        Init(0, 0, GetText(0), 1);
    else
        Init(0, 0, m_parDimX, m_parDimY, 1);

    SetPos(m_parPosX, m_parPosY);
    m_bViewInitialized = 1;
}

int zCMenuItem::Show()
{
    if (orgWin) {
        orgWin->InsertItem(this);
        InsertItem(m_pInnerWindow);
    }

    return int(orgWin);
}

int zCMenuItem::Hide()
{
    if (!orgWin)
        return 0;

    if (m_pInnerWindow)
        RemoveItem(m_pInnerWindow);
    orgWin->RemoveItem(this);
    return 1;
}

void zCMenuItem::Open()
{
    open = 1;
    opened = 0;
    close = 0;
    closed = 0;
    openDelayTimer = 0.0f;
}

void zCMenuItem::Close()
{
    close = 1;
    closed = 0;
}

int zCMenuItem::ToggleValue(int direction, int alternate)
{
    if ((m_parItemFlags & 1024) && !(m_parItemFlags & 4096))
        zoptions->WriteInt("INTERNAL", "perfQualityIndex", 0, 0);
    return 0;
}

int zCMenuItem::GetType()
{
    return m_parType;
}

zSTRING zCMenuItem::GetText(int index)
{
    if (index < m_listLines.numInArray)
        return m_listLines.parray[index];
    return zSTRING();
}

void zCMenuItem::SetName(const zSTRING& name)
{
    id = name;
}

zSTRING& zCMenuItem::GetName()
{
    return id;
}

void zCMenuItem::GetCoords(int& x1, int& y1, int& x2, int& y2)
{
    GetScreenExtends(x1, y1, x2, y2);
}

int zCMenuItem::GetSelActionType(int index)
{
    return m_parOnSelAction[index];
}

zSTRING& zCMenuItem::GetSelActionString(int index)
{
    return m_parOnSelAction_S[index];
}

void zCMenuItem::SetName(zSTRING& name)
{
    id = name;
    id.Upper();
}

void zCMenuItem::Register()
{
    if (!IsRegistered()) {
        itemList.InsertSort(this);
        m_iRefCtr++;
    }
}

void zCMenuItem::Unregister()
{
    for (int i = 0; i < itemList.numInArray; i++) {
        if (itemList.array[i] == this) {
            for (int j = i + 1; j < itemList.numInArray; j++)
                itemList.array[j - 1] = itemList.array[j];
            itemList.numInArray--;
            Release();
            return;
        }
    }
}

int zCMenuItem::IsRegistered()
{
    for (int i = 0; i < itemList.numInArray; i++) {
        if (itemList.array[i] == this)
            return 1;
    }

    return 0;
}

void zCMenuItem::SetText(const zSTRING& text, int index, int animated)
{
    if (index < 0)
        return;

    while (m_listLines.numInArray <= index)
        m_listLines.InsertEnd(zSTRING());

    m_listLines.parray[index] = text;
    UpdateSize();
    DrawFront();

    if (animated)
        DrawItems();
}

void zCMenuItem::UpdateSize()
{
    SetSize(GetDimX(m_parDimX), GetDimY(m_parDimY));

    if (m_pInnerWindow) {
        m_pInnerWindow->SetPos(m_parFrameSizeX, m_parFrameSizeY);
        m_pInnerWindow->SetSize((4096 - m_parFrameSizeX) * 2, (4096 - m_parFrameSizeY) * 2);
    }
}

void zCMenuItem::SetText(zCArray<zSTRING> text, int animated)
{
    m_listLines.DeleteList();

    for (int i = 0; i < text.numInArray; i++)
        m_listLines.InsertEnd(text.parray[i]);

    DrawFront();

    if (animated)
        DrawItems();
}

void zCMenuItem::Enter()
{
}

void zCMenuItem::Leave()
{
}

int zCMenuItem::HandleEvent(int key)
{
    if (key == 1 || key == 525)
        m_bLeaveItem = 1;

    int function = m_parOnEventAction[MENU_EVENT_CLOSE];
    if (!function)
        return 1;

    return int(zCMenu::GetParser()->CallFunc(function));
}

int zCMenuItem::Run()
{
    SetIsActive(1);
    SetEnableHandleEvent(1);
    Enter();
    m_bLeaveItem = 0;

    while (!m_bLeaveItem)
        zCMenu::GetActive()->HandleFrame(1);

    Leave();
    SetEnableHandleEvent(0);
    SetIsActive(0);
    return 0;
}

int zCMenuItem::Input(int maxLength, zSTRING& text)
{
    SetText(text + "_", 0, 0);

    while (true) {
        zCMenu::GetActive()->HandleFrame(1);
        int key = zinput->GetKey(1, 1);

        if (key == 28 || key == 524 || key == 1 || key == 525) {
            SetText(text, 0, 0);
            return key == 28 || key == 524;
        }

        if (key == 211) {
            text.Clear();
            SetText("_", 0, 0);
            continue;
        }

        if (key == 14) {
            if (text.Length() > 0)
                text.Delete(text.Length() - 1, 1);
            SetText(text + "_", 0, 0);
            continue;
        }

        int shift = zinput->KeyPressed(54) || zinput->KeyPressed(42);
        int character = GetCharFromDIK(key, shift, zinput->KeyPressed(184));

        if (text.Length() < maxLength && character >= 32 && character <= 255) {
            char value[2] = {char(character), 0};
            text += value;
            SetText(text + "_", 0, 0);
        }
    }
}

void zCMenuItem::Draw()
{
    if (m_bDontRender)
        return;

    DrawFront();
    DrawItems();
}

void zCMenuItem::DrawFront()
{
    if (!owner || !m_pInnerWindow || m_bDontRender)
        return;

    zSTRING text = GetText(0);
    UpdateFX();
    m_pInnerWindow->ClrPrintwin();

    if (m_parItemFlags & 2048)
        m_pInnerWindow->Printwin(text + "\n");
    else if (m_parItemFlags & 16)
        m_pInnerWindow->PrintCXY(text);
    else
        m_pInnerWindow->Print(0, 0, text);
}

void zCMenuItem::DrawItems()
{
    if (m_bDontRender)
        return;

    zCView::DrawItems();

    if (m_pInnerWindow)
        UpdateContent();
}

void zCMenuItem::UpdateContent()
{
}

void zCMenuItem::UpdateFX()
{
    if (!m_bVisible)
        return;

    int disable = (m_parItemFlags & 32) ||
        ((m_parItemFlags & 512) && !zCMenu::inGameMenu) ||
        ((m_parItemFlags & 256) && zCMenu::inGameMenu);
    zCFont* font = active ? m_pFontSel : disable ? m_pFontDis : m_pFont;
    zCOLOR color = disable ? zCOLOR(175, 175, 175, 220) : zCOLOR(255, 255, 255, 255);
    m_pInnerWindow->SetFont(font);
    m_pInnerWindow->SetFontColor(color);
    m_pInnerWindow->SetTransparency(m_parAlpha);
    UpdateSize();
}

int zCMenuItem::GetEventFunc(int event)
{
    return m_parOnEventAction[event];
}

void zCMenuItem::SetIsActive(int value)
{
    active = value;
    zCMenu* menu = zCMenu::GetActive();

    if (menu) {
        if (active)
            menu->PrintInformation(m_parText[1]);
        else
            menu->PrintInformation(zSTRING());
    }

    DrawFront();
}

void zCMenuItem::SetIsSelectable(int selectable)
{
    if (selectable)
        m_parItemFlags |= 4;
    else {
        m_parItemFlags &= ~4;
        if (active)
            zCMenu::GetActive()->NextOption();
    }

    DrawFront();
}

int zCMenuItem::GetIsSelectable()
{
    if (!m_bVisible || m_bDontRender || (m_parItemFlags & 32) ||
            ((m_parItemFlags & 512) && !zCMenu::inGameMenu) ||
            ((m_parItemFlags & 256) && zCMenu::inGameMenu))
        return 0;

    return m_parItemFlags & 4;
}

void zCMenuItem::SetIsDisabled(int value)
{
    if (value)
        m_parItemFlags |= 32;
    else
        m_parItemFlags &= ~32;
    DrawFront();
}

int zCMenuItem::GetIsDisabled()
{
    if (!m_bVisible || ((m_parItemFlags & 512) && !zCMenu::inGameMenu) ||
            ((m_parItemFlags & 256) && zCMenu::inGameMenu))
        return 1;

    return m_parItemFlags & 32;
}

void zCMenuItem::SetIsVisible(int visible)
{
    m_bVisible = visible;
}

int zCMenuItem::GetIsVisible()
{
    return m_bVisible;
}

zCMenuItemInput::zCMenuItemInput(const zSTRING& name) : zCMenuItem(name)
{
    value.Clear();
    curPos = 0;
    canceled = 0;
}

zCMenuItemInput::~zCMenuItemInput()
{
    SetEnableHandleEvent(0);
}

void zCMenuItemInput::SetValue(zSTRING newValue)
{
    value = newValue;
    SetText(value, 0, 0);
}

zSTRING zCMenuItemInput::GetValue()
{
    return GetText(0);
}

void zCMenuItemInput::InitMenuItem()
{
    zCMenuItem::InitMenuItem();

    if (!m_parOnChgSetOption.IsEmpty() &&
            zoptions->EntryExists(m_parOnChgSetOptionSection, m_parOnChgSetOption.ToChar())) {
        zSTRING newValue = zoptions->ReadString(m_parOnChgSetOptionSection, m_parOnChgSetOption, "");
        SetText(newValue, 0, 1);
        zoptions->WriteString(m_parOnChgSetOptionSection, m_parOnChgSetOption, newValue,
            (m_parItemFlags & 4096) != 0);
    }
}

void zCMenuItemInput::InsertInWin(zCView* window)
{
    zCMenuItem::InsertInWin(window);

    if (m_bVisible)
        Draw();
}

int zCMenuItemInput::Run()
{
    canceled = 0;
    SetIsActive(1);
    SetEnableHandleEvent(1);
    Enter();
    zSTRING oldValue = GetText(0);
    zSTRING newValue = oldValue;
    canceled = !Input(80, newValue);

    if (!(oldValue == newValue) && !m_parOnChgSetOption.IsEmpty())
        zoptions->WriteString(m_parOnChgSetOptionSection, m_parOnChgSetOption, newValue,
            (m_parItemFlags & 4096) != 0);

    SetText(newValue, 0, 0);
    Leave();
    SetEnableHandleEvent(0);
    SetIsActive(0);
    return 0;
}

zCMenuItemText::zCMenuItemText(const zSTRING& name) : zCMenuItem(name)
{
    m_fullText.Clear();
    m_mode = MODE_SIMPLE;
    m_numOptions = 0;
    m_topLine = 1;
    m_unformated = 1;
    m_viewLines = -1;
    m_numLines = -1;
    ClrFlags(512);
}

zCMenuItemText::~zCMenuItemText()
{
}

void zCMenuItemText::DrawFront()
{
    if (!owner || !m_pInnerWindow)
        return;

    UpdateFX();

    if (m_mode == MODE_SIMPLE || m_mode == MODE_ENUM)
        zCMenuItem::DrawFront();
    else if (m_mode == MODE_MULTILINE)
        DrawMultiline();
}

void zCMenuItemText::FormatMultiline()
{
    m_listLines.DeleteList();
    zSTRING current;

    for (int index = 1; ; index++) {
        zSTRING word = m_fullText.PickWord(index, " ", "");
        if (word.IsEmpty())
            break;

        zSTRING candidate = current;
        if (!candidate.IsEmpty())
            candidate += " ";
        candidate += word;

        if (!current.IsEmpty() && m_pInnerWindow->FontSize(candidate) > 8100) {
            current.TrimLeft(' ');
            m_listLines.InsertEnd(current);
            current = word;
        }
        else
            current = candidate;
    }

    current.TrimLeft(' ');
    m_listLines.InsertEnd(current);
    m_numLines = m_listLines.numInArray;
    m_viewLines = 8100 / m_pInnerWindow->FontY();
    m_topLine = m_numLines - m_viewLines + 1;
    if (m_topLine < 1)
        m_topLine = 1;
    m_unformated = 0;
}

void zCMenuItemText::DrawMultiline()
{
    ClrPrintwin();
    m_pInnerWindow->ClrPrintwin();

    if (m_unformated)
        FormatMultiline();

    int y = 0;
    m_viewLines = 8100 / m_pInnerWindow->FontY();

    for (int i = m_topLine - 1; i < m_listLines.numInArray && y < 8100; i++) {
        if (m_parItemFlags & 16)
            m_pInnerWindow->PrintCX(0, m_listLines.parray[i]);
        else
            m_pInnerWindow->Print(0, y, m_listLines.parray[i]);
        y += m_pInnerWindow->FontY();
    }

    if (m_numLines > m_viewLines && m_viewLines > 0) {
        int pages = (m_numLines + m_viewLines - 1) / m_viewLines;
        int page = (m_topLine + m_viewLines - 1) / m_viewLines;
        PrintCX(0, zSTRING(page) + "/" + zSTRING(pages));
    }
}

void zCMenuItemText::InitMenuItem()
{
    zCMenuItem::InitMenuItem();
    m_fullText = GetText(0);

    if (!m_parOnChgSetOption.IsEmpty() &&
            zoptions->EntryExists(m_parOnChgSetOptionSection, m_parOnChgSetOption.ToChar()))
        m_fullText = zoptions->ReadString(m_parOnChgSetOptionSection, m_parOnChgSetOption, "");

    m_mode = (m_parItemFlags & 2048) ? MODE_MULTILINE : MODE_SIMPLE;

    if (m_mode != MODE_MULTILINE && m_fullText.Search("|", 0) >= 0)
        SetEnum(m_fullText);
}

void zCMenuItemText::SetEnum(const zSTRING& text)
{
    m_fullText = text;
    m_numOptions = 0;

    for (int index = 1; ; index++) {
        if (m_fullText.PickWord(index, "|", "").IsEmpty())
            break;
        m_numOptions++;
    }

    if (m_numOptions > 1)
        m_mode = MODE_ENUM;
    m_topLine = 1;
    SetText("0", 0, 0);
}

int zCMenuItemText::HandleEvent(int key)
{
    if (m_mode == MODE_ENUM && m_numOptions > 0) {
        int oldSelection = m_topLine;

        if (key == 200 || key == 199)
            m_topLine = m_topLine <= 1 ? m_numOptions : m_topLine - 1;
        else if (key == 208 || key == 207)
            m_topLine = m_topLine >= m_numOptions ? 1 : m_topLine + 1;

        if (oldSelection != m_topLine)
            SetText(zSTRING(m_topLine - 1), 0, 0);
    }
    else if (m_mode == MODE_MULTILINE) {
        if (key == 199)
            m_topLine = 1;
        else if (key == 200 && m_topLine > 1)
            m_topLine--;
        else if (key == 208 && m_topLine < m_numLines)
            m_topLine++;
        else if (key == 207)
            m_topLine = m_numLines - m_viewLines + 1;
        if (m_topLine < 1)
            m_topLine = 1;
    }

    if (key == 1 || key == 525)
        m_bLeaveItem = 1;

    int function = m_parOnEventAction[MENU_EVENT_CLOSE];
    if (function)
        return int(zCMenu::GetParser()->CallFunc(function));
    return 1;
}

void zCMenuItemText::SetText(const zSTRING& text, int index, int animated)
{
    m_topLine = 1;
    m_viewLines = -1;
    m_numLines = -1;
    m_unformated = 1;

    if (m_mode != MODE_ENUM) {
        zCMenuItem::SetText(text, index, animated);
        m_fullText = GetText(0);
        return;
    }

    int selection = text.ToInt();
    if (selection >= m_numOptions)
        selection = m_numOptions - 1;
    if (selection < 0)
        selection = 0;
    m_topLine = selection + 1;
    zCMenuItem::SetText(m_fullText.PickWord(selection + 1, "|", ""), index, animated);
}

void zCMenuItemText::SetText(zCArray<zSTRING> text, int animated)
{
    zCMenuItem::SetText(text, animated);
}

zCMenuItemSlider::zCMenuItemSlider(const zSTRING& name) : zCMenuItem(name)
{
    Setflags(512);
    slider = NULL;
    slideStep = 0;
    sliderDimX = 0;
    sliderPos = 0;
    sliderValue = 0.75f;
}

zCMenuItemSlider::~zCMenuItemSlider()
{
}

int zCMenuItemSlider::ToggleValue(int direction, int alternate)
{
    if ((m_parItemFlags & 0x400) && !(m_parItemFlags & 0x1000))
        zoptions->WriteInt("INTERNAL", "perfQualityIndex", 0, 0);
    zCMenuItem::ToggleValue(direction, alternate);

    if (!slider)
        return 0;

    float oldValue = sliderValue;
    sliderPos += direction;
    if (sliderPos > m_parUserFloat[0])
        sliderPos = int(m_parUserFloat[0]);
    if (sliderPos < 0)
        sliderPos = 0;

    int x1, y1, x2, y2;
    slider->GetExtends(x1, y1, x2, y2);
    slider->SetPos(slideStep * sliderPos - (x2 - x1) / 2 + 100, y1);
    sliderValue = sliderPos / m_parUserFloat[0];

    if (sliderValue != oldValue && !m_parOnChgSetOption.IsEmpty())
        zoptions->WriteReal(m_parOnChgSetOptionSection, m_parOnChgSetOption, sliderValue,
            (m_parItemFlags & 4096) != 0);
    return 1;
}

void zCMenuItemSlider::InitMenuItem()
{
    zCMenuItem::InitMenuItem();

    if (!m_parOnChgSetOption.IsEmpty()) {
        sliderValue = zoptions->ReadReal(m_parOnChgSetOptionSection, m_parOnChgSetOption, sliderValue);
        zoptions->WriteReal(m_parOnChgSetOptionSection, m_parOnChgSetOption, sliderValue,
            (m_parItemFlags & 4096) != 0);
    }

    slideStep = m_parUserFloat[0] < 1.0f ? 7991 : int(7991.0f / m_parUserFloat[0]);
    sliderPos = int(m_parUserFloat[0] * sliderValue);
}

void zCMenuItemSlider::InsertInWin(zCView* window)
{
    sliderValue = zoptions->ReadReal(m_parOnChgSetOptionSection, m_parOnChgSetOption, 0.0f);
    zoptions->WriteReal(m_parOnChgSetOptionSection, m_parOnChgSetOption, sliderValue,
        (m_parItemFlags & 4096) != 0);
    zCMenuItem::InsertInWin(window);

    if (!m_bVisible)
        return;

    sliderDimX = m_pInnerWindow->anx(16);
    int height = m_pInnerWindow->any(16);
    int y = (8192 - height) / 2;
    slider = new zCViewWindow(0, y, sliderDimX, y + height);
    m_pInnerWindow->InsertItem(slider);
    slider->ClrFlags(512);
    zoptions->ChangeDir(13);

    if (!m_parUserString[0].IsEmpty())
        slider->InsertBack(m_parUserString[0]);

    SetValue(sliderValue);
}

void zCMenuItemSlider::RemoveFromWin()
{
    if (slider) {
        if (m_pInnerWindow)
            m_pInnerWindow->RemoveItem(slider);
        delete slider;
        slider = NULL;
    }

    zCMenuItem::RemoveFromWin();
}

void zCMenuItemSlider::SetValue(float value)
{
    sliderValue = value;
    sliderPos = int(m_parUserFloat[0] * value);

    int x1, y1, x2, y2;
    slider->GetExtends(x1, y1, x2, y2);
    slider->SetPos(slideStep * sliderPos - (x2 - x1) / 2 + 100, y1);
}

float zCMenuItemSlider::GetValue()
{
    return sliderValue;
}

zCMenuItemChoice::zCMenuItemChoice(const zSTRING& name) : zCMenuItem(name)
{
    winCursor = NULL;
    option = 0;
    curStep = 0;
    optionStart = 0;
}

zCMenuItemChoice::~zCMenuItemChoice()
{
}

int zCMenuItemChoice::ToggleValue(int direction, int alternate)
{
    zCMenuItem::ToggleValue(direction, alternate);
    option += direction;

    if (!alternate) {
        if (option < optionStart)
            option = optionStart;
        if (option >= GetNumOptions())
            option = GetNumOptions() - 1;
    }
    else {
        if (option < optionStart)
            option = GetNumOptions() - 1;
        if (option >= GetNumOptions())
            option = optionStart;
    }

    zoptions->WriteInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, option,
        (m_parItemFlags & 4096) != 0);
    DrawFront();
    return 1;
}

int zCMenuItemChoice::SetOptionByString(zSTRING text)
{
    option = 0;

    for (int index = 1; ; index++) {
        zSTRING candidate = m_listLines.parray[0].PickWord(index, "|", "");
        if (candidate.IsEmpty())
            break;
        if (candidate == text) {
            option = index - 1;
            break;
        }
    }

    if (option < 0)
        option = GetNumOptions() - 1;
    if (option >= GetNumOptions())
        option = optionStart;
    return option;
}

zSTRING zCMenuItemChoice::GetStringByOption()
{
    return m_listLines.parray[0].PickWord(option + 1, "|#", "");
}

void zCMenuItemChoice::InsertInWin(zCView* window)
{
    option = zoptions->ReadInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, 0);
    if (option < 0)
        option = GetNumOptions() - 1;
    if (option >= GetNumOptions())
        option = optionStart;
    zoptions->WriteInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, option,
        (m_parItemFlags & 4096) != 0);
    zCMenuItem::InsertInWin(window);

    if (!m_bVisible)
        return;

    winCursor = new zCViewWindow(0, 0, 8191, m_pInnerWindow->FontY());
    m_pInnerWindow->InsertItem(winCursor);
    winCursor->ClrFlags(512);
    winCursor->SetTransparency(100);
    winCursor->SetAlphaBlendFunc(3);

    if (!m_parUserString[0].IsEmpty())
        winCursor->InsertBack(m_parUserString[0]);

    if (m_parOnEventAction[MENU_EVENT_INIT])
        zCMenu::GetParser()->CallFunc(m_parOnEventAction[MENU_EVENT_INIT]);

    winCursor->SetPos(0, m_pInnerWindow->FontY() * option);
    DrawFront();

    if (zCMenu::GetActive())
        zCMenu::GetActive()->Render();
}

void zCMenuItemChoice::RemoveFromWin()
{
    if (winCursor) {
        if (m_pInnerWindow)
            m_pInnerWindow->RemoveItem(winCursor);
        delete winCursor;
        winCursor = NULL;
    }

    zCMenuItem::RemoveFromWin();
}

void zCMenuItemChoice::Resize()
{
    UpdateSize();
}

void zCMenuItemChoice::Draw()
{
    if (m_bDontRender)
        return;

    DrawFront();
    DrawItems();
}

void zCMenuItemChoice::DrawFront()
{
    if (!owner || !m_pInnerWindow)
        return;

    zSTRING text = zCMenuItem::GetText(0).PickWord(option + 1, "#|", "");
    m_pInnerWindow->ClrPrintwin();
    UpdateFX();

    if (m_parItemFlags & 2048)
        m_pInnerWindow->Printwin(text + "\n");
    else if (m_parItemFlags & 16)
        m_pInnerWindow->PrintCXY(text);
    else
        m_pInnerWindow->Print(m_pInnerWindow->anx(20), 0, text);
}

int zCMenuItemChoice::GetNumOptions()
{
    zSTRING text = zCMenuItem::GetText(0);
    int count = 0;

    for (int index = 1; ; index++) {
        if (text.PickWord(index, "|", "").IsEmpty())
            break;
        count++;
    }

    return count;
}

zSTRING zCMenuItemChoice::GetText(int index)
{
    if (index < 1)
        return zCMenuItem::GetText(index);
    return zCMenuItem::GetText(0).PickWord(index, "#|", "");
}

void zCMenuItemChoice::InitMenuItem()
{
    zCMenuItem::InitMenuItem();

    if (!m_parOnChgSetOption.IsEmpty()) {
        optionStart = zCMenuItem::GetText(0).Search(0, "#", 1) < 0 ? 0 : 1;
        option = zoptions->ReadInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, 0);
    }
}

zCMenuItemButton::zCMenuItemButton(const zSTRING& name) : zCMenuItem(name)
{
    pic.Clear();
    active = 1;
}

zCMenuItemButton::~zCMenuItemButton()
{
    SetEnableHandleEvent(0);
}

void zCMenuItemButton::InitMenuItem()
{
    zCMenuItem::InitMenuItem();

    if (!m_parOnChgSetOption.IsEmpty()) {
        active = zoptions->ReadInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, 1);
        zoptions->WriteInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, active, 0);
    }

    pic = active ? m_parBackPic : m_parUserString[0];
}

void zCMenuItemButton::Toggle()
{
    active = !active;
    pic = active ? m_parBackPic : m_parUserString[0];
    zoptions->WriteInt(m_parOnChgSetOptionSection, m_parOnChgSetOption, active,
        (m_parItemFlags & 4096) != 0);
    zoptions->ChangeDir(13);
    InsertBack(pic);
    Draw();
}

int zCMenuItemButton::Run()
{
    Enter();
    Toggle();
    Leave();
    return 0;
}

void zCMenuItemButton::InsertInWin(zCView* window)
{
    zCMenuItem::InsertInWin(window);

    if (m_bVisible) {
        zoptions->ChangeDir(14);
        InsertBack(pic);
    }
}

void zCMenuItemButton::RemoveFromWin()
{
    zCMenuItem::RemoveFromWin();
}

zTMenuItemListElement::zTMenuItemListElement()
{
    contentViewer = NULL;
}

zCMenuItemList::zCMenuItemList(const zSTRING& name) : zCMenuItem(name)
{
    m_pInnerWindow2 = NULL;
    m_pViewArrowUp = NULL;
    m_pViewArrowDn = NULL;
    m_cur_element = -1;
    m_top_element = -1;
}

zCMenuItemList::~zCMenuItemList()
{
    m_pViewArrowUp = NULL;
    m_pViewArrowDn = NULL;
    m_pInnerWindow2 = NULL;
    m_list.DeleteList();
}

int zCMenuItemList::Run()
{
    if (m_list.numInArray > 0) {
        if (m_cur_element < 0)
            m_cur_element = 0;
        if (m_cur_element >= m_list.numInArray)
            m_cur_element = m_list.numInArray - 1;
        return zCMenuItem::Run();
    }

    return 0;
}

void zCMenuItemList::InitMenuItem()
{
}

int zCMenuItemList::Show()
{
    int result = zCMenuItem::Show();
    if (result)
        DrawFront();
    return result;
}

int zCMenuItemList::Hide()
{
    return zCMenuItem::Hide();
}

void zCMenuItemList::DeleteContent()
{
    m_list.DeleteList();
}

void zCMenuItemList::InsertEnd(zTMenuItemListElement& element)
{
    m_list.InsertEnd(element);
}

void zCMenuItemList::InsertFront(zTMenuItemListElement& element)
{
    if (m_list.numAlloc < m_list.numInArray + 1)
        m_list.AllocDelta(m_list.numAlloc >= 16 ? m_list.numAlloc / 2 : 16);

    for (int i = m_list.numInArray; i > 0; i--)
        m_list.parray[i] = m_list.parray[i - 1];
    m_list.parray[0] = element;
    m_list.numInArray++;
}

void zCMenuItemList::InsertInWin(zCView* window)
{
    zCMenuItem::InsertInWin(window);

    if (!m_bVisible)
        return;

    int width = anx(16);
    int height = any(16);

    if (!m_pViewArrowUp)
        m_pViewArrowUp = new zCView(8192 - width, m_parFrameSizeY, 8192, m_parFrameSizeY + height);
    if (!m_pViewArrowDn)
        m_pViewArrowDn = new zCView(8192 - width, 8192 - m_parFrameSizeY - height,
            8192, 8192 - m_parFrameSizeY);

    m_pViewArrowUp->InsertBack("MENU_ARROW_UP.TGA");
    m_pViewArrowDn->InsertBack("MENU_ARROW_DOWN.TGA");
    InsertItem(m_pViewArrowUp);
    InsertItem(m_pViewArrowDn);
}

void zCMenuItemList::RemoveFromWin()
{
    delete m_pViewArrowUp;
    m_pViewArrowUp = NULL;
    delete m_pViewArrowDn;
    m_pViewArrowDn = NULL;
    delete m_pInnerWindow2;
    m_pInnerWindow2 = NULL;
    zCMenuItem::RemoveFromWin();
}

void zCMenuItemList::Enter()
{
}

void zCMenuItemList::Leave()
{
}

void zCMenuItemList::UpdateContent()
{
}

void zCMenuItemList::DrawFront()
{
    if (!owner || !m_pInnerWindow)
        return;

    m_pInnerWindow->ClrPrintwin();
    if (m_top_element < 0)
        m_top_element = 0;
    if (m_cur_element < 0)
        m_cur_element = 0;

    int fontY = m_pInnerWindow->FontY();
    int visibleLines = (8192 - fontY) / fontY;
    if (m_top_element > m_cur_element)
        m_top_element = m_cur_element;
    if (m_top_element + visibleLines < m_cur_element)
        m_top_element = m_cur_element - visibleLines;

    int y = 0;
    int index = m_top_element;
    while (index < m_list.numInArray && y < 8192) {
        m_pInnerWindow->SetFont(active && index == m_cur_element ? m_pFontHi : m_pFont);
        m_pInnerWindow->Print(0, y, m_list.parray[index].title);
        y += fontY;
        index++;
    }

    if (m_pViewArrowUp) {
        if (m_top_element)
            InsertItem(m_pViewArrowUp);
        else
            RemoveItem(m_pViewArrowUp);
    }
    if (m_pViewArrowDn) {
        if (index < m_list.numInArray)
            InsertItem(m_pViewArrowDn);
        else
            RemoveItem(m_pViewArrowDn);
    }
}

int zCMenuItemList::HandleEvent(int key)
{
    int logical = zinput->GetFirstBindedLogicalKey(key);
    if (logical == 3)
        key = 200;
    else if (logical == 4)
        key = 208;

    switch (key) {
    case 28:
    case 524:
        ShowContent();
        break;
    case 199:
        JumpToTopEntry();
        break;
    case 200:
        JumpToPrevEntry();
        break;
    case 207:
        JumpToLastEntry();
        break;
    case 208:
        JumpToNextEntry();
        break;
    case 1:
    case 525:
        m_bLeaveItem = 1;
        break;
    default:
        return zCMenuItem::HandleEvent(key);
    }

    return 1;
}

void zCMenuItemList::ShowContent()
{
    if (m_cur_element < 0)
        m_cur_element = 0;
    if (m_cur_element >= m_list.numInArray)
        m_cur_element = m_list.numInArray - 1;

    zTMenuItemListElement& element = m_list.parray[m_cur_element];
    if (element.contentViewer) {
        element.contentViewer->SetText(element.content, 0, 0);
        element.contentViewer->Show();
        element.contentViewer->Draw();
        element.contentViewer->UpdateContent();
    }
}

void zCMenuItemList::JumpToNextEntry()
{
    if (m_cur_element + 1 < m_list.numInArray) {
        m_cur_element++;
        DrawFront();
    }
}

void zCMenuItemList::JumpToPrevEntry()
{
    if (m_cur_element > 0) {
        m_cur_element--;
        DrawFront();
    }
}

void zCMenuItemList::JumpToTopEntry()
{
    m_cur_element = 0;
    m_top_element = 0;
    DrawFront();
}

void zCMenuItemList::JumpToLastEntry()
{
    m_cur_element = m_list.numInArray - 1;
    m_top_element = m_cur_element;
    DrawFront();
}
