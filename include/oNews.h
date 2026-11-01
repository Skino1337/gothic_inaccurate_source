#ifndef ONEWS_H
#define ONEWS_H

#include "zModel.h"

class oCNpc;

enum oTNewsSpreadType {
    NEWS_DONT_SPREAD,
    NEWS_SPREAD_NPC_FRIENDLY_TOWARDS_VICTIM,
    NEWS_SPREAD_NPC_FRIENDLY_TOWARDS_WITNESS,
    NEWS_SPREAD_NPC_FRIENDLY_TOWARDS_OFFENDER,
    NEWS_SPREAD_NPC_SAME_GUILD_VICTIM
};

class oCNews {
public:

    oCNews();
    oCNews(int id, int spreadType, oCNpc* witness, oCNpc* offender, oCNpc* victim, int gossip);
    ~oCNews();

    static int  GetRemainingNews();
    static void CheckRemainingNews();

    int   GetID();
    int   IsIdentical(int id, oCNpc* offender, oCNpc* victim);
    void  SetSpreadTime(float time);
    float GetSpreadTime();
    void  SetGossip(int value);
    int   IsGossip();
    int   IsGuildVictim();
    void  GetNewsData(int& id, int& spreadType, oCNpc*& witness, oCNpc*& offender,
        oCNpc*& victim);

    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    int              told;
    float            spread_time;
    oTNewsSpreadType spreadType;
    int              news_id;
    int              gossip;
    oCNpc*           mNpcWitness;
    oCNpc*           mNpcOffender;
    oCNpc*           mNpcVictim;
    zSTRING          witnessName;
    zSTRING          offenderName;
    zSTRING          victimName;
    int              guildvictim;

    static int news_counter;
    static zCArray<oCNews*> newsList;
};

class oCNewsMemory {
public:

    oCNewsMemory();
    ~oCNewsMemory();

    void    Insert(oCNews* news);
    void    Remove(oCNews* news);
    oCNews* GetNews(int id, oCNpc* offender, oCNpc* victim);
    void    CheckSpreadNews();
    int     SearchNews(int id, oCNpc* offender, oCNpc* victim);
    void    ShowDebugInfos();
    oCNews* GetNewsByNumber(int number);
    void    ClearNews();
    int     DeleteNewsByNumber(int number);

    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCList<oCNews> iknow;
};

class oCNewsManager {
public:

    enum { MAX_ENTRY = 20 };

    oCNewsManager();
    ~oCNewsManager();

    void CreateNews(int id, int spreadType, oCNpc* witness, oCNpc* offender, oCNpc* victim,
        int gossip);
    void SpreadToGuild(int guild);
    int  IsInSpreadList(int guild);
    void SpreadNews(oCNews* news);

    int called_BAssessAndMem;
    int sentry;
    int spreadList[MAX_ENTRY];
};

#endif
