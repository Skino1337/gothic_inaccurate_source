#include "include/zNetVersion.h"
#include "include/zObject.h"

zCNetVersion::zCNetVersion()
    : checksum(0), version(1)
{
}

zCNetVersion::~zCNetVersion()
{
}

zSTRING zCNetVersion::GetVersionString() const
{
    return zSTRING((int)version);
}

long zCNetVersion::GetVersionNumber() const
{
    return version;
}

long zCNetVersion::GetChecksum() const
{
    return checksum;
}

long zCNetVersion::CreateChecksum()
{
    checksum = GetVersionNumber();
    return checksum;
}

void zCNetVersion::Pack(zCBuffer& buffer) const
{
    buffer.Write(&version, 4);
    buffer.Write(&checksum, 4);
}

void zCNetVersion::Unpack(zCBuffer& buffer)
{
    buffer.Read(&version, 4);
    buffer.Read(&checksum, 4);
}

int operator==(const zCNetVersion& left, const zCNetVersion& right)
{
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: VERSION: Checksums: " + zSTRING((int)left.checksum) + " / " +
            zSTRING((int)right.checksum), 4, 0, 112,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetVersion.cpp", NULL);
    return left.checksum == right.checksum && left.version == right.version;
}
