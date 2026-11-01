#include "include/oGame.h"
#include <cstring>

oCGuilds::oCGuilds()
{
    size = 66;
    attitude = new unsigned char[size * size];
    std::memset(attitude, 3, size * size);
    InitGuildTable("GIL_ATTITUDES");
}

oCGuilds::~oCGuilds()
{
    delete[] attitude;
    attitude = 0;
}

int oCGuilds::GetAttitude(int firstGuild, int secondGuild)
{
    if (firstGuild >= 66 || secondGuild >= 66) {
        zerr->Message(zSTRING("U:GIL:Unknown Guild ") + zSTRING((long)firstGuild) + ":" +
            zSTRING((long)secondGuild));
        return 2;
    }
    return attitude[secondGuild * size + firstGuild];
}

void oCGuilds::SetAttitude(int firstGuild, int secondGuild, unsigned char value)
{
    if (firstGuild < 66 && secondGuild < 66)
        attitude[secondGuild * size + firstGuild] = value;
}

void oCGuilds::InitGuildTable(const zSTRING& name)
{
    int tableIndex = parser.GetIndex(name);
    if (tableIndex < 0) {
        zerr->Message(zSTRING("U:GUILDS:Guild-Attitudes not found :") + name);
        return;
    }
    zCPar_Symbol* symbol = parser.GetSymbol(tableIndex);
    if ((symbol->ele & 0xfff) != 0x100)
        zerr->Message("U:GUILDS:Guild-attitude-table size mismatch : Size has to be 66");
    int valueIndex = 0;
    for (int first = 0; first < 16; ++first) {
        for (int second = 0; second < 16; ++second) {
            int value;
            symbol->GetValue(value, valueIndex++);
            attitude[second * size + first] = (unsigned char)value;
        }
    }
}

zSTRING oCGuilds::GetGuildName(int guild)
{
    zCPar_Symbol* symbol = parser.GetSymbol("TXT_GUILDS");
    if (!symbol)
        return zSTRING("Error : Missing guild-names !");
    zSTRING name;
    symbol->GetValue(name, guild);
    return name;
}

void oCGuilds::SaveGuildTable(zCArchiver& archiver)
{
    archiver.WriteRaw(attitude, size * size);
}

void oCGuilds::LoadGuildTable(zCArchiver& archiver)
{
    archiver.ReadRaw(attitude, size * size);
}
