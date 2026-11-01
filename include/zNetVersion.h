#ifndef ZNETVERSION_H
#define ZNETVERSION_H

#include "zString.h"
#include "zBuffer.h"

class zCNetVersion {
public:

    zCNetVersion();
    virtual ~zCNetVersion();
    virtual zSTRING GetVersionString() const;
    virtual long GetVersionNumber() const;
    virtual long GetChecksum() const;
    virtual long CreateChecksum();
    virtual void Pack(zCBuffer& buffer) const;
    virtual void Unpack(zCBuffer& buffer);

    long checksum;
    long version;
};

int operator==(const zCNetVersion& left, const zCNetVersion& right);

#endif
