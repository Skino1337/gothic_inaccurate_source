#include "include/oMagic.h"
#include "include/zAlgebra.h"
#include <cmath>

oCMag_Book::oCMag_Book()
    : wld(NULL), owner(NULL), model(NULL), spellnr(0), MAG_HEIGHT(90.0f), active(0),
      remove_all(0), in_movement(0), show_handsymbol(0), step(0.0f), action(0), fullangle(0),
      open(0), open_delay_timer(0.0f), show_particles(0), targetdir(0.0f), t1(0.0f),
      nextRegister(0), keys(0)
{
    targetpos.n[0] = 0.0f;
    targetpos.n[1] = 0.0f;
    targetpos.n[2] = 0.0f;
    startpos = targetpos;
}

void oCMag_Book::StartInvestEffect(zCVob*, int, int, int)
{
    zerr->Report(zERR_TYPE_INFO, 0, "C: MAG: StartInvestEffect", 8, 0, 465,
        "P:\\dev\\g2addon\\release\\Gothic\\_carsten\\oMagic.cpp", NULL);
}

void oCMag_Book::StartCastEffect(zCVob*, const zVEC3&)
{
    zerr->Report(zERR_TYPE_INFO, 0, "C: MAG: StartCastEffect", 8, 0, 575,
        "P:\\dev\\g2addon\\release\\Gothic\\_carsten\\oMagic.cpp", NULL);
}

oCMag_Book::~oCMag_Book()
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spells[i])
        {
            spells[i]->Stop();
            spells[i]->Release();
            spells[i] = NULL;
        }
    }
    spells.numInArray = 0;
    spellitems.numInArray = 0;
    wld = NULL;
    owner = NULL;
    model = NULL;
}

void oCMag_Book::SetOwner(zCVob* value)
{
    owner = value;
    wld = owner->homeWorld;
    model = dynamic_cast<zCModel*>(owner->visual);
    MAG_HEIGHT = owner->bbox3D.maxs.n[1] - owner->trafoObjToWorld.m[1][3] + 10.0f;
}

void oCMag_Book::SetWorld(zCWorld* world)
{
    wld = world;
}

int oCMag_Book::Register(oCSpell* spell, int)
{
    if (!spell)
        return 0;
    spell->AddRef();
    spells.InsertEnd(spell);
    return 1;
}

int oCMag_Book::Register(oCItem* item, int)
{
    if (spells.numInArray >= 7)
        return 0;
    if (!nextRegister)
    {
        int key = 0;
        while (keys & (1 << key))
            ++key;
        nextRegister = key + 1;
        keys |= 1 << key;
    }
    oCSpell* spell = CreateNewSpell(item->spellID);
    if (spell)
    {
        Register(spell, 1);
        spellitems.InsertEnd(item);
        spell->keyNo = nextRegister;
        spell->Release();
    }
    nextRegister = 0;
    return 1;
}

int oCMag_Book::DeRegister(oCItem* item)
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spellitems[i] == item)
            return DeRegister(i);
    }
    return 0;
}

int oCMag_Book::DeRegister(int index)
{
    oCSpell* spell = spells[index];
    keys &= ~(1 << (spell->keyNo - 1));
    spellitems.Remove(spellitems[index]);
    spells.Remove(spell);
    spell->Release();
    return 1;
}

oCSpell* oCMag_Book::CreateNewSpell(int spellID)
{
    oCSpell* spell = new oCSpell;
    spell->spellID = spellID;
    zCClassDef::ObjectCreated(spell, oCSpell::classDef);
    return spell;
}

int oCMag_Book::Register(int spellID, int active)
{
    oCSpell* spell = CreateNewSpell(spellID);
    if (spell)
        Register(spell, active);
    return (int)spell;
}

int oCMag_Book::IsIn(int spellID)
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spells[i]->GetSpellID() == spellID)
            return 1;
    }
    return 0;
}

int oCMag_Book::GetNoOfSpells()
{
    return spells.numInArray;
}

oCSpell* oCMag_Book::GetSpell(oCItem* item, int& index)
{
    for (index = 0; index < spells.numInArray; ++index)
    {
        if (spellitems[index] == item)
            return spells[index];
    }
    index = 0x7f35;
    return NULL;
}

oCSpell* oCMag_Book::GetSpell(int index)
{
    if (spells.numInArray && index <= spells.numInArray)
        return spells[index];
    return NULL;
}

oCItem* oCMag_Book::GetSpellItem(int index)
{
    if (index >= spells.numInArray)
        return NULL;
    return spellitems[index];
}

oCItem* oCMag_Book::GetSpellItem(oCSpell* spell)
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spells[i] == spell)
            return spellitems[i];
    }
    return NULL;
}

oCSpell* oCMag_Book::GetSelectedSpell()
{
    if (spellnr < 0 || spellnr >= spells.numInArray)
        return NULL;
    return spells[spellnr];
}

int oCMag_Book::GetSelectedSpellNr()
{
    return spellnr;
}


void oCMag_Book::CheckConsistency()
{
    unsigned int realKeys = 0;
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (!spells[i])
            continue;
        realKeys |= 1 << (spells[i]->keyNo - 1);
        if (spells[i]->GetSpellID() != spellitems[i]->spellID)
            spells[i]->spellID = spellitems[i]->spellID;
    }
    keys = realKeys;
}

void oCMag_Book::Spell_Setup(int index, oCNpc* npc, zCVob* target)
{
    spells[index]->Setup(npc, target, 0);
}

void oCMag_Book::Spell_Setup(oCNpc* npc, zCVob* target, int flags)
{
    oCSpell* spell = GetSelectedSpell();
    if (spell)
        spell->Setup(npc, target, flags);
}

void oCMag_Book::Spell_Invest()
{
    oCSpell* spell = GetSelectedSpell();
    if (spell)
        spell->Invest();
}

void oCMag_Book::Spell_Cast()
{
    oCSpell* spell = GetSelectedSpell();
    if (!spell)
        return;

    parser.SetInstance("SELF", spell->spellCasterNpc);
    parser.SetInstance("OTHER", spell->spellTargetNpc);
    int castIndex = parser.GetIndex("SPELL_CAST_" + spell->GetObjectName());
    if (castIndex > 0)
        parser.CallFunc(castIndex, spell->GetLevel());

    int spellID = spell->GetSpellID();
    int keyNo = spell->keyNo;
    int canBeDeleted = spell->canBeDeleted;
    oCItem* item = spellitems[spellnr];
    oCNpc* ownerNpc = dynamic_cast<oCNpc*>(owner);
    spells[spellnr] = NULL;

    if (!spell->Cast())
        spells[spellnr] = spell;
    else
    {
        spell->Release();
        oCSpell* newSpell = CreateNewSpell(spellID);
        spells[spellnr] = newSpell;
        newSpell->keyNo = keyNo;
        bool setupImmediately = !item->MultiSlot() || !canBeDeleted || item->amount <= 1;
        if (remove_all && setupImmediately)
            newSpell->Setup(ownerNpc, NULL, 0);
    }

    if (item->MultiSlot() && canBeDeleted)
    {
        if (item->amount > 0)
            --item->amount;
        if (!item->amount)
        {
            if (ownerNpc)
                delete ownerNpc->RemoveFromInv(item, 0);
            if (spells[spellnr])
                spells[spellnr]->Setup(ownerNpc, NULL, 0);
        }
    }
}

int oCMag_Book::Spell_InCast()
{
    return 0;
}

float oCMag_Book::Ease(float time, float start, float end)
{
    float factor = (sinf(time * 3.1415926535897931f - 1.5707963267948966f) + 1.0f) * 0.5f;
    return factor * (end - start) + start;
}

zVEC3 oCMag_Book::EasePos(float time, zVEC3& start, zVEC3& end)
{
    float factor = (sinf(time * 3.1415926535897931f - 1.5707963267948966f) + 1.0f) * 0.5f;
    zVEC3 result;
    for (int i = 0; i < 3; ++i)
        result.n[i] = (end.n[i] - start.n[i]) * factor + start.n[i];
    return result;
}

int oCMag_Book::IsInMovement()
{
    return in_movement;
}

int oCMag_Book::IsActive()
{
    return 0;
}


void oCMag_Book::Open(int spell)
{
    if (open || !spells.numInArray)
        return;

    wld = owner->homeWorld;
    if (spellnr >= 0 && show_handsymbol)
        Spell_Stop(spellnr);
    for (int i = 0; i < spells.numInArray; ++i)
        spells[i]->Reset();

    if (spell)
    {
        oCSpell* selected = GetSelectedSpell();
        if (!selected)
            return;
        zCVob* vob = selected->GetVob();
        if (!vob->homeWorld)
            wld->AddVob(vob);
        selected->Reset();
        vob->SetPositionWorld(owner->trafoObjToWorld.v[3]);
        oCItem* item = spellitems[spellnr];
        if (!item->visual)
            item->CreateVisual();
        if (item->visual)
            vob->SetVisual(item->visual);
        in_movement = 0;
        show_particles = 0;
        open_delay_timer = 0.0f;
        SetShowHandSymbol(1);
        return;
    }

    open = 1;
    step = 360.0f / spells.numInArray;
    startpos = owner->trafoObjToWorld.v[3];
    for (int i = 0; i < spells.numInArray; ++i)
    {
        zCVob* vob = spells[i]->GetVob();
        if (!vob->homeWorld)
            wld->AddVob(vob);
        spells[i]->Reset();
        vob->SetPositionWorld(startpos);
        vob->SetCollDetStat(0);
        vob->SetCollDetDyn(0);
        oCItem* item = spellitems[i];
        if (!item->visual)
            item->CreateVisual();
        if (item->visual)
            vob->SetVisual(item->visual);
    }
    t1 = 0.0f;
    action = 2;
    in_movement = 1;
    show_particles = 0;
    open_delay_timer = 2000.0f;
    SetShowHandSymbol(0);
}

void oCMag_Book::Close(int removeAll)
{
    if (!open)
    {
        SetShowHandSymbol(!removeAll);
        return;
    }
    t1 = 0.0f;
    in_movement = 1;
    action = 3;
    remove_all = removeAll;
}

void oCMag_Book::Spell_Open(int index)
{
    spells[index]->Open();
}

void oCMag_Book::StopSelectedSpell()
{
    if (GetSelectedSpell())
        Spell_Stop(spellnr);
}

void oCMag_Book::KillSelectedSpell()
{
    oCSpell* spell = GetSelectedSpell();
    if (!wld || !spell)
        return;

    int spellID = spell->GetSpellID();
    zVEC3 position = spell->GetVob()->trafoObjToWorld.v[3];
    int keyNo = spell->Kill();
    oCSpell* newSpell = CreateNewSpell(spellID);
    spells[spellnr] = newSpell;
    newSpell->keyNo = keyNo;
    zCVob* vob = newSpell->GetVob();
    oCItem* item = spellitems[spellnr];
    if (!item->visual)
        item->CreateVisual();
    if (item->visual)
        vob->SetVisual(item->visual);
    if (show_handsymbol || open)
    {
        vob->SetPositionWorld(position);
        wld->AddVob(vob);
    }
    show_particles = 0;
    open_delay_timer = 2000.0f;
}

void oCMag_Book::Spell_Stop(int index)
{
    if (index >= 0 && index < spells.numInArray && spells[index])
        spells[index]->Stop();
}

void oCMag_Book::Left()
{
    if (!open || in_movement || spells.numInArray == 1)
        return;
    in_movement = 1;
    Spell_Stop(spellnr);
    targetdir = -step;
    action = 1;
    t1 = 0.0f;
    if (++spellnr >= spells.numInArray)
        spellnr = 0;
}

void oCMag_Book::Right()
{
    if (!open || in_movement || spells.numInArray == 1)
        return;
    in_movement = 1;
    Spell_Stop(spellnr);
    targetdir = step;
    action = 1;
    t1 = 0.0f;
    if (--spellnr < 0)
        spellnr = spells.numInArray - 1;
}

int oCMag_Book::DoOpen()
{
    extern zTIMER ztimer;
    t1 += ztimer.frameTimeFloat / 250.0f;
    if (t1 > 1.0f)
        t1 = 1.0f;

    startpos = owner->trafoObjToWorld.v[3];
    for (int i = 0; i < spells.numInArray; ++i)
    {
        zCVob* vob = spells[i]->GetVob();
        targetpos = CalcPos(vob, (i - spellnr) * step);
        zVEC3 position = EasePos(t1, startpos, targetpos);
        if (vob)
            vob->SetPositionWorld(position);
    }
    return t1 >= 1.0f;
}

int oCMag_Book::DoClose()
{
    if (!wld)
        return 1;

    t1 += ztimer.frameTimeFloat / 250.0f;
    if (t1 > 1.0f)
        t1 = 1.0f;
    targetpos = owner->trafoObjToWorld.v[3];
    for (int i = 0; i < spells.numInArray; ++i)
    {
        zCVob* vob = spells[i]->GetVob();
        zVEC3 start = CalcPos(vob, (i - spellnr) * step);
        zVEC3 position = EasePos(t1, start, targetpos);
        if (vob)
            vob->SetPositionWorld(position);
    }
    if (t1 < 1.0f)
        return 0;

    for (int i = 0; i < spells.numInArray; ++i)
    {
        zCVob* vob = spells[i]->GetVob();
        if (vob && vob->homeWorld)
            vob->homeWorld->RemoveVobSubtree(vob);
        if (i == spellnr && remove_all)
            Spell_Stop(i);
    }
    action = 0;
    in_movement = 0;
    open = 0;
    show_particles = 0;
    open_delay_timer = 0.0f;
    SetShowHandSymbol(!remove_all);
    return 1;
}

void oCMag_Book::SetFrontSpell(int index)
{
    oCNpc* ownerNpc = dynamic_cast<oCNpc*>(owner);
    if (ownerNpc && ownerNpc->GetWeaponMode() != 7)
    {
        spellnr = index;
        return;
    }

    if (index != spellnr)
    {
        Spell_Stop(spellnr);
        if (!open && wld)
            wld->RemoveVob(spells[spellnr]->GetVob());
    }
    spellnr = index;
    in_movement = 0;
    action = 0;

    if (!open)
        Spell_Setup(spellnr, ownerNpc, NULL);
    else
    {
        for (int i = 0; i < spells.numInArray; ++i)
        {
            zCVob* vob = spells[i]->GetVob();
            targetpos = CalcPos(vob, (i - spellnr) * step);
            if (vob)
                vob->SetPositionWorld(targetpos);
        }
    }
}

int oCMag_Book::DoTurn()
{
    t1 += ztimer.frameTimeFloat / 250.0f;
    if (t1 > 1.0f)
        t1 = 1.0f;

    float turn = Ease(t1, 0.0f, targetdir);
    int direction = targetdir < 0.0f ? -1 : 1;
    for (int i = 0; i < spells.numInArray; ++i)
    {
        zCVob* vob = spells[i]->GetVob();
        targetpos = CalcPos(vob, (i - spellnr - direction) * step + turn);
        if (vob)
            vob->SetPositionWorld(targetpos);
    }
    return t1 >= 1.0f;
}

void oCMag_Book::ShowHandSymbol()
{
    oCSpell* spell = GetSelectedSpell();
    if (!spell)
        return;
    zCVob* vob = spell->GetVob();
    if (vob)
    {
        zMAT4 trafo = owner->GetTrafoModelNodeToWorld("ZS_RIGHTHAND");
        vob->SetPositionWorld(trafo.v[3]);
    }
}

void oCMag_Book::SetShowHandSymbol(int enabled)
{
    show_handsymbol = enabled;
    if (enabled && !open)
        Spell_Setup(spellnr, (oCNpc*)owner, NULL);
}

void oCMag_Book::DoPerFrame()
{
    if (!action && !open && !show_handsymbol)
        return;

    int finished = 1;
    if (action == 1)
    {
        finished = DoTurn();
        if (finished)
            zCView::stdoutput->Printwin(GetSelectedSpell()->GetName() + "\n");
    }
    else if (action == 2)
        finished = DoOpen();
    else if (action == 3)
        finished = DoClose();
    else
        action = 0;
    if (finished)
        action = 0;
    in_movement = !finished;

    if (show_handsymbol && spellnr >= 0)
        ShowHandSymbol();
    if (open)
    {
        if (!action && !show_particles && GetSelectedSpell() &&
            GetSelectedSpell()->GetVob()->homeWorld)
        {
            open_delay_timer -= ztimer.frameTimeFloat;
            if (open_delay_timer <= 0.0f)
            {
                Spell_Open(spellnr);
                show_particles = 1;
            }
        }

        for (int i = 0; i < spells.numInArray; ++i)
        {
            zCVob* vob = spells[i]->GetVob();
            if (vob)
            {
                vob->ResetRotationsLocal();
                vob->trafoObjToWorld.PostRotateX(-90.0f);
                vob->UpdateVisualDependencies(1);
            }
        }
    }
}


zVEC3 oCMag_Book::CalcPos(zCVob*, float angle)
{
    zMAT4 rotation = Alg_Rotation3DN(zVEC3(0.0f, 1.0f, 0.0f), angle);
    zMAT4 trafo = rotation * owner->trafoObjToWorld;
    zVEC3 result = owner->trafoObjToWorld.v[3] + trafo.v[0] * 90.0f;
    float normalizedAngle = angle;
    if (normalizedAngle >= 180.0f)
        normalizedAngle -= 360.0f;
    else if (normalizedAngle < -180.0f)
        normalizedAngle += 360.0f;
    result.n[1] -= fabsf(normalizedAngle) / 3.6f;
    return result;
}

float oCMag_Book::GetAngle(zCVob* vob)
{
    zVEC3 direction = vob->trafoObjToWorld.v[3] - owner->trafoObjToWorld.v[3];
    float ownerAngle = atan2f(owner->trafoObjToWorld.m[0][2],
        owner->trafoObjToWorld.m[2][2]);
    float targetAngle = atan2f(direction.n[0], direction.n[2]);
    float angle = (targetAngle - ownerAngle) * 57.2957764f;
    if (angle > 180.0f)
        angle -= 360.0f;
    else if (angle < -180.0f)
        angle += 360.0f;
    return fabsf(angle);
}

void oCMag_Book::NextRegisterAt(int key)
{
    if (key >= 0 && key < 0x7d00)
        nextRegister = key;
}

oCSpell* oCMag_Book::GetSpellByKey(int key)
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spells[i]->keyNo == key)
            return spells[i];
    }
    return NULL;
}

oCItem* oCMag_Book::GetSpellItemByKey(int key)
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spells[i]->keyNo == key)
            return spellitems[i];
    }
    return NULL;
}

int oCMag_Book::GetNoOfSpellByKey(int key)
{
    if (!(keys & (1 << (key - 1))))
        return 0;
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spells[i]->keyNo == key)
            return i + 1;
    }
    return 0;
}

int oCMag_Book::GetKeyByItem(oCItem* item)
{
    for (int i = 0; i < spells.numInArray; ++i)
    {
        if (spellitems[i] == item)
            return spells[i]->keyNo;
    }
    return 0x7f35;
}

void Alg_CalcAziElev(const zVEC3& vector, float& azimuth, float& elevation)
{
    float x = fabsf(vector.n[0]);
    float y = fabsf(vector.n[1]);
    float z = fabsf(vector.n[2]);
    float largest = x;
    if (y > largest)
        largest = y;
    if (z > largest)
        largest = z;
    float length = largest - 0.0625f * largest + (x + y + z - largest) * 0.375f;
    if (length == 0.0f)
        length = 0.000001f;
    azimuth = atan2f(vector.n[2], vector.n[0]);
    float vertical = vector.n[1] / length;
    if (vertical > 1.0f)
        vertical = 1.0f;
    else if (vertical < -1.0f)
        vertical = -1.0f;
    elevation = 1.57079637f - acosf(vertical);
}
