#include "include/oText.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"

static int oCTextAttributes[6];

zCView* oCText::output = NULL;
int oCText::TXT_GUILD = 0;
int oCText::TXT_ATTRIBUTE = 0;
int oCText::TXT_FIGHT_TALENT = 0;
int oCText::TXT_TALENT = 0;
int oCText::TXT_DAYTIME = 0;
int* oCText::TXT_ATR = oCTextAttributes;
int oCText::TXT_ATR_HP = 0;
int oCText::TXT_SPELL = 0;
int oCText::TXT_ATTITUDE = 0;
int oCText::TXT_PC_GREETING = 0;
int oCText::TXT_PC_FIRST_PHRASE = 0;
int oCText::TXT_PC_SECOND_PHRASE = 0;
int oCText::TXT_PC_TRADE_PHRASE = 0;
int oCText::TXT_NPC_LAST_PHRASE = 0;
int oCText::TXT_PC_TRADE_THREAT = 0;
int oCText::TXT_NPC_TRADE_THREAT = 0;
int oCText::TXT_PC_INFO_THREAT = 0;
int oCText::TXT_NPC_INFO_THREAT = 0;
int oCText::TXT_NEWS_INTRO = 0;
int oCText::TXT_NEWS_STOLE = 0;
int oCText::TXT_NEWS_STOLE_10 = 0;
int oCText::TXT_NEWS_STOLE_30 = 0;
int oCText::TXT_NEWS_STOLE_50 = 0;
int oCText::TXT_NEWS_STOLE_70 = 0;
int oCText::TXT_NEWS_STOLE_90 = 0;
int oCText::TXT_NEWS_STOLE_100 = 0;
int oCText::TXT_NEWS_ATTACKED = 0;
int oCText::TXT_NEWS_KILLED = 0;
int oCText::TXT_YNEWS_INTRO = 0;
int oCText::TXT_YNEWS_STOLE = 0;
int oCText::TXT_YNEWS_STOLE_10 = 0;
int oCText::TXT_YNEWS_STOLE_30 = 0;
int oCText::TXT_YNEWS_STOLE_50 = 0;
int oCText::TXT_YNEWS_STOLE_70 = 0;
int oCText::TXT_YNEWS_STOLE_90 = 0;
int oCText::TXT_YNEWS_STOLE_100 = 0;
int oCText::TXT_YNEWS_ATTACKED = 0;
int oCText::TXT_YNEWS_KILLED = 0;
int oCText::TXT_INTIMIDATION = 0;
int oCText::TXT_INVCAT = 0;
int oCText::TXT_HELP = 0;

void oCText::InitText()
{
    TXT_DAYTIME = parser.GetIndex("TXT_DAYTIMES");
    TXT_GUILD = parser.GetIndex("TXT_GUILDS");
    TXT_ATTRIBUTE = parser.GetIndex("TXT_ATTRIBUTES");
    TXT_FIGHT_TALENT = parser.GetIndex("TXT_FIGHT_TALENTS");
    TXT_TALENT = parser.GetIndex("TXT_TALENTS");
    TXT_ATR[0] = parser.GetIndex("TXT_ATR_ST");
    TXT_ATR[1] = parser.GetIndex("TXT_ATR_KO");
    TXT_ATR[2] = parser.GetIndex("TXT_ATR_DK");
    TXT_ATR[3] = parser.GetIndex("TXT_ATR_WK");
    TXT_ATR[4] = parser.GetIndex("TXT_ATR_AG");
    TXT_ATR[5] = parser.GetIndex("TXT_ATR_WI");
    TXT_ATR_HP = parser.GetIndex("TXT_ATR_HP");
    TXT_ATTITUDE = parser.GetIndex("TXT_ATTITUDE");
    TXT_SPELL = parser.GetIndex("TXT_SPELLS");
    TXT_NEWS_INTRO = parser.GetIndex("TXT_NEWS_INTRO");
    TXT_NEWS_STOLE = parser.GetIndex("TXT_NEWS_STOLE");
    TXT_NEWS_STOLE_10 = parser.GetIndex("TXT_NEWS_STOLE_10");
    TXT_NEWS_STOLE_30 = parser.GetIndex("TXT_NEWS_STOLE_30");
    TXT_NEWS_STOLE_50 = parser.GetIndex("TXT_NEWS_STOLE_50");
    TXT_NEWS_STOLE_70 = parser.GetIndex("TXT_NEWS_STOLE_70");
    TXT_NEWS_STOLE_90 = parser.GetIndex("TXT_NEWS_STOLE_90");
    TXT_NEWS_STOLE_100 = parser.GetIndex("TXT_NEWS_STOLE_100");
    TXT_NEWS_ATTACKED = parser.GetIndex("TXT_NEWS_ATTACKED");
    TXT_NEWS_KILLED = parser.GetIndex("TXT_NEWS_KILLED");
    TXT_YNEWS_INTRO = parser.GetIndex("TXT_YNEWS_INTRO");
    TXT_YNEWS_STOLE = parser.GetIndex("TXT_YNEWS_STOLE");
    TXT_YNEWS_STOLE_10 = parser.GetIndex("TXT_YNEWS_STOLE_10");
    TXT_YNEWS_STOLE_30 = parser.GetIndex("TXT_YNEWS_STOLE_30");
    TXT_YNEWS_STOLE_50 = parser.GetIndex("TXT_YNEWS_STOLE_50");
    TXT_YNEWS_STOLE_70 = parser.GetIndex("TXT_YNEWS_STOLE_70");
    TXT_YNEWS_STOLE_90 = parser.GetIndex("TXT_YNEWS_STOLE_90");
    TXT_YNEWS_STOLE_100 = parser.GetIndex("TXT_YNEWS_STOLE_100");
    TXT_YNEWS_ATTACKED = parser.GetIndex("TXT_YNEWS_ATTACKED");
    TXT_YNEWS_KILLED = parser.GetIndex("TXT_YNEWS_KILLED");
    TXT_PC_GREETING = parser.GetIndex("TXT_PC_GREETING_PHRASE");
    TXT_PC_FIRST_PHRASE = parser.GetIndex("TXT_PC_FIRST_PHRASE");
    TXT_PC_SECOND_PHRASE = parser.GetIndex("TXT_PC_SECOND_PHRASE");
    TXT_PC_TRADE_PHRASE = parser.GetIndex("TXT_PC_TRADE_PHRASE");
    TXT_NPC_LAST_PHRASE = parser.GetIndex("TXT_NPC_LAST_PHRASE");
    TXT_PC_TRADE_THREAT = parser.GetIndex("TXT_PC_TRADE_THREAT");
    TXT_NPC_TRADE_THREAT = parser.GetIndex("TXT_NPC_TRADE_THREAT");
    TXT_PC_INFO_THREAT = parser.GetIndex("TXT_PC_INFO_THREAT");
    TXT_NPC_INFO_THREAT = parser.GetIndex("TXT_NPC_INFO_THREAT");
    TXT_INTIMIDATION = parser.GetIndex("TXT_INTIMIDATION");
    TXT_INVCAT = parser.GetIndex("TXT_INVCAT");
    TXT_HELP = parser.GetIndex("TXT_HELP");
}
