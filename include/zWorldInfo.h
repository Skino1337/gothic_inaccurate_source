#ifndef ZWORLDINFO_H
#define ZWORLDINFO_H

#include "zString.h"
#include "zModel.h"

class zCPlayerGroup;
class zCPlayerInfo;

class zCWorldInfo {
public:

    zCWorldInfo(const zSTRING& name);
    virtual ~zCWorldInfo();
    zSTRING GetName();
    zCPlayerGroup* GetPlayerGroup();
    int IsMember(zCPlayerInfo* player);
    zCPlayerInfo* GetLevelMaster();
    void EnterWorld(zCPlayerInfo* player);
    void LeaveWorld(zCPlayerInfo* player);
    void ShowDebugInfo();

    int isRunning;
    zSTRING name;
    zCPlayerGroup* playerGroup_Level;
    zCList<zCPlayerInfo> historyList;
};

#endif
