#ifndef OLOGMANAGER_H
#define OLOGMANAGER_H

#include "zModel.h"
#include "zString.h"

class oCLogEntry {
public:

    oCLogEntry();
    ~oCLogEntry();
    void Archive(zCArchiver& archiver);
    void Unarchive(zCArchiver& archiver);

    zSTRING m_strDescription;
};

class oCLogTopic {
public:

    enum zELogTopicStatus {
        zELogTopicStatus_Free,
        zELogTopicStatus_Running,
        zELogTopicStatus_Success,
        zELogTopicStatus_Failure,
        zELogTopicStatus_Obsolete
    };

    enum zELogSection {
        zELogSection_Missions,
        zELogSection_Notes,
        zELogSection_All
    };

    oCLogTopic();
    ~oCLogTopic();
    void AddEntry(const zSTRING& description);
    void Archive(zCArchiver& archiver);
    void Unarchive(zCArchiver& archiver);

    zSTRING m_strDescription;
    zELogSection m_enuSection;
    zELogTopicStatus m_enuStatus;
    zCList<oCLogEntry> m_lstEntries;

    static char** ms_strSections;
    static char** ms_strStati;
};

class oCLogManager {
public:

    oCLogManager& operator=(const oCLogManager&);
    oCLogManager(const oCLogManager&);
    oCLogManager();
    ~oCLogManager();
    void Clear();
    void Archive(zCArchiver& archiver);
    void Unarchive(zCArchiver& archiver);
    static oCLogManager& GetLogManager();

    zCList<oCLogTopic> m_lstTopics;
};

#endif
