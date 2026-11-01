#ifndef OMENU_STATUS_H
#define OMENU_STATUS_H

#include "zMenu.h"
#include "oLogManager.h"

class oCMenu_Log : public zCMenu {
public:

    oCMenu_Log(const zSTRING& name);
    virtual ~oCMenu_Log();
    void SetDayTime(const zSTRING& day, const zSTRING& time);
    void SetLogTopics(const zCList<oCLogTopic>& topics);
    void SetInfo(const zCMenuItem* item);
    virtual void ScreenInit();
    virtual void ScreenDone();
    virtual int ExecCommand(const zSTRING& command);
    virtual int HandleEnterItem(zCMenuItem* item);
    virtual int HandleLeaveItem(zCMenuItem* item);
    virtual int HandleActivateItem(zCMenuItem* item);
    virtual int HandleEvent(int key);
    virtual void Activate();
    void InitForDisplay();

    zCMenuItemList* listItemAct;
    zCMenuItemList* listItemOld;
    zCMenuItemList* listItemFailed;
    zCMenuItemList* listItemLog;
    zCMenuItemText* contentViewer;
};

struct oSMenuInfoAttribute {
    enum oTMenuAttrib {
        ATTRIB_HP,
        ATTRIB_DEX,
        ATTRIB_MANA,
        ATTRIB_STR
    };

    zSTRING name;
    zSTRING descr;
    int actValue;
    int maxValue;
    oTMenuAttrib type;
};

struct oSMenuInfoArmor {
    enum oTMenuArmor {
        ATTRIB_1H,
        ATTRIB_2H,
        ATTRIB_BOW,
        ATTRIB_CROSSBOW
    };

    int actValue;
};

struct oSMenuInfoTalent {
    zSTRING name;
    zSTRING desc;
    zSTRING skillEnum;
    int value;
    int skill;
};

class oCMenu_Status : public zCMenu {
public:

    oCMenu_Status(const zSTRING& name) : zCMenu(name) {}
    virtual ~oCMenu_Status() {}
    virtual int HandleEvent(int key);
    void SetPlayerName(const zSTRING& name);
    void SetGuild(const zSTRING& guild);
    void SetMagicalGuild(const zSTRING& guild);
    void SetExperience(unsigned long experience, unsigned long level, unsigned long nextLevel);
    void SetLearnPoints(unsigned long points);
    void AddArmor(const oSMenuInfoArmor& armor);
    void AddAttribute(const oSMenuInfoAttribute& attribute);
    void AddTalent(const oSMenuInfoTalent& talent);
    void InitForDisplay();

    zCArray<oSMenuInfoAttribute> m_listAttributes;
    zCArray<oSMenuInfoTalent> m_listTalents;
    zCArray<oSMenuInfoArmor> m_listArmory;
};

class oCStatusScreen {
public:

    oCStatusScreen();
    virtual ~oCStatusScreen();
    void Show();

    oCMenu_Status* m_pStatusMenu;
};

class oCLogScreen {
public:

    oCLogScreen();
    virtual ~oCLogScreen();
    void Show();

    oCMenu_Log* m_pLogMenu;
};

class oCMapScreen {
public:

    oCMapScreen() {}
    virtual ~oCMapScreen() {}
    void Show(int mode);
};

#endif
