#include "include/oMenu_Status.h"
#include "include/zMenuItem.h"

oCMenu_Log::oCMenu_Log(const zSTRING& name)
    : zCMenu(name), listItemAct(NULL), listItemOld(NULL), listItemFailed(NULL), listItemLog(NULL),
      contentViewer(NULL)
{
}

oCMenu_Log::~oCMenu_Log()
{
}

void oCMenu_Log::ScreenInit()
{
    zCMenu::ScreenInit();
    listItemAct = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_MISSIONS_ACT"));
    listItemOld = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_MISSIONS_OLD"));
    listItemFailed = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_MISSIONS_FAILED"));
    listItemLog = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName("MENU_ITEM_LIST_LOG"));
    contentViewer = dynamic_cast<zCMenuItemText*>(zCMenuItem::GetByName("MENU_ITEM_CONTENT_VIEWER"));
}

void oCMenu_Log::ScreenDone()
{
    if (listItemAct)
        listItemAct->Release();
    if (listItemFailed)
        listItemFailed->Release();
    if (listItemOld)
        listItemOld->Release();
    if (listItemLog)
        listItemLog->Release();
    if (contentViewer)
        contentViewer->Release();
    listItemAct = NULL;
    listItemFailed = NULL;
    listItemOld = NULL;
    listItemLog = NULL;
    contentViewer = NULL;
    zCMenu::ScreenDone();
}

void oCMenu_Log::SetDayTime(const zSTRING& day, const zSTRING& time)
{
    zCMenuItem* dayItem = zCMenuItem::GetByName("MENU_ITEM_DAY");
    if (dayItem)
        dayItem->SetText(day, 0, 0);

    zCMenuItem* timeItem = zCMenuItem::GetByName("MENU_ITEM_TIME");
    if (timeItem)
        timeItem->SetText(time, 0, 0);
}

static zSTRING LogTopicText(const oCLogTopic& topic)
{
    zSTRING text;
    for (zCList<oCLogEntry>* entry = topic.m_lstEntries.next; entry; entry = entry->next)
    {
        if (!text.IsEmpty())
            text += "\n---\n";
        text += entry->data->m_strDescription;
    }
    return text;
}

void oCMenu_Log::SetLogTopics(const zCList<oCLogTopic>& topics)
{
    zCMenuItemList* active = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_MISSIONS_ACT"));
    zCMenuItemList* old = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_MISSIONS_OLD"));
    zCMenuItemList* failed = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_MISSIONS_FAILED"));
    zCMenuItemList* notes = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(
        "MENU_ITEM_LIST_LOG"));
    zCMenuItemText* viewer = dynamic_cast<zCMenuItemText*>(zCMenuItem::GetByName(
        "MENU_ITEM_CONTENT_VIEWER"));
    active->DeleteContent();
    old->DeleteContent();
    failed->DeleteContent();
    notes->DeleteContent();

    for (const zCList<oCLogTopic>* topicNode = topics.next; topicNode;
        topicNode = topicNode->next)
    {
        const oCLogTopic* topic = topicNode->data;
        zTMenuItemListElement element;
        element.title = topic->m_strDescription;
        element.content = LogTopicText(*topic);
        element.contentViewer = viewer;

        if (topic->m_enuSection == oCLogTopic::zELogSection_Notes)
            notes->InsertEnd(element);
        else if (topic->m_enuStatus == oCLogTopic::zELogTopicStatus_Running)
            active->InsertEnd(element);
        else if (topic->m_enuStatus == oCLogTopic::zELogTopicStatus_Failure)
            failed->InsertEnd(element);
        else
            old->InsertEnd(element);
    }

    active->Release();
    old->Release();
    failed->Release();
    notes->Release();
    viewer->Release();
}

void oCMenu_Log::SetInfo(const zCMenuItem*)
{
}

int oCMenu_Log::ExecCommand(const zSTRING& command)
{
    if (command.Copied(0, 7) != "EFFECTS")
        return 0;

    zSTRING effects = command.Copied(7, command.Length() - 7);
    effects.TrimLeft(' ');
    effects.TrimRight(' ');
    if (listItemLog)
        listItemLog->SetText(effects, 0, 0);
    return 1;
}

int oCMenu_Log::HandleEnterItem(zCMenuItem* item)
{
    int result = zCMenu::HandleEnterItem(item);
    zSTRING action = item->m_parOnSelAction_S[0];
    if (action.PickWord(1, " ", "\"") == "EFFECTS")
    {
        zSTRING targetName = action.Copied(7, action.Length() - 7);
        targetName.TrimLeft(' ');
        targetName.TrimRight(' ');
        zCMenuItemList* target = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(targetName));
        if (target)
        {
            target->Enter();
            target->Release();
        }
    }
    return result;
}

int oCMenu_Log::HandleLeaveItem(zCMenuItem* item)
{
    zSTRING action = item->m_parOnSelAction_S[0];
    if (action.PickWord(1, " ", "\"") == "EFFECTS")
    {
        zSTRING targetName = action.Copied(7, action.Length() - 7);
        targetName.TrimLeft(' ');
        targetName.TrimRight(' ');
        zCMenuItemList* target = dynamic_cast<zCMenuItemList*>(zCMenuItem::GetByName(targetName));
        if (target)
        {
            target->Leave();
            target->Release();
        }
    }
    return zCMenu::HandleLeaveItem(item);
}

int oCMenu_Log::HandleActivateItem(zCMenuItem*)
{
    return zCMenu::HandleEvent(1);
}

int oCMenu_Log::HandleEvent(int key)
{
    if (zinput->GetFirstBindedLogicalKey(key) == 0x15)
    {
        zCMenu::HandleEvent(1);
        return 1;
    }
    if (key == 0xcb || key == 0xcd)
        key = 0x1c;
    zCMenu::HandleEvent(key);
    return 1;
}

void oCMenu_Log::Activate()
{
    zCMenu::Activate();
}

void oCMenu_Log::InitForDisplay()
{
}

#include "include/zMoving.h"

static zSTRING statusNameInput;

int oCMenu_Status::HandleEvent(int key)
{
    unsigned short logicalKey = zinput->GetFirstBindedLogicalKey(key);
    if (logicalKey != 0x14 && key != 1 && logicalKey != 0x0e)
    {
        int character = GetCharFromDIK(key, 0, 0);
        if (character >= 0x20 && character <= 0xff)
        {
            statusNameInput += zSTRING((char)character);
            SetPlayerName(statusNameInput);
        }
    }

    zCMenu::HandleEvent(key);
    return 1;
}

void oCMenu_Status::SetPlayerName(const zSTRING& name)
{
    zCMenuItem* item = zCMenuItem::GetByName("MENU_ITEM_PLAYERNAME");
    if (item)
        item->SetText(name, 0, 0);
}

void oCMenu_Status::SetGuild(const zSTRING& guild)
{
    zCMenuItem* item = zCMenuItem::GetByName("MENU_ITEM_PLAYERGUILD");
    if (item)
        item->SetText(guild.IsEmpty() ? item->GetText(1) : guild, 0, 0);
}

void oCMenu_Status::SetMagicalGuild(const zSTRING& guild)
{
    zCMenuItem* item = zCMenuItem::GetByName("MENU_ITEM_PLAYER_MAGICALGUILD");
    if (item)
        item->SetText(guild.IsEmpty() ? item->GetText(1) : guild, 0, 0);
}

void oCMenu_Status::SetExperience(unsigned long experience, unsigned long level,
    unsigned long nextLevel)
{
    zCMenuItem* experienceItem = zCMenuItem::GetByName("MENU_ITEM_EXP");
    if (experienceItem)
    {
        experienceItem->SetText(zSTRING((int)experience), 0, 0);
        experienceItem->Release();
    }

    zCMenuItem* levelItem = zCMenuItem::GetByName("MENU_ITEM_LEVEL");
    if (levelItem)
    {
        levelItem->SetText(zSTRING((int)level), 0, 0);
        levelItem->Release();
    }

    zCMenuItem* nextItem = zCMenuItem::GetByName("MENU_ITEM_LEVEL_NEXT");
    if (nextItem)
    {
        nextItem->SetText(zSTRING((int)nextLevel), 0, 0);
        nextItem->Release();
    }
}

void oCMenu_Status::SetLearnPoints(unsigned long points)
{
    zCMenuItem* item = zCMenuItem::GetByName("MENU_ITEM_LEARN");
    if (item)
        item->SetText(zSTRING((int)points), 0, 0);
}

void oCMenu_Status::AddArmor(const oSMenuInfoArmor& armor)
{
    m_listArmory.Insert(armor);
}

void oCMenu_Status::AddAttribute(const oSMenuInfoAttribute& attribute)
{
    m_listAttributes.Insert(attribute);
}

void oCMenu_Status::AddTalent(const oSMenuInfoTalent& talent)
{
    m_listTalents.Insert(talent);
}

void oCMenu_Status::InitForDisplay()
{
    for (int i = 0; i < m_listAttributes.numInArray; ++i)
    {
        zSTRING base = "MENU_ITEM_ATTRIBUTE_" + zSTRING(i + 1);
        zCMenuItem* title = zCMenuItem::GetByName(base + "_TITLE");
        if (title)
        {
            title->SetText(m_listAttributes[i].name, 0, 0);
            title->Release();
        }
        zCMenuItem* value = zCMenuItem::GetByName(base);
        if (value)
        {
            value->SetText(zSTRING(m_listAttributes[i].actValue) + "/" +
                zSTRING(m_listAttributes[i].maxValue), 0, 0);
            value->Release();
        }
    }

    for (int i = 0; i < m_listArmory.numInArray; ++i)
    {
        zCMenuItem* item = zCMenuItem::GetByName("MENU_ITEM_ARMOR_" + zSTRING(i + 1));
        if (item)
        {
            item->SetText(zSTRING(m_listArmory[i].actValue), 0, 0);
            item->Release();
        }
    }

    for (int i = 0; i < m_listTalents.numInArray; ++i)
    {
        const oSMenuInfoTalent& talent = m_listTalents[i];
        if (talent.value <= 0 && talent.skillEnum == "|")
            continue;
        zSTRING base = "MENU_ITEM_TALENT_" + zSTRING(i + 1);
        zCMenuItem* title = zCMenuItem::GetByName(base + "_TITLE");
        if (title)
        {
            title->SetText(talent.name, 0, 0);
            title->Release();
        }
        zCMenuItem* skill = zCMenuItem::GetByName(base + "_SKILL");
        if (skill)
        {
            skill->SetText(talent.skillEnum, 0, 0);
            skill->SetText(talent.value < 0 ? "" : zSTRING(talent.skill), 0, 0);
            skill->Release();
        }
        if (talent.value >= 0)
        {
            zCMenuItem* value = zCMenuItem::GetByName(base);
            if (value)
            {
                value->SetText(zSTRING(talent.value) + "%", 0, 0);
                value->Release();
            }
        }
    }
}
