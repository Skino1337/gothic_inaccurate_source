#include "include/oLogManager.h"

char** oCLogTopic::ms_strSections;
char** oCLogTopic::ms_strStati;

oCLogEntry::oCLogEntry()
{
}

oCLogEntry::~oCLogEntry()
{
}

void oCLogEntry::Archive(zCArchiver& archiver)
{
    archiver.WriteString(m_strDescription);
}

void oCLogEntry::Unarchive(zCArchiver& archiver)
{
    m_strDescription = archiver.ReadString();
}

oCLogTopic::oCLogTopic()
    : m_enuSection(zELogSection_Missions), m_enuStatus(zELogTopicStatus_Free)
{
    m_lstEntries.data = 0;
    m_lstEntries.next = 0;
}

oCLogTopic::~oCLogTopic()
{
    m_lstEntries.DeleteListDatas();
}

void oCLogTopic::AddEntry(const zSTRING& description)
{
    zCList<oCLogEntry>* last = &m_lstEntries;
    while (last) {
        if (last->data && last->data->m_strDescription == description)
            return;
        if (!last->next)
            break;
        last = last->next;
    }
    oCLogEntry* entry = new oCLogEntry;
    entry->m_strDescription = description;
    last->next = new zCList<oCLogEntry>;
    last->next->data = entry;
    last->next->next = 0;
}

void oCLogTopic::Archive(zCArchiver& archiver)
{
    archiver.WriteString(m_strDescription);
    archiver.WriteEnum(m_enuSection);
    archiver.WriteEnum(m_enuStatus);
    archiver.WriteInt(m_lstEntries.GetNumInList());
    for (int i = 0; i < m_lstEntries.GetNumInList(); ++i)
        m_lstEntries.Get(i)->Archive(archiver);
}

void oCLogTopic::Unarchive(zCArchiver& archiver)
{
    m_strDescription = archiver.ReadString();
    m_enuSection = static_cast<zELogSection>(archiver.ReadEnum());
    m_enuStatus = static_cast<zELogTopicStatus>(archiver.ReadEnum());
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i) {
        oCLogEntry* entry = new oCLogEntry;
        entry->Unarchive(archiver);
        m_lstEntries.Insert(entry);
    }
}

oCLogManager& oCLogManager::operator=(const oCLogManager&)
{
    return *this;
}

oCLogManager::oCLogManager(const oCLogManager&)
{
    m_lstTopics.data = 0;
    m_lstTopics.next = 0;
}

oCLogManager::oCLogManager()
{
    m_lstTopics.data = 0;
    m_lstTopics.next = 0;
}

oCLogManager::~oCLogManager()
{
    Clear();
}

oCLogManager& oCLogManager::GetLogManager()
{
    static oCLogManager manager;
    return manager;
}

void oCLogManager::Clear()
{
    m_lstTopics.DeleteListDatas();
}

void oCLogManager::Archive(zCArchiver& archiver)
{
    archiver.WriteInt(m_lstTopics.GetNumInList());
    for (int i = 0; i < m_lstTopics.GetNumInList(); ++i)
        m_lstTopics.Get(i)->Archive(archiver);
}

void oCLogManager::Unarchive(zCArchiver& archiver)
{
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i) {
        oCLogTopic* topic = new oCLogTopic;
        topic->Unarchive(archiver);
        m_lstTopics.Insert(topic);
    }
}
