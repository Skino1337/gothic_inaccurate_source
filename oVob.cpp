#include "include/oGame.h"
#include "include/oDialog.h"
#include "include/oFocus.h"
#include "include/oAiShoot.h"

static int vobRuntimeState = -1;
static zSTRING vobRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING vobRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING vobRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING vobRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING vobRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING vobRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING vobRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING vobRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING vobRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING vobRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float vobRuntimeFarDistance = 250000.0f * 0.0166666675f;
static float vobRuntimeNearDistance = vobRuntimeFarDistance * 0.0166666675f;
static float vobRuntimeFarPlaneDistance = 250000.0f * 24.0f;
static zCClassDef oCVobClassDef("oCVob", "zCVob", oCVob::_CreateNewInstance, 0, 0x120, 0, "");
zCClassDef* oCVob::classDef = &oCVobClassDef;
zCList<oCVob> oCVob::debuglist = {};

oCVob::oCVob()
{
}

oCVob::~oCVob()
{
}

zCClassDef* oCVob::_GetClassDef() const
{
    return classDef;
}

void oCVob::GetSoundMaterial_AM(zCSoundManager::zTSndManMedium& medium,
    oTSndMaterial& material, int slot)
{
}

void oCVob::Init()
{
}

void oCVob::ClearDebugList()
{
    debuglist.DeleteList();
}

void oCVob::ShowVobDebugInfo(zCCamera* camera)
{
    for (zCList<oCVob>* node = debuglist.next; node; node = node->next)
        node->data->ShowDebugInfo(camera);
}

int oCVob::GetShowDebug()
{
    for (zCList<oCVob>* node = debuglist.next; node; node = node->next)
    {
        if (node->data == this)
            return 1;
    }
    return 0;
}

void oCVob::ToggleShowDebug()
{
    SetShowDebug(!GetShowDebug());
}

void oCVob::SetShowDebug(int enabled)
{
    if (enabled)
    {
        if (!GetShowDebug())
            debuglist.Insert(this);
    }
    else
        debuglist.Remove(this);
}

void oCVob::ShowDebugInfo(zCCamera* camera)
{
    zVEC3 center = trafoObjToWorld.GetTranslation();
    center.n[1] += (bbox3D.maxs.n[1] - bbox3D.mins.n[1]) * 0.5f;
    float projectedX;
    float projectedY;
    camera->Project(&center, projectedX, projectedY);
    int x = screen->anx(projectedX) - 30;
    int y = screen->any(projectedY);
    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (model)
    {
        for (int i = 0; i < model->numActiveAnis; ++i)
        {
            zCModelAniActive* active = &model->activeAniList[i];
            if (!active->isFadingOut)
                continue;
            screen->Print(x, y, active->protoAni->aniName + " (" +
                zSTRING((long)active->actFrame) + ")");
            y += screen->FontY();
            break;
        }
    }
    screen->Print(x, y, "CDS:" + zSTRING((showVisual >> 7) & 1) +
        "  CDD:" + zSTRING((showVisual >> 6) & 1));
    y += screen->FontY();
    screen->Print(x, y, "GR:" + zSTRING(GetRigidBody()->gravityOn & 1) +
        "  PHY:" + zSTRING((showVisual >> 3) & 1));
    y += screen->FontY();
    zSTRING aiName = "-None-";
    if (callback_ai && callback_ai->_GetClassDef())
        aiName = callback_ai->_GetClassDef()->className;
    screen->Print(x, y, "SL:" + zSTRING((sleepingMode & 3) == 0) + "  AI:" + aiName);
    y += screen->FontY();
    screen->Print(x, y, "REF:" + zSTRING(refCtr));
    oCMobInter* mob = dynamic_cast<oCMobInter*>(this);
    if (mob)
    {
        y += screen->FontY();
        screen->Print(x, y, "MOB:" + mob->GetScemeName() + " SC:" + zSTRING(mob->state));
    }
}

int oCVob::HasNpcEnoughSpace(zVEC3& position)
{
    zCArray<zCVob*> ignoreVobs;
    ignoreVobs.InsertEnd(this);
    zVEC3 extent = bbox3D.maxs - bbox3D.mins;
    for (int axis = 0; axis < 3; ++axis)
    {
        zVEC3 direction(0.0f, 0.0f, 0.0f);
        direction.n[axis] = -extent.n[axis];
        zVEC3 origin = position - direction;
        while (homeWorld->TraceRayNearestHit(origin, direction, &ignoreVobs, 5))
        {
            zCVob* foundVob = homeWorld->traceRayReport.foundVob;
            if (!foundVob || dynamic_cast<oCNpc*>(foundVob))
                return 0;
            zTTraceRayReport report;
            if (foundVob->TraceRay(origin, direction, 0x80, report))
                return 0;
            ignoreVobs.InsertEnd(foundVob);
        }
    }
    zVEC3 origin = position + zVEC3(0.0f, 200.0f, 0.0f);
    zVEC3 direction(0.0f, -1000.0f, 0.0f);
    if (homeWorld->TraceRayNearestHit(origin, direction, this, 5))
    {
        zCVob* foundVob = homeWorld->traceRayReport.foundVob;
        if (foundVob && dynamic_cast<oCNpc*>(foundVob))
            return 0;
    }
    return 1;
}

int oCVob::HasEnoughSpace(zVEC3& position)
{
    if (!homeWorld)
    {
        zerr.Warning("U: OVOB: Space-Test failed : Vob not in World.");
        return 0;
    }
    zCCollisionObjectClass* oldClass = m_poCollisionObjectClass;
    zMAT4 oldTrafo = trafoObjToWorld;
    SetCollisionClass(&zCCollObjectComplex::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(0);
    zMAT4 newTrafo = oldTrafo;
    newTrafo.v[0].n[3] = position.n[0];
    newTrafo.v[1].n[3] = position.n[1];
    newTrafo.v[2].n[3] = position.n[2];
    int collision = DetectCollision(&newTrafo);
    SetCollDetStat(0);
    SetCollDetDyn(0);
    SetTrafoObjToWorld(oldTrafo);
    SetCollisionClass(oldClass);
    return !collision;
}

int oCVob::SearchNpcPosition(zVEC3& position)
{
    if (!homeWorld)
    {
        zerr.Warning("U: OVOB: SearchNpcPos failed : HomeWorld == NULL.");
        return 0;
    }
    float radius = bbox3D.maxs.n[0] - bbox3D.mins.n[0];
    float depth = bbox3D.maxs.n[2] - bbox3D.mins.n[2];
    if (depth > radius)
        radius = depth;
    radius *= 1.5f;
    const int offsets[9][2] = {
        { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 },
        { 1, 1 }, { 1, -1 }, { -1, -1 }, { -1, 1 }
    };
    for (int ring = 0; ring < 6; ++ring)
    {
        float distance = radius * ring;
        for (int direction = 0; direction < 9; ++direction)
        {
            zVEC3 candidate = position + zVEC3(offsets[direction][0] * distance, 0.0f,
                offsets[direction][1] * distance);
            int found;
            if (dynamic_cast<oCNpc*>(this))
                found = GetFloorPosition(candidate) && HasNpcEnoughSpace(candidate);
            else
                found = HasEnoughSpace(candidate);
            if (found)
            {
                position = candidate;
                return 1;
            }
        }
    }
    zerr.Warning(GetInstanceName() + "U: OVOB: SearchNpcPos failed : Not enough space.");
    return 0;
}

int oCVob::SearchFreePosition(zVEC3& position, zCVob* originVob)
{
    if (!originVob || !originVob->homeWorld)
    {
        zerr.Warning("U: OVOB: SearchPos failed : Origin or HomeWorld = NULL.");
        return 0;
    }
    zVEC3 initial = position;
    zVEC3 origin = originVob->trafoObjToWorld.GetTranslation();
    zVEC3 atVector(originVob->trafoObjToWorld.v[0].n[2],
        originVob->trafoObjToWorld.v[1].n[2], originVob->trafoObjToWorld.v[2].n[2]);
    zVEC3 extent = (bbox3D.maxs - bbox3D.mins) * 0.5f;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (extent.n[axis] < 10.0f)
            extent.n[axis] = 10.0f;
    }
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int sign = -1; sign <= 1; sign += 2)
        {
            for (int distance = 0; distance <= 4; ++distance)
            {
                zVEC3 candidate = initial;
                for (int step = 0; step < 7; ++step)
                {
                    zVEC3 direction = candidate - origin;
                    int flags = pass ? 0x22 : 0;
                    if (!originVob->homeWorld->TraceRayNearestHit(origin, direction, originVob,
                        flags) && HasEnoughSpace(candidate))
                    {
                        position = candidate;
                        return 1;
                    }
                    float scale = distance ? extent.n[0] * distance : 10.0f;
                    candidate += atVector * (sign * scale);
                }
            }
        }
    }
    position = initial;
    return 0;
}

void oCVob::SetOnFloor(zVEC3& position)
{
    if (GetFloorPosition(position))
        SetPositionWorld(position);
}

int oCVob::GetFloorPosition(zVEC3& position)
{
    zCVob* floorVob;
    return GetFloorPositionWithVob(position, floorVob);
}

int oCVob::GetFloorPositionWithVob(zVEC3& position, zCVob*& floorVob)
{
    floorVob = NULL;
    if (!homeWorld)
        return 0;
    zVEC3 origin = position;
    origin.n[1] -= bbox3D.mins.n[1];
    zVEC3 direction(0.0f, -1000.0f, 0.0f);
    if (!homeWorld->TraceRayNearestHit(origin, direction, this, 0x21))
        return 0;
    zTTraceRayReport& report = homeWorld->traceRayReport;
    if (!report.foundPoly && !report.foundVob)
        return 0;
    position = report.foundIntersection;
    position.n[1] += bbox3D.mins.n[1] + 4.0f;
    floorVob = report.foundVob;
    return 1;
}

int oCVob::GetInstance()
{
    return -1;
}

zSTRING oCVob::GetInstanceName()
{
    return zSTRING();
}

void oCVob::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
}

void oCVob::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
}

void oCVob::StartParticleEffects(zCTree<zCVob>* node)
{
}

int oCVob::Render(zTRenderContext& context)
{
    static zCVob* lastFocusVob = NULL;
    static float highlightAlpha = 0.0f;
    context.hintLightingFullbright &= ~4;
    zCVob* focusVob = NULL;
    if (oCNpc::player && !oCNpc::player->IsDead() && !oCNpc::player->IsUnconscious())
        focusVob = oCNpc::player->GetFocusVob();
    if (lastFocusVob != focusVob)
        highlightAlpha = 0.0f;
    int highlight = 0;
    if (focusVob == this)
    {
        if (dynamic_cast<oCNpc*>(this))
            highlight = oCNpcFocus::s_iHightlightMeleeFocus >= 2 && oCNpcFocus::isHighLighted;
        else
        {
            oCMobInter* mob = dynamic_cast<oCMobInter*>(this);
            highlight = oCNpcFocus::s_bHighlightInteractFocus
                && (!mob || !mob->GetScemeName().IsEmpty());
        }
    }
    if (highlight)
    {
        context.hintLightingFullbright &= ~2;
        highlightAlpha += ztimer.frameTimeFloat * 0.003f;
        if (highlightAlpha < 0.0f)
            highlightAlpha = 0.0f;
        else if (highlightAlpha > 1.0f)
            highlightAlpha = 1.0f;
        context.hintLightingFullbright |= 4;
        context.hintLightingHighlightColor.b = 255;
        context.hintLightingHighlightColor.g = 255;
        context.hintLightingHighlightColor.r = 255;
        context.hintLightingHighlightColor.a = (unsigned char)(highlightAlpha * 255.0f);
    }
    else if (!focusVob)
        highlightAlpha = 0.0f;
    lastFocusVob = focusVob;
    return zCVob::Render(context);
}

oCAIVobMove* oCVob::GetAIVobMove()
{
    oCAIVobMove* ai = dynamic_cast<oCAIVobMove*>(callback_ai);
    if (!ai)
    {
        ai = new oCAIVobMove;
        zCClassDef::ObjectCreated(ai, oCAIVobMove::classDef);
        SetAI(ai);
        ai->Release();
    }
    return ai;
}

static zCClassDef oCTouchDamageClassDef("oCTouchDamage", "zCTouchDamage",
    oCTouchDamage::_CreateNewInstance, 0, 0x134, 0, "");
zCClassDef* oCTouchDamage::classDef = &oCTouchDamageClassDef;

const char* oCTouchDamage::GetDamageTypeArcEnum(unsigned long index)
{
    static const char* names[8] = {
        "Barrier", "Blunt", "Edge", "Fire", "Fly", "Magic", "Point", "Fall"
    };
    if (index > 7)
        return NULL;
    return names[index];
}

zCObject* oCTouchDamage::_CreateNewInstance()
{
    oCTouchDamage* touchDamage = new oCTouchDamage;
    zCClassDef::ObjectCreated(touchDamage, classDef);
    return touchDamage;
}

void oCTouchDamage::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCTouchDamage::_GetClassDef() const
{
    return classDef;
}

oCTouchDamage::~oCTouchDamage()
{
}
