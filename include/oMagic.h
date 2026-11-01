#ifndef OMAGIC_H
#define OMAGIC_H

#include "oCollisionObjectMiscChar.h"

class oCMag_Book {
public:

    oCMag_Book();
    ~oCMag_Book();
    void SetOwner(zCVob* owner);
    void SetWorld(zCWorld* world);
    int Register(oCSpell* spell, int active);
    int Register(oCItem* item, int active);
    int DeRegister(oCItem* item);
    int DeRegister(int index);
    oCSpell* CreateNewSpell(int spellID);
    int Register(int spellID, int active);
    int IsIn(int spellID);
    int GetNoOfSpells();
    oCSpell* GetSpell(oCItem* item, int& index);
    oCSpell* GetSpell(int index);
    oCItem* GetSpellItem(int index);
    oCItem* GetSpellItem(oCSpell* spell);
    oCSpell* GetSelectedSpell();
    int GetSelectedSpellNr();
    void CheckConsistency();
    void Spell_Setup(int index, oCNpc* npc, zCVob* target);
    void Spell_Setup(oCNpc* npc, zCVob* target, int flags);
    void Spell_Invest();
    void Spell_Cast();
    int Spell_InCast();
    float Ease(float time, float start, float end);
    zVEC3 EasePos(float time, zVEC3& start, zVEC3& end);
    void StartInvestEffect(zCVob* vob, int key, int spellID, int flags);
    void StartCastEffect(zCVob* vob, const zVEC3& position);
    int IsInMovement();
    int IsActive();
    void Open(int spell);
    void Close(int removeAll);
    void Spell_Open(int index);
    void StopSelectedSpell();
    void KillSelectedSpell();
    void Spell_Stop(int index);
    void Left();
    void Right();
    int DoOpen();
    int DoClose();
    void SetFrontSpell(int index);
    int DoTurn();
    void ShowHandSymbol();
    void SetShowHandSymbol(int enabled);
    void DoPerFrame();
    zVEC3 CalcPos(zCVob* vob, float angle);
    float GetAngle(zCVob* vob);
    void NextRegisterAt(int key);
    oCSpell* GetSpellByKey(int key);
    oCItem* GetSpellItemByKey(int key);
    int GetNoOfSpellByKey(int key);
    int GetKeyByItem(oCItem* item);

    zCArray<oCSpell*> spells;
    zCArray<oCItem*> spellitems;
    zCWorld* wld;
    zCVob* owner;
    zCModel* model;
    int spellnr;
    float MAG_HEIGHT;
    int active;
    int remove_all;
    int in_movement;
    int show_handsymbol;
    float step;
    int action;
    int fullangle;
    int open;
    float open_delay_timer;
    int show_particles;
    float targetdir;
    float t1;
    zVEC3 targetpos;
    zVEC3 startpos;
    int nextRegister;
    unsigned int keys;
};

#endif
