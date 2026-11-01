#ifndef ZOPTION_H
#define ZOPTION_H

#include "zObject.h"

class zCOptionEntry {
public:

    zCOptionEntry(const zSTRING& name, const zSTRING& value);
    ~zCOptionEntry();

    int changed;
    zCArray<int (*)(zCOptionEntry&)> ccbList;
    zSTRING varName;
    zSTRING varValue;
    zSTRING varValueTemp;
    int varFlag;
};

class zCOptionSection {
public:

    zCOptionSection(const zSTRING& name);
    ~zCOptionSection();

    zSTRING secName;
    zCArray<zCOptionEntry*> entryList;
};

#endif
