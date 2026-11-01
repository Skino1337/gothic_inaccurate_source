#ifndef ONPCTALENT_H
#define ONPCTALENT_H

#include "zObject.h"

class oCNpcTalent : public zCObject {
public:

    enum oTEnumNpcTalent {
        NPC_TAL_UNKNOWN,
        NPC_TAL_1H,
        NPC_TAL_2H,
        NPC_TAL_BOW,
        NPC_TAL_CROSSBOW,
        NPC_TAL_PICKLOCK,
        NPC_TAL_PICKPOCKET,
        NPC_TAL_MAGE,
        NPC_TAL_SNEAK,
        NPC_TAL_REGENERATE,
        NPC_TAL_FIREMASTER,
        NPC_TAL_ACROBAT,
        NPC_TALENT_PICKPOCKET,
        NPC_TAL_SMITH,
        NPC_TAL_RUNES,
        NPC_TAL_ALCHEMY,
        NPC_TAL_TAKEANIMALTROPHY,
        NPC_TAL_A,
        NPC_TAL_B,
        NPC_TAL_C,
        NPC_TAL_D,
        NPC_TAL_E,
        NPC_TAL_MAX
    };

    oCNpcTalent();
    virtual ~oCNpcTalent();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    static void      CreateTalentList(zCArray<oCNpcTalent*>* talents);

    virtual zCClassDef* _GetClassDef() const;
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);

    oTEnumNpcTalent m_talent;
    int             m_skill;
    int             m_value;

    static zCClassDef* classDef;
};

#endif
