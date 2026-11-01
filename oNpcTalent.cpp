#include "include/oNpcTalent.h"

static zCClassDef oCNpcTalentClassDef("oCNpcTalent", "zCObject", oCNpcTalent::_CreateNewInstance,
    0, 0x30, 1, "");
zCClassDef* oCNpcTalent::classDef = &oCNpcTalentClassDef;

oCNpcTalent::oCNpcTalent()
    : m_talent(NPC_TAL_UNKNOWN), m_skill(0), m_value(-1)
{
}

oCNpcTalent::~oCNpcTalent()
{
}

zCObject* oCNpcTalent::_CreateNewInstance()
{
    oCNpcTalent* talent = new oCNpcTalent;
    zCClassDef::ObjectCreated(talent, classDef);
    return talent;
}

void oCNpcTalent::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCNpcTalent::_GetClassDef() const
{
    return classDef;
}

void oCNpcTalent::CreateTalentList(zCArray<oCNpcTalent*>* talents)
{
    talents->DeleteList();
    for (int i = 0; i < NPC_TAL_MAX; ++i)
        talents->InsertEnd(0);
}

void oCNpcTalent::Archive(zCArchiver& archiver)
{
    archiver.WriteInt(m_talent);
    archiver.WriteInt(m_value);
    archiver.WriteInt(m_skill);
}

void oCNpcTalent::Unarchive(zCArchiver& archiver)
{
    m_talent = static_cast<oTEnumNpcTalent>(archiver.ReadInt());
    m_value = archiver.ReadInt();
    m_skill = archiver.ReadInt();
}
