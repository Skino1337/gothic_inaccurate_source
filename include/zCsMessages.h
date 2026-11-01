#ifndef ZCSMESSAGES_H
#define ZCSMESSAGES_H

#include "zVob.h"

class zCEvMsgCutscene : public zCEventMessage {
public:

    enum TCutsceneSubType {
        EV_CS_STARTPLAY,
        EV_CS_STOP,
        EV_CS_INTERRUPT,
        EV_CS_RESUME
    };

    zCEvMsgCutscene();
    zCEvMsgCutscene(TCutsceneSubType eventSubType);
    virtual ~zCEvMsgCutscene();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void SetCutsceneName(zSTRING name);
    virtual int IsHighPriority();
    virtual void Delete();
    virtual int IsDeleted();
    virtual int MD_GetNumOfSubTypes();
    virtual zSTRING MD_GetSubTypeString(int eventSubType);

    zSTRING csName;
    int isOutputUnit;
    int isGlobalCutscene;
    int isMainRole;
    int deleted;

    static zCClassDef* classDef;
};

#endif
