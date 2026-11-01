#include "include/oNews.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/oGame.h"
#include "include/zTools.h"
#include "include/oHelp.h"

// EXE: 0x00AB1B84
int oCNews::news_counter;
// EXE: 0x00AB1B14
zCArray<oCNews*> oCNews::newsList;

oCNewsManager::oCNewsManager()
    : called_BAssessAndMem(0), sentry(0)
{
    for (int i = 0; i < MAX_ENTRY; ++i)
        spreadList[i] = 0;
}

oCNewsManager::~oCNewsManager()
{
}

void oCNewsManager::CreateNews(int id, int spreadType, oCNpc* witness, oCNpc* offender,
    oCNpc* victim, int gossip)
{
    if (!witness)
        return;

    for (zCList<oCNews>* entry = witness->knownNews.iknow.next; entry; entry = entry->next)
    {
        oCNews* news = entry->data;
        if (news->news_id == id && news->mNpcOffender == offender && news->mNpcVictim == victim)
        {
            if (gossip)
            {
                news->gossip = 1;
                news->spread_time = 0.0f;
            }
            return;
        }
    }

    if (witness->IsAPlayer())
        return;

    oCNews* news = new oCNews(id, spreadType, witness, offender, victim, gossip);
    news->spread_time = 0.0f;
    if (witness->knownNews.iknow.GetNumInList() >= 200)
        witness->knownNews.Remove(witness->knownNews.iknow.Get(199));
    witness->knownNews.Insert(news);

    if (!spreadType)
    {
        int function = parser.GetIndex("C_CANNEWSBESPREAD");
        if (function > 0)
        {
            parser.SetInstance("SELF", witness);
            parser.SetInstance("OTHER", offender);
            parser.SetInstance("VICTIM", victim);
            int allowed = parser.CallFunc(function);
            if (allowed)
            {
                news->spread_time = 60000.0f;
                news->spreadType = static_cast<oTNewsSpreadType>(allowed);
            }
        }
    }
}

void oCNewsManager::SpreadToGuild(int guild)
{
    if (sentry < MAX_ENTRY)
        spreadList[sentry++] = guild;
}

int oCNewsManager::IsInSpreadList(int guild)
{
    for (int i = 0; i < sentry; ++i)
    {
        if (spreadList[i] == guild)
            return 1;
    }
    return 0;
}

void oCNewsManager::SpreadNews(oCNews* news)
{
    if (news->gossip)
        return;

    int function = parser.GetIndex("B_SPREADANDMEMORIZE");
    if (function < 0)
        return;

    int id;
    int spreadType;
    oCNpc* witness;
    oCNpc* offender;
    oCNpc* victim;
    news->GetNewsData(id, spreadType, witness, offender, victim);
    spreadType = news->spreadType;

    for (zCList<zCVob>* entry = ogame->GetWorld()->voblist_npcs->next; entry; entry = entry->next)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(entry->data);
        if (!npc || npc->IsDead() || npc == witness || npc->IsAPlayer())
            continue;

        int spread = 0;
        switch (spreadType)
        {
        case NEWS_SPREAD_NPC_FRIENDLY_TOWARDS_VICTIM:
            spread = victim && oCNpc::IsGuildFriendly(victim->GetTrueGuild(), npc->GetTrueGuild());
            break;
        case NEWS_SPREAD_NPC_FRIENDLY_TOWARDS_WITNESS:
            spread = witness && oCNpc::IsGuildFriendly(witness->GetTrueGuild(), npc->GetTrueGuild());
            break;
        case NEWS_SPREAD_NPC_FRIENDLY_TOWARDS_OFFENDER:
            spread = offender && oCNpc::IsGuildFriendly(offender->GetTrueGuild(), npc->GetTrueGuild());
            break;
        case NEWS_SPREAD_NPC_SAME_GUILD_VICTIM:
            spread = victim && victim->GetTrueGuild() == npc->GetTrueGuild();
            break;
        }

        if (spread)
        {
            parser.SetInstance("SELF", npc);
            parser.SetInstance("OTHER", offender);
            parser.SetInstance("VICTIM", victim);
            parser.CallFunc(function);
        }
    }
}

oCNewsMemory::oCNewsMemory()
{
    iknow.data = 0;
    iknow.next = 0;
}

oCNewsMemory::~oCNewsMemory()
{
    ClearNews();
}

void oCNewsMemory::Insert(oCNews* news)
{
    if (!news)
        return;
    if (iknow.GetNumInList() >= 200)
        Remove(iknow.Get(199));
    iknow.Insert(news);
}

void oCNewsMemory::Remove(oCNews* news)
{
    if (!news)
        return;

    zCList<oCNews>* previous = &iknow;
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
    {
        if (entry->data == news)
        {
            previous->next = entry->next;
            entry->next = 0;
            delete entry;
            break;
        }
        previous = entry;
    }
    delete news;
}

oCNews* oCNewsMemory::GetNews(int id, oCNpc* offender, oCNpc* victim)
{
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
    {
        oCNews* news = entry->data;
        if (news->news_id == id && news->mNpcOffender == offender && news->mNpcVictim == victim)
            return news;
    }
    return 0;
}

void oCNewsMemory::CheckSpreadNews()
{
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
    {
        oCNews* news = entry->data;
        if (news->spread_time <= 0.0f)
            continue;

        news->spread_time -= ztimer.frameTimeFloat;
        if (news->spread_time < 0.0f)
            news->spread_time = 0.0f;
        if (news->spread_time == 0.0f)
            ogame->GetNewsManager()->SpreadNews(news);
    }
}

int oCNewsMemory::SearchNews(int id, oCNpc* offender, oCNpc* victim)
{
    int result = 0;
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
    {
        ++result;
        oCNews* news = entry->data;
        if (news->news_id == id && (!offender || offender == news->mNpcOffender)
            && (!victim || victim == news->mNpcVictim))
            return result;
    }
    return 0;
}

void oCNewsMemory::ShowDebugInfos()
{
    screen->Print(0, 0, "News");
    screen->Print(500, 0, "Witness");
    screen->Print(2000, 0, "Offender");
    screen->Print(3500, 0, "Victim");
    screen->Print(5000, 0, "Type");
    screen->Print(7500, 0, "Time");

    int y = screen->FontY() * 2;
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
    {
        oCNews* news = entry->data;
        screen->Print(0, y, zSTRING(news->news_id));
        if (news->mNpcWitness)
            screen->Print(500, y, news->mNpcWitness->GetName());
        if (news->mNpcOffender)
            screen->Print(2000, y, news->mNpcOffender->GetName());
        if (news->mNpcVictim)
            screen->Print(3500, y, news->mNpcVictim->GetName());
        screen->Print(5000, y, zSTRING(news->spreadType));
        screen->Print(7500, y, zSTRING(static_cast<long>(news->spread_time)));
        y += screen->FontY();
    }
}

oCNews* oCNewsMemory::GetNewsByNumber(int number)
{
    int current = 1;
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
    {
        if (current == number)
            return entry->data;
        ++current;
    }
    return 0;
}

void oCNewsMemory::ClearNews()
{
    while (iknow.next)
        Remove(iknow.next->data);
}

int oCNewsMemory::DeleteNewsByNumber(int number)
{
    oCNews* news = GetNewsByNumber(number);
    if (!news)
        return 0;
    Remove(news);
    return 1;
}

void oCNewsMemory::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;

    archiver.WriteInt(iknow.GetNumInList());
    for (zCList<oCNews>* entry = iknow.next; entry; entry = entry->next)
        entry->data->Archive(archiver);
}

void oCNewsMemory::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;

    ClearNews();
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
    {
        oCNews* news = new oCNews;
        news->Unarchive(archiver);
        iknow.Insert(news);
    }
}

int oCNews::GetRemainingNews()
{
    return news_counter;
}

void oCNews::CheckRemainingNews()
{
    for (int i = 0; i < newsList.numInArray; ++i)
    {
        oCNews* news = newsList[i];
        if (news)
            screen->Print(0, i * screen->FontY(),
                zSTRING("U: NEWS: Found illegal news : ") + zSTRING(news->news_id));
    }
}

oCNews::oCNews()
    : told(0), spread_time(0.0f), spreadType(NEWS_DONT_SPREAD), news_id(-1), gossip(0),
      mNpcWitness(0), mNpcOffender(0), mNpcVictim(0), guildvictim(0)
{
    ++news_counter;
    newsList.InsertEnd(this);
}

oCNews::oCNews(int id, int spreadType, oCNpc* witness, oCNpc* offender, oCNpc* victim, int gossip)
    : told(0), spread_time(0.0f), spreadType(NEWS_DONT_SPREAD), news_id(id), gossip(gossip),
      mNpcWitness(witness), mNpcOffender(offender), mNpcVictim(victim),
      guildvictim(victim ? victim->GetTrueGuild() : 0)
{
    ++news_counter;
    if (mNpcWitness)
        mNpcWitness->AddRef();
    if (mNpcOffender)
        mNpcOffender->AddRef();
    if (mNpcVictim)
        mNpcVictim->AddRef();
    newsList.InsertEnd(this);
}

oCNews::~oCNews()
{
    newsList.RemoveOrder(this);
    if (mNpcWitness)
        mNpcWitness->Release();
    if (mNpcOffender)
        mNpcOffender->Release();
    if (mNpcVictim)
        mNpcVictim->Release();
    --news_counter;
}

int oCNews::GetID()
{
    return news_id;
}

int oCNews::IsIdentical(int id, oCNpc* offender, oCNpc* victim)
{
    return news_id == id && mNpcOffender == offender && mNpcVictim == victim;
}

void oCNews::SetSpreadTime(float time)
{
    spread_time = time < 0.0f ? 0.0f : time;
}

float oCNews::GetSpreadTime()
{
    return spread_time;
}

void oCNews::SetGossip(int value)
{
    gossip = value;
    if (value)
        spread_time = 0.0f;
}

int oCNews::IsGossip()
{
    return gossip;
}

int oCNews::IsGuildVictim()
{
    return guildvictim;
}

void oCNews::GetNewsData(int& id, int& spreadType, oCNpc*& witness, oCNpc*& offender,
    oCNpc*& victim)
{
    id = news_id;
    spreadType = gossip;
    if (!mNpcWitness && !witnessName.IsEmpty())
    {
        mNpcWitness = dynamic_cast<oCNpc*>(ogame->GetWorld()->SearchVobByName(witnessName));
        if (mNpcWitness)
            mNpcWitness->AddRef();
    }
    if (!mNpcOffender && !offenderName.IsEmpty())
    {
        mNpcOffender = dynamic_cast<oCNpc*>(ogame->GetWorld()->SearchVobByName(offenderName));
        if (mNpcOffender)
            mNpcOffender->AddRef();
    }
    if (!mNpcVictim && !victimName.IsEmpty())
    {
        mNpcVictim = dynamic_cast<oCNpc*>(ogame->GetWorld()->SearchVobByName(victimName));
        if (mNpcVictim)
            mNpcVictim->AddRef();
    }
    witness = mNpcWitness;
    offender = mNpcOffender;
    victim = mNpcVictim;
}

void oCNews::Archive(zCArchiver& archiver)
{
    archiver.WriteBool(told);
    archiver.WriteFloat(spread_time);
    archiver.WriteInt(spreadType);
    archiver.WriteInt(news_id);
    archiver.WriteBool(gossip);
    archiver.WriteBool(guildvictim);
    if (mNpcWitness)
        witnessName = mNpcWitness->GetInstanceName();
    archiver.WriteString(witnessName);
    if (mNpcOffender)
        offenderName = mNpcOffender->GetInstanceName();
    archiver.WriteString(offenderName);
    if (mNpcVictim)
        victimName = mNpcVictim->GetInstanceName();
    archiver.WriteString(victimName);
}

void oCNews::Unarchive(zCArchiver& archiver)
{
    told = archiver.ReadBool();
    spread_time = archiver.ReadFloat();
    spreadType = static_cast<oTNewsSpreadType>(archiver.ReadInt());
    news_id = archiver.ReadInt();
    gossip = archiver.ReadBool();
    guildvictim = archiver.ReadBool();
    if (mNpcWitness)
        mNpcWitness->Release();
    if (mNpcOffender)
        mNpcOffender->Release();
    if (mNpcVictim)
        mNpcVictim->Release();
    mNpcWitness = 0;
    mNpcOffender = 0;
    mNpcVictim = 0;
    witnessName = archiver.ReadString();
    offenderName = archiver.ReadString();
    victimName = archiver.ReadString();
}
