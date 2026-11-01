#include "include/oCollisionObjectMiscChar.h"
#include "include/zViewBase.h"
#include "include/zTools.h"
#include "include/oAiShoot.h"
#include <cmath>

void ApplyDamages(unsigned long damageTypes, int* damage, int damageTotal);

static int itemState = -1;
static float itemFarDistance = 250000.0f * 0.0166666675f;
static float itemNearDistance = itemFarDistance * 0.0166666675f;
static float itemFarPlaneDistance = 250000.0f * 24.0f;
// EXE: 0x00AB11D4
static zCArray<int> itemList(600);
// EXE: 0x00AB11E4
static int itemReferencesCreated;
static zCClassDef oCItemClassDef("oCItem", "oCVob", oCItem::_CreateNewInstance, 0, 0x348, 0, "");
zCClassDef* oCItem::classDef = &oCItemClassDef;
static int itemLightingSwell = 1;
static int itemClassIndex = -1;

void oCItem::SetLightingSwell(int enabled)
{
    itemLightingSwell = enabled;
}

int oCItem::GetLightingSwell()
{
    return itemLightingSwell;
}

int oCVob::DoFocusCheckBBox()
{
    return 0;
}

void oCVob::SetSoundMaterial(oTSndMaterial material)
{
}

int oCVob::GetCSStateFlags()
{
    return 0;
}

oCItem::oCItem()
{
    Init();
}

oCItem::oCItem(zSTRING& itemName, int itemAmount)
{
    nameID = itemName;
    Init();
    InitByScript(parser.GetIndex(itemName), itemAmount);
}

oCItem::oCItem(int instance, int itemAmount)
{
    Init();
    InitByScript(instance, itemAmount);
}

// EXE: 0x00711730
oCItem::~oCItem()
{
    if (effectVob) {
        effectVob->Stop(1);
        effectVob->Release();
        effectVob = 0;
    }
    parser.ResetWithVarReferenceList(itemList, this);
    next = 0;
}

zCClassDef* oCItem::_GetClassDef() const
{
    return classDef;
}

int oCItem::GetInstance() const
{
    return instanz;
}

oTSndMaterial oCItem::GetSoundMaterial() const
{
    return sndMat;
}

void oCItem::SetVisual(zCVisual* newVisual)
{
    zCVob::SetVisual(newVisual);
    SetCollisionClass(&zCCollObjectPoint::s_oCollObjClass);
}

void oCItem::CreateVisual()
{
    if (!visual && file.Search(".ZEN") < 0)
        zCVob::SetVisual(file);
}

// EXE: 0x00711970
void oCItem::Init()
{
    instanz = -1;
    mainflag = -1;
    flags = 0;
    amount = 1;
    damageTypes = 0;
    damageTotal = 0;
    disguiseGuild = 0;
    effectVob = 0;
    next = 0;
    range = 0;
    spellID = 0;
    refCtr = 1;
    sndMat = oTSNDMAT_DEFAULT;
    description = "";
    for (int i = 0; i < 8; ++i)
        damage[i] = 0;
    for (int i = 0; i < 6; ++i) {
        text[i] = "";
        count[i] = 0;
    }
    inv_zbias = 0;
    c_manipulation = 0;
    type = zVOB_TYPE_ITEM;
    m_zBias = 1;
    SetCollisionClass(&zCCollObjectPoint::s_oCollObjClass);
    if (!itemReferencesCreated) {
        parser.CreateVarReferenceList("C_ITEM", itemList);
        itemReferencesCreated = 1;
    }
}

void oCItem::InitByScript(int instance, int itemAmount)
{
    if (itemClassIndex < 0) {
        itemClassIndex = parser.GetIndex("C_ITEM");
        parser.CheckClassSize(itemClassIndex, 0x20c);
    }
    instanz = instance;
    parser.CreateInstance(instance, this);
    flags |= mainflag;
    amount = itemAmount;
    if (damage[0])
        zerr->Message(zSTRING("R: ITEM: item \"") + name + "\" causes DAM_INDEX_BARRIER damage.");
    ApplyDamages(damageTypes, damage, damageTotal);
    file.Upper();
    if (file.Search(".ZEN") > 0) {
        SetVisual(0);
        SetCollDetStat(0);
        SetCollDetDyn(0);
        ogame->GetGameWorld()->AddVob(this);
        zVEC3 position = { 0.0f, 0.0f, 0.0f };
        SetPositionWorld(position);
        zCVob* subtree = ogame->GetGameWorld()->MergeVobSubtree(file, 0, 0);
        if (subtree) {
            subtree->SetCollDetStat(0);
            subtree->SetCollDetDyn(0);
            subtree->SetPositionWorld(position);
            SetTrafoObjToWorld(subtree->trafoObjToWorld);
            subtree->AddRefVobSubtree();
            ogame->GetGameWorld()->RemoveVobSubtree(subtree);
            ogame->GetGameWorld()->AddVobAsChild(this, subtree);
            ogame->GetGameWorld()->RemoveVobSubtree(this);
        }
    } else if (!file.IsEmpty()) {
        CreateVisual();
    }
    type = zVOB_TYPE_ITEM;
    int first = 0;
    int second = 0;
    SetVobName(parser.GetSymbolInfo(instance, first, second));
}

zSTRING& oCItem::GetDescription()
{
    return description;
}

zSTRING& oCItem::GetText(int index)
{
    static zSTRING empty;
    return index >= 0 && index < 6 ? text[index] : empty;
}

int oCItem::GetCount(int index) const
{
    return index >= 0 && index < 6 ? count[index] : 0;
}

int oCItem::GetHealMode(int& value) const
{
    if (!(flags & 0x20))
        return 10;
    value = nutrition;
    if (nutrition > 0)
        return 0;
    for (int i = 0; i < 3; ++i) {
        value = change_value[i];
        if (value > 0) {
            if (i == 0)
                return 0;
            if (i == 1)
                return 2;
            if (i == 2)
                return 4;
        }
    }
    return 10;
}

int oCItem::GetProtectionByIndex(oEIndexDamage index) const
{
    return protection[index];
}

int oCItem::GetProtectionByType(oETypeDamage type) const
{
    for (int i = 0; i < 8; ++i) {
        if (type & (1 << i))
            return protection[i];
    }
    return 0;
}

int oCItem::GetProtectionByMode(unsigned long mode) const
{
    int result = 0;
    for (int i = 0; i < 8; ++i) {
        if (mode & (1 << i))
            result += protection[i];
    }
    return result;
}

int oCItem::GetFullProtection() const
{
    int result = 0;
    for (int i = 0; i < 8; ++i)
        result += protection[i];
    return result;
}

void oCItem::CopyDamage(oCItem* other)
{
    if (!other)
        return;
    other->damageTypes = damageTypes;
    other->damageTotal = damageTotal;
    for (int i = 0; i < 8; ++i)
        other->damage[i] = damage[i];
}

int oCItem::GetDamageByIndex(oEIndexDamage index) const
{
    return damage[index];
}

int oCItem::GetDamageByType(oETypeDamage type) const
{
    for (int i = 0; i < 8; ++i) {
        if (type & (1 << i))
            return damage[i];
    }
    return 0;
}

int oCItem::GetDamageByMode(unsigned long mode) const
{
    int result = 0;
    for (int i = 0; i < 8; ++i) {
        if (mode & (1 << i))
            result += damage[i];
    }
    return result;
}

void oCItem::GetDamages(unsigned long* values) const
{
    if (values) {
        for (int i = 0; i < 8; ++i)
            values[i] = damage[i];
    }
}

int oCItem::GetFullDamage() const
{
    int result = 0;
    for (int i = 0; i < 8; ++i)
        result += damage[i];
    return result;
}

unsigned long oCItem::GetDamageTypes() const
{
    return damageTypes;
}

int oCItem::HasDamageType(oETypeDamage type) const
{
    return (damageTypes & type) == (unsigned long)type;
}

int oCItem::IsDeadly() const
{
    if (damageTypes & 2)
        return 0;
    return !(damageTypes & 4);
}

int oCItem::IsOneHandWeapon() const
{
    return (flags & 0x8000) || (flags & 0x4000);
}

int oCItem::MultiSlot() const
{
    if (!(flags & 0x200000) && !(flags & 8) && !(flags & 0x20)) {
        if (flags & 0x4000000)
            return 0;
        if (flags & 2)
            return !(flags & 0x40000000);
        if (!(flags & 4) || (flags & 0x40000000))
            return 0;
    }
    return 1;
}

int oCItem::SplitSlot() const
{
    if (flags & 0x4000000)
        return 0;
    return (flags & 2) || (flags & 4);
}

int oCItem::GetValue() const
{
    float result = (float)(magic_value + value);
    if (max_hitp > 0)
        result *= (float)hitp / max_hitp;
    return (int)std::ceil(result);
}

int oCItem::GetWeight() const
{
    return amount <= 1 ? weight : weight * amount;
}

int oCItem::HasFlag(int flag) const
{
    return (flags & flag) == flag;
}

void oCItem::ClearFlag(int flag)
{
    flags &= ~flag;
}

void oCItem::SetFlag(int flag)
{
    flags |= flag;
}

int oCItem::TwoHanded() const
{
    return (flags & 0x20000) || (flags & 0x10000);
}

int oCItem::IsOwnedByNpc(int npcInstance) const
{
    return npcInstance > 0 && owner == npcInstance;
}

int oCItem::IsOwnedByGuild(int guild) const
{
    return guild > 0 && ownerGuild == guild;
}

int oCItem::IsOwned(int npcInstance, int guild) const
{
    return IsOwnedByNpc(npcInstance) || IsOwnedByGuild(guild);
}

int oCItem::s_bItemEffectEnabled = 1;

int oCItem::GetDisguiseGuild() const
{
    return disguiseGuild;
}

int oCItem::AddManipulation()
{
    if (c_manipulation > 0 && ogame->GetWorldTimer()->GetPassedTime(last_manipulation) > 60.0f)
        c_manipulation = 0;
    last_manipulation = ogame->GetWorldTimer()->GetFullTime();
    return ++c_manipulation;
}

void oCItem::GetSoundMaterial_AM(zCSoundManager::zTSndManMedium& medium,
    oTSndMaterial& material, int slot)
{
    medium = zCSoundManager::zSND_MAN_MEDIUM_ITEM;
    material = GetSoundMaterial();
}

void oCItem::Identify()
{
}

zSTRING oCItem::GetName(int) const
{
    return name;
}

zSTRING oCItem::GetStatus() const
{
    int status = hitp * 100 / max_hitp - 1;
    if (status >= 100)
        status = 9;
    else if (status > 0)
        status /= 10;
    else
        status = 0;
    return parser.GetText("TXT_ATR_HP", status);
}

void oCItem::UseItem()
{
}

zSTRING oCItem::GetVisualChange() const
{
    zSTRING result = visual_change;
    result.Upper();
    int extension = result.Search(".ASC");
    if (extension > 0)
        result.Delete(extension, 4);
    return result;
}

zSTRING oCItem::GetEffectName() const
{
    return effectName;
}

int oCItem::GetStateEffectFunc(int index) const
{
    if (index >= 0 && index < 4 && onState[index] > 0)
        return onState[index];
    return -1;
}

oCItem* oCItem::SplitItem(int splitAmount)
{
    if (splitAmount >= amount)
        return this;
    oCItem* result = (oCItem*)ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM, instanz);
    if (result) {
        result->amount = splitAmount;
        amount -= splitAmount;
        return result;
    }
    return this;
}

void oCItem::RemoveEffect()
{
    if (!effectVob)
        return;
    effectVob->Stop(1);
    effectVob->Release();
    effectVob = 0;
}

void oCItem::InsertEffect()
{
    if (!s_bItemEffectEnabled || effectVob || effectName.IsEmpty())
        return;
    effectVob = oCVisualFX::CreateAndPlay(effectName, this, 0, 1, 0.0f, 0, 0);
    if (effectVob && visual)
        effectVob->SetPFXShapeVisual(visual, 0);
    else if (!effectVob)
        zerr->Message(zSTRING("C: effect ") + effectName + " could not be started for item " + name);
}

void oCItem::ThisVobAddedToWorld(zCWorld* world)
{
    CreateVisual();
    zCVob::ThisVobAddedToWorld(world);
    SetCollDetDyn(1);
    SetCollDetStat(1);
    SetSleeping(1);
    showVisual &= 0xef;
    SetPhysicsEnabled(0);
    if (!world->m_bIsInventoryWorld)
        InsertEffect();
    if (GetRigidBody())
        GetRigidBody()->gravityOn = 1;
}

void oCItem::ThisVobRemovedFromWorld(zCWorld* world)
{
    if (!world->m_bIsInventoryWorld)
        RemoveEffect();
    zCVob::ThisVobRemovedFromWorld(world);
    SetAI(0);
}

void oCItem::RotateForInventory(int rotation)
{
    ResetRotationsLocal();
    if (inv_rotx || inv_roty || inv_rotz) {
        RotateWorldX((float)inv_rotx * rotation);
        RotateWorldY((float)inv_roty * rotation);
        RotateWorldZ((float)inv_rotz * rotation);
        return;
    }
    switch (mainflag) {
    case 1:
        RotateWorldX(-25.0f * rotation);
        RotateWorldZ(-45.0f * rotation);
        break;
    case 2:
        RotateWorldY(-90.0f * rotation);
        if (flags & 0x2000)
            RotateWorldZ(-45.0f * rotation);
        else
            RotateWorldZ(-135.0f * rotation);
        break;
    case 4:
        RotateWorldX(90.0f * rotation);
        RotateWorldZ(45.0f * rotation);
        break;
    case 8:
        RotateWorldY(-135.0f * rotation);
        break;
    case 0x10:
        RotateWorldX(360.0f * rotation);
        break;
    case 0x20:
        RotateWorldY(-90.0f * rotation);
        RotateWorldZ(-35.0f * rotation);
        break;
    case 0x40:
        RotateWorldX(90.0f * rotation);
        break;
    case 0x80:
        RotateWorldY(90.0f * rotation);
        break;
    case 0x200:
        RotateWorldX(-90.0f * rotation);
        break;
    case 0x80000000:
        RotateWorldY(-90.0f * rotation);
        RotateWorldZ(-35.0f * rotation);
        break;
    default:
        break;
    }
}

void oCItem::RotateInInventory()
{
    if (inv_animate)
        RotateLocal(zVEC3{ 0.0f, 1.0f, 0.0f }, ztimer.totalTimeFloat * 0.0199999996f);
}

void oCItem::RenderItem(int x, int y)
{
    RenderItem(ogame->GetGameWorld(), (zCViewBase*)screen, 0.0f);
}

float oCItem::s_fGlobalItemZBiasScale = 1.0f;

void oCItem::RenderItemPlaceCamera(zCCamera* camera, float angle)
{
    oCItem* renderItem = this;
    if (!visual && globalVobTreeNode && globalVobTreeNode->firstChild) {
        oCItem* child = dynamic_cast<oCItem*>(globalVobTreeNode->firstChild->data);
        if (child)
            renderItem = child;
    }
    camera->SetFOV(45.0f);
    zTBBox3D box = renderItem->GetBBox3DLocal();
    zVEC3 center = {
        (box.mins.n[0] + box.maxs.n[0]) * 0.5f,
        (box.mins.n[1] + box.maxs.n[1]) * 0.5f,
        (box.mins.n[2] + box.maxs.n[2]) * 0.5f
    };
    zVEC3 size = {
        (box.maxs.n[0] - box.mins.n[0]) * 0.5f,
        (box.maxs.n[1] - box.mins.n[1]) * 0.5f,
        (box.maxs.n[2] - box.mins.n[2]) * 0.5f
    };
    if (mainflag == 0x80000000) {
        if (flags & 0x10000) {
            center.n[1] -= 4.0f;
            size.n[1] -= 4.0f;
        }
        if (flags & 0x800) {
            center.n[1] -= 0.5f;
            size.n[1] -= 0.5f;
        }
        if (flags & 0x1000000) {
            center.n[1] -= 2.0f;
            size.n[1] -= 2.0f;
        }
    }
    float halfFov = camera->GetFOV() * 0.5f * 0.0174532924f;
    float distance = std::sqrt(size.n[0] * size.n[0] + size.n[1] * size.n[1] +
        size.n[2] * size.n[2]) * std::cos(halfFov) / std::sin(halfFov);
    if (inv_zbias)
        distance *= inv_zbias * 0.00999999978f;
    else {
        if (mainflag == 0x200 || mainflag == 0x20)
            distance *= 1.5f;
        else if (mainflag == 0x80)
            distance *= 1.45000005f;
        else if (mainflag == 0x40)
            distance *= 1.35000002f;
        else if (mainflag == 8 || mainflag == 4 || mainflag == 2)
            distance *= 1.35000002f;
        else if (mainflag == 1)
            distance *= 1.45000005f;
        else if (mainflag == 0x80000000 && (flags & 0x800))
            distance *= 2.0f;
    }
    if (renderItem != this)
        distance *= 2.0f;
    if (s_fGlobalItemZBiasScale != 0.0f)
        distance *= s_fGlobalItemZBiasScale;
    center.n[2] = -std::fabs(distance);
    camera->connectedVob->SetPositionWorld(center);
}

void oCItem::RenderItem(zCWorld* world, zCViewBase* view, float angle)
{
    zCCamera* previousCamera = zCCamera::activeCam;
    zCCamera camera;
    zCVob* cameraVob = new zCVob;
    zCClassDef::ObjectCreated(cameraVob, zCVob::classDef);
    zVEC3 position = { 0.0f, 0.0f, 0.0f };
    SetPositionWorld(position);
    if (angle == 0.0f)
        RotateForInventory(1);
    else
        RotateInInventory();
    groundPoly = 0;
    world->AddVob(this);
    world->AddVob(cameraVob);
    RenderItemPlaceCamera(&camera, angle);
    camera.SetRenderTarget(view);
    camera.SetFarClipZ(2147483647.0f);
    int oldBspMode = world->bspTree.bspTreeMode;
    world->bspTree.bspTreeMode = 0;
    world->GetActiveSkyControler()->SetFillBackground(0);
    zCEventManager::disableEventManagers = 1;
    world->Render(&camera);
    zCEventManager::disableEventManagers = 0;
    world->bspTree.bspTreeMode = oldBspMode;
    world->RemoveVob(cameraVob);
    world->RemoveVobSubtree(this);
    cameraVob->Release();
    zCCamera::activeCam = previousCamera;
}

int oCItem::GetScriptInstance(zSTRING*& instanceName, int& instance)
{
    instanceName = &name;
    instance = instanz;
    return 1;
}

int oCItem::SetByScriptInstance(const zSTRING* instanceName, int instance)
{
    if (!instanceName) {
        InitByScript(instance, 1);
        return 1;
    }
    InitByScript(parser.GetIndex(*instanceName), 1);
    return 1;
}

zSTRING oCItem::GetInstanceName()
{
    int first = 0;
    int second = 0;
    return parser.GetSymbolInfo(instanz, first, second);
}

void oCItem::Archive(zCArchiver& archiver)
{
    zCVisual* savedVisual = 0;
    if (file.Search(".ZEN") < 0) {
        savedVisual = visual;
        if (savedVisual)
            ++savedVisual->refCtr;
        zCVob::SetVisual(0);
    }
    oCVob::Archive(archiver);
    archiver.WriteString(GetInstanceName());
    if (savedVisual) {
        zCVob::SetVisual(savedVisual);
        savedVisual->Release();
    }
    if (archiver.InSaveGame()) {
        archiver.WriteInt(amount);
        archiver.WriteInt(flags);
    }
}

void oCItem::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    name = archiver.ReadString();
    int instance = parser.GetIndex(name);
    if (instance >= 0)
        InitByScript(instance, 1);
    if (archiver.InSaveGame()) {
        amount = archiver.ReadInt();
        flags = archiver.ReadInt();
    }
}

oCAIVobMove* oCItem::GetAIVobMove()
{
    if (!(flags & 0x10000000))
        return oCVob::GetAIVobMove();
    oCAIVobMoveTorch* ai = dynamic_cast<oCAIVobMoveTorch*>(callback_ai);
    if (!ai) {
        ai = new oCAIVobMoveTorch;
        zCClassDef::ObjectCreated(ai, oCAIVobMoveTorch::classDef);
        SetAI(ai);
        ai->Release();
    }
    return ai;
}

int oCItem::Render(zTRenderContext& context)
{
    if (!(flags & 0x800000))
        context.hintLightingFullbright =
            ((itemLightingSwell << 1 ^ context.hintLightingFullbright) & 2) ^
            context.hintLightingFullbright;
    else
        context.hintLightingFullbright &= ~2;
    return oCVob::Render(context);
}

zCObject* oCItem::_CreateNewInstance()
{
    oCItem* item = new oCItem;
    zCClassDef::ObjectCreated(item, classDef);
    return item;
}

void oCItem::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}
