#include "include/oVisFx.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zParticle.h"
#include "include/zSound.h"
#include "include/zMathTools.h"
#include "include/zAlgebra.h"
#include "include/oBarrier.h"
#include "include/zMorphMesh.h"
#include <math.h>

extern zCOption* zgameoptions;
extern zCParser* parserVisualFX;
static int showTarget;
static int visualFXState = -1;
static zSTRING visualFXListBoxLoadGame = "MENU_ITEM_LISTBOX_LOADGAME";
static zSTRING visualFXListBoxSaveGame = "MENU_ITEM_LISTBOX_SAVEGAME";
static zSTRING visualFXOptControls = "MENU_OPT_CONTROLS";
static zSTRING visualFXOptControlsExtKeys = "MENU_OPT_CONTROLS_EXTKEYS";
static int visualFXRuntimeState = -1;
static zSTRING visualFXParserPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING visualFXParserPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING visualFXParserPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING visualFXParserPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING visualFXParserPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING visualFXParserPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING visualFXParserPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING visualFXParserPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING visualFXParserPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING visualFXParserPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING visualFXMenuStatus = "MENU_STATUS";
static zSTRING visualFXMenuLog = "MENU_LOG";
static zSTRING visualFXMenuPlayerName = "MENU_ITEM_PLAYERNAME";
static zSTRING visualFXMenuPlayerGuild = "MENU_ITEM_PLAYERGUILD";
static zSTRING visualFXMenuPlayerMagicalGuild = "MENU_ITEM_PLAYER_MAGICALGUILD";
static zSTRING visualFXMenuAttribute = "MENU_ITEM_ATTRIBUTE";
static zSTRING visualFXMenuLevel = "MENU_ITEM_LEVEL";
static zSTRING visualFXMenuLevelNext = "MENU_ITEM_LEVEL_NEXT";
static zSTRING visualFXMenuExp = "MENU_ITEM_EXP";
static zSTRING visualFXMenuLearn = "MENU_ITEM_LEARN";
static zSTRING visualFXMenuTalent = "MENU_ITEM_TALENT";
static zSTRING visualFXMenuArmor = "MENU_ITEM_ARMOR";
static zSTRING visualFXMenuMissionsAct = "MENU_ITEM_LIST_MISSIONS_ACT";
static zSTRING visualFXMenuMissionsOld = "MENU_ITEM_LIST_MISSIONS_OLD";
static zSTRING visualFXMenuMissionsFailed = "MENU_ITEM_LIST_MISSIONS_FAILED";
static zSTRING visualFXMenuListLog = "MENU_ITEM_LIST_LOG";
static zSTRING visualFXMenuContentViewer = "MENU_ITEM_CONTENT_VIEWER";
static zSTRING visualFXMenuDay = "MENU_ITEM_DAY";
static zSTRING visualFXMenuTime = "MENU_ITEM_TIME";
static int visualFXListState = -1;
static zSTRING visualFXListPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING visualFXListPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING visualFXListPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING visualFXListPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING visualFXListPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING visualFXListPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING visualFXListPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING visualFXListPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING visualFXListPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING visualFXListPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float visualFXHalfValue = 0.5f;
zCArray<oCVisualFX> s_worldVisFXList;
static unsigned int visualFXStateArray[3];
static int visualFXMultiTargetState = -1;
static zSTRING visualFXMultiPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING visualFXMultiPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING visualFXMultiPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING visualFXMultiPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING visualFXMultiPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING visualFXMultiPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING visualFXMultiPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING visualFXMultiPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING visualFXMultiPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING visualFXMultiPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static int visualFXMultiTargetClassDefState = -1;

void oCTrajectory::Calc()
{
    if (keyList.numInArray < 2)
        return;

    if (mode & 8)
    {
        spl = new zCKBSpline;
        spl->InitVars();

        zCPositionKey* first = new zCPositionKey;
        zCPositionKey* last = new zCPositionKey;
        zMAT4 matrix = {};
        for (int i = 0; i < 4; ++i)
            matrix.m[i][i] = 1.0f;
        keyList[0]->rot.QuatToMatrix4(matrix);
        matrix.SetTranslation(keyList[0]->p);
        first->SetMat(matrix);

        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                matrix.m[i][j] = i == j ? 1.0f : 0.0f;
        keyList[keyList.numInArray - 1]->rot.QuatToMatrix4(matrix);
        matrix.SetTranslation(keyList[keyList.numInArray - 1]->p);
        last->SetMat(matrix);

        keyList.Insert(first, 0);
        keyList.InsertEnd(last);
        for (int i = 0; i < keyList.numInArray; ++i)
            keyList[i]->t = (float)i;
        spl->Initialize(keyList, 1);
        length = spl->totalLength;
        return;
    }

    if (mode & 4)
    {
        for (int i = 1; i < keyList.numInArray; ++i)
        {
            zVEC3 delta = keyList[i]->p - keyList[i - 1]->p;
            length += sqrt(delta.Length2());
        }
    }
    else if (mode & 2)
    {
        zVEC3 delta = keyList[1]->p - keyList[0]->p;
        length = sqrt(delta.Length2());
    }
}

zMAT4& zMAT4::SetTranslation(const zVEC3& translation)
{
    m[0][3] = translation.n[0];
    m[1][3] = translation.n[1];
    m[2][3] = translation.n[2];
    return *this;
}

zVEC3 zMAT4::GetTranslation() const
{
    return zVEC3(m[0][3], m[1][3], m[2][3]);
}

zMAT4::zMAT4(const zMAT4& other)
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = other.m[i][j];
}

zVEC3 zMAT4::GetUpVector() const
{
    return zVEC3(m[0][1], m[1][1], m[2][1]);
}

zVEC3 zMAT4::GetRightVector() const
{
    return zVEC3(m[0][0], m[1][0], m[2][0]);
}

float zVEC3::LengthApprox() const
{
    float x = fabs(n[0]);
    float y = fabs(n[1]);
    float z = fabs(n[2]);
    if (x < y)
    {
        float swap = x;
        x = y;
        y = swap;
    }
    if (x < z)
    {
        float swap = x;
        x = z;
        z = swap;
    }
    return x * 0.9375f + (y + z) * 0.375f;
}

zVEC3& zVEC3::Normalize()
{
    float inverseLength = 1.0f / sqrt(Length2());
    n[0] *= inverseLength;
    n[1] *= inverseLength;
    n[2] *= inverseLength;
    return *this;
}

int operator==(const zVEC3& first, const zVEC3& second)
{
    return first.n[0] == second.n[0] && first.n[1] == second.n[1] && first.n[2] == second.n[2];
}

void zCPositionKey::SetMat(const zMAT4& matrix)
{
    rot.Matrix4ToQuat(matrix);
    p = matrix.GetTranslation();
}

void oCTrajectory::InsertKey(zMAT4& matrix)
{
    zCPositionKey* key = new zCPositionKey;
    key->t = 0.0f;
    key->SetMat(matrix);
    keyList.InsertEnd(key);
}

zMAT4 oCTrajectory::GetKey(int index)
{
    zMAT4 matrix = {};
    for (int i = 0; i < 4; ++i)
        matrix.m[i][i] = 1.0f;
    keyList[index]->rot.QuatToMatrix4(matrix);
    matrix.SetTranslation(keyList[index]->p);
    return matrix;
}

int oCTrajectory::GetNumKeys()
{
    return keyList.numInArray;
}

void zCKBSpline::InitVars()
{
    totalLength = 0.0f;
    numPolynomials = 0;
    polynom = NULL;
    length = NULL;
    lastKey = 0;
    deleteKeysWhenDone = 1;
}

void oCTrajectory::Changed()
{
    if (spl)
        spl->Changed();
}

zMAT4 oCTrajectory::Interpolate(float time)
{
    res = zMAT4();
    for (int i = 0; i < 4; ++i)
        res.m[i][i] = 1.0f;
    if (mode & 8)
    {
        if (spl)
        {
            float start = spl->polynom->tmin;
            float range = spl->polynom[spl->numPolynomials - 1].tmax - start;
            spl->Interpolate0(start + range * time / spl->totalLength, res, 1);
            lastKey = spl->lastKey;
        }
        return res;
    }
    if (!keyList.numInArray)
        return res;
    if (time >= length)
    {
        res.SetTranslation(keyList[keyList.numInArray - 1]->p);
        return res;
    }

    float segmentStart = 0.0f;
    for (int i = 1; i < keyList.numInArray; ++i)
    {
        zVEC3 direction = keyList[i]->p - keyList[i - 1]->p;
        float segmentLength = sqrt(direction.Length2());
        if (segmentStart + segmentLength >= time)
        {
            zVEC3 heading = direction * (1.0f / segmentLength);
            zVEC3 position = keyList[i - 1]->p + direction * ((time - segmentStart) / segmentLength);
            res = GetPoseOfHeading(position, heading);
            lastKey = i - 1;
            return res;
        }
        segmentStart += segmentLength;
    }

    if (keyList.numInArray > 1)
    {
        int index = keyList.numInArray;
        zVEC3 direction = keyList[index - 2]->p - keyList[index - 1]->p;
        float segmentLength = sqrt(direction.Length2());
        zVEC3 heading = direction * (1.0f / segmentLength);
        zVEC3 position = keyList[index - 2]->p + direction * ((time - segmentStart) / segmentLength);
        res = GetPoseOfHeading(position, heading);
        lastKey = index - 2;
    }
    return res;
}

zMAT4 oCTrajectory::InterpolateN(float time)
{
    if (!(mode & 8))
        return Interpolate(length * time);
    if (spl)
    {
        float start = spl->polynom->tmin;
        float range = spl->polynom[spl->numPolynomials - 1].tmax - start;
        spl->Interpolate0(start + range * time, res, 1);
        lastKey = spl->lastKey;
    }
    return res;
}

void oCTrajectory::Draw()
{
    if (mode & 8)
    {
        spl->Draw(zCOLOR(255, 255, 255, 255), 1.0f);
        return;
    }

    zVEC3 previous = zCCamera::activeCam->camMatrix * keyList[0]->p;
    if (keyList.numInArray <= 1)
        return;

    for (int i = 1; i < keyList.numInArray; ++i)
    {
        float previousZ = previous.n[2];
        zVEC3 current = zCCamera::activeCam->camMatrix * keyList[i]->p;
        Alg_ClipAtZ0(previous, current);

        float previousInverseZ = 1.0f / previousZ;
        float currentInverseZ = 1.0f / current.n[2];
        int previousX = (int)(previous.n[0] * zCCamera::activeCam->viewDistanceX * previousInverseZ +
            zCCamera::activeCam->vpData.xcenter);
        int previousY = (int)(zCCamera::activeCam->vpData.ycenter -
            previous.n[1] * zCCamera::activeCam->viewDistanceY * previousInverseZ);
        int currentX = (int)(current.n[0] * zCCamera::activeCam->viewDistanceX * currentInverseZ +
            zCCamera::activeCam->vpData.xcenter);
        int currentY = (int)(zCCamera::activeCam->vpData.ycenter -
            current.n[1] * zCCamera::activeCam->viewDistanceY * currentInverseZ);

        if (previousZ >= 1.0f && current.n[2] >= 1.0f &&
            screen->ClipLine(previousX, previousY, currentX, currentY))
            zrenderer->DrawLineZ(previousX, previousY, previousInverseZ, currentX, currentY,
                currentInverseZ, zCOLOR(255, 255, 255, 255));
        previous = current;
    }
}

void oCTrajectory::Clear()
{
    if (spl)
    {
        delete spl;
        spl = NULL;
    }
    else
    {
        for (int i = 0; i < keyList.numInArray; ++i)
            delete keyList[i];
    }
    delete[] keyList.parray;
    keyList.parray = NULL;
    keyList.numAlloc = 0;
    keyList.numInArray = 0;
    length = 0.0f;
}

zCKBSpline::~zCKBSpline()
{
    Terminate();
}

zVEC3 oCTrajectory::RecalcByColl(const zVEC3& start, const zVEC3& end,
    const zVEC3& direction, int flags)
{
    if (keyList.numInArray < 2)
        return direction * -1.0f;

    zVEC3 normal = direction;
    if (!flags)
        normal = direction * -1.0f;
    float normalLength = sqrt(normal.Length2());
    if (normalLength == 0.0f)
    {
        zerr->Report(zERR_TYPE_WARN, 0,
            "C: oCTrajectory::RecalcByColl(): collision normal is (0/0/0)", 0, 0, 0x14F, 0, 0);
        return zVEC3(0.0f, 1.0f, 0.0f);
    }

    zVEC3 velocity = end - start;
    float dot = velocity.n[0] * normal.n[0] + velocity.n[1] * normal.n[1] +
        velocity.n[2] * normal.n[2];
    zVEC3 reflected = velocity - normal * (2.0f * dot);

    zMAT4 pose = GetPoseOfHeading(start, normal);
    zCPositionKey* key = new zCPositionKey;
    key->rot.Matrix4ToQuat(pose);
    key->p = pose.GetTranslation();
    key->t = (float)(lastKey + 1);
    keyList.Insert(key, lastKey + 1);
    for (int i = lastKey + 2; i < keyList.numInArray; ++i)
        keyList[i]->t += 1.0f;

    zCPositionKey* nextKey = keyList[lastKey + 2];
    float nextDistance = sqrt((nextKey->p - start).Length2());
    zMAT4 nextPose = GetPoseOfHeading(start + normal * nextDistance, normal);
    nextKey->rot.Matrix4ToQuat(nextPose);
    nextKey->p = nextPose.GetTranslation();

    if (spl)
    {
        delete spl;
        spl = new zCKBSpline;
        spl->InitVars();
        spl->Initialize(keyList, 1);
    }
    return reflected;
}

zVEC3 operator-(const zVEC3& first, const zVEC3& second)
{
    return zVEC3(first.n[0] - second.n[0], first.n[1] - second.n[1], first.n[2] - second.n[2]);
}

float operator*(const zVEC3& first, const zVEC3& second)
{
    return first.n[0] * second.n[0] + first.n[1] * second.n[1] + first.n[2] * second.n[2];
}

void oCTrajectory::ApplyGrav(const float& gravity)
{
    if (gravity == 0.0f)
        return;
    for (int i = 0; i < keyList.numInArray; ++i)
        keyList[i]->p.n[1] += gravity;
    if (spl)
        spl->Changed();
}

void oCTrajectory::SetByList(zCArray<zCVob*> vobs)
{
    Clear();
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        zCPositionKey* key = new zCPositionKey;
        key->t = 0.0f;
        key->rot.Matrix4ToQuat(vobs[i]->trafoObjToWorld);
        key->p = vobs[i]->GetPositionWorld();
        keyList.InsertEnd(key);
    }
    if (spl)
        spl->Changed();
}

int oCTrajectory::VobCross(const zCVob* vob)
{
    for (int i = 1; i < keyList.numInArray; ++i)
    {
        zVEC3 direction = keyList[i]->p - keyList[i - 1]->p;
        if (vob->homeWorld->TraceRayFirstHit(keyList[i - 1]->p, direction, NULL, 4) &&
            vob->homeWorld->traceRayReport.foundVob == vob)
            return i;
    }
    return -1;
}

oCEmitterKey::oCEmitterKey()
{
    targetPos = zVEC3(0.0f, 0.0f, 0.0f);
    pfx_flyGravity = zVEC3(0.0f, 0.0f, 0.0f);
    pfx_shpOffsetVec = zVEC3(0.0f, 0.0f, 0.0f);
    pfx_dirModeTargetPos = zVEC3(0.0f, 0.0f, 0.0f);
    emSelfRotVel = zVEC3(0.0f, 0.0f, 0.0f);
    visSizeScale = 0.0f;
    scaleDuration = 1.0f;
    pfx_ppsValue = 0.0f;
    pfx_ppsIsSmoothChg = 0;
    pfx_ppsIsLoopingChg = 0;
    pfx_scTime = 0.0f;
    pfx_shpDistribWalkSpeed = 0.0f;
    pfx_velAvg = 0.0f;
    pfx_lspPartAvg = 0.0f;
    pfx_visAlphaStart = 0.0f;
    pfx_shpIsVolumeChg = 0;
    pfx_shpScaleFPS = 0.0f;
    lightRange = 0.0f;
    emTrjEaseVel = 0.0f;
    emFlyGravity = 0.0f;
    vob = NULL;
    sfx = NULL;
    sfxHnd = 0;
    sfxIsAmbient = 0;
    emCheckCollision = 0;
    pfx_shpDistribType = 0;
    pfx_dirMode = 0;
    pfx_dirFOR = 0;
    pfx_dirModeTargetFOR = 0;
    emTrjMode = 0;
    emFXLifeSpan = 0.0f;
}

oCEmitterKey::~oCEmitterKey()
{
    if (sfx)
    {
        if (sfxHnd && zsound->IsSoundActive(sfxHnd))
            zsound->StopSound(sfxHnd);
        sfx->Release();
        sfx = NULL;
    }
}

void oCEmitterKey::SetByScript(const zSTRING& instanceName)
{
    zSTRING upperName = instanceName;
    upperName.Upper();
    if (oCVisualFX::fxParser->GetSymbol(upperName))
    {
        oCVisualFX::fxParser->CreateInstance(upperName, this);
        ParseStrings();
        name = upperName;
        if (!sfxID.IsEmpty())
            sfx = zsound->LoadSoundFXScript(sfxID);
    }
    else
    {
        zerr->Report(zERR_TYPE_FAULT, 0,
            "C:oVisFX.cpp(oCEmitterKey::SetByScript):Unknown ID: " + instanceName,
            1, 0, 0x1D2, 0, 0);
        ParseStrings();
        name = "";
    }
}

void oCEmitterKey::ParseStrings()
{
    emTrjMode = 0;
    pfx_flyGravity = zCParticleEmitter::String2Vec3(pfx_flyGravity_S);
    pfx_shpOffsetVec = zCParticleEmitter::String2Vec3(pfx_shpOffsetVec_S);

    pfx_dirMode_S.Upper();
    if (pfx_dirMode_S.Search("DIR", 1) >= 0)
        pfx_dirMode = 1;
    else if (pfx_dirMode_S.Search("TARGET", 1) >= 0)
        pfx_dirMode = 2;
    else if (pfx_dirMode_S.Search("MESH", 1) >= 0)
        pfx_dirMode = 3;
    else
        pfx_dirMode = 0;

    pfx_dirModeTargetFOR_S.Upper();
    pfx_dirModeTargetFOR = pfx_dirModeTargetFOR_S.Search("ORIGIN", 1) >= 0 ? 1 : 0;
    pfx_dirModeTargetPos = zCParticleEmitter::String2Vec3(pfx_dirModeTargetPos_S);
    emSelfRotVel = zCParticleEmitter::String2Vec3(emSelfRotVel_S);

    emTrjMode_S.Upper();
    if (emTrjMode_S.Search("FIXED", 1) >= 0)
        emTrjMode |= 1;
    if (emTrjMode_S.Search("TARGET", 1) >= 0)
        emTrjMode |= 2;
    if (emTrjMode_S.Search("LINE", 1) >= 0)
        emTrjMode |= 4;
    if (emTrjMode_S.Search("SPLINE", 1) >= 0)
        emTrjMode |= 8;
    if (emTrjMode_S.Search("RANDOM", 1) >= 0)
        emTrjMode |= 16;
    if (emTrjMode_S.Search("FOLLOW", 1) >= 0)
        emTrjMode |= 64;
    if (emTrjMode_S.Search("MISSILE", 1) >= 0)
        emTrjMode |= rand() / 32768.0f < 0.7f ? 2 : 128;
}

void oCVisualFX::SetupAIForAllFXInWorld()
{
    if (!zCCamera::activeCam)
        return;
    for (int i = 0; i < classDef->objectList.numInArray; ++i)
    {
        oCVisualFX* fx = dynamic_cast<oCVisualFX*>(classDef->objectList[i]);
        if (!fx)
            continue;
        if (!(fx->aiMode & 1))
        {
            fx->SetSleeping(1);
            fx->aiTimer = -1.0f;
            continue;
        }
        zVEC3 position = fx->GetPositionWorld();
        zVEC3 delta = position - zCCamera::activeCamPos;
        if (position.Length2() == 0.0f || delta.Length2() >= 36000000.0f)
        {
            fx->SetSleeping(1);
            fx->aiTimer = -1.0f;
        }
        else if (fx->aiTimer == -1.0f)
            fx->aiTimer = 5.0f;
        else if (fx->aiTimer <= 0.0f)
            fx->SetSleeping(0);
    }
}

void oCVisualFX::RemoveInstancesOfClass(zCClassDef& definition, int)
{
    for (int i = 0; i < definition.objectList.numInArray; ++i)
    {
        zCVob* vob = dynamic_cast<zCVob*>(definition.objectList[i]);
        if (vob && vob->homeWorld)
        {
            vob->RemoveVobFromWorld();
            vob->Release();
        }
    }
}

void oCVisualFX::PreSaveGameProcessing(int)
{
}

void oCVisualFX::PostSaveGameProcessing()
{
}

oCVisualFX::oCVisualFX()
{
    zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: construct new oCVisualFX", 10, 0, 0x254, 0, 0);
    InitValues();
    SetAI(ai);
}

oCTrajectory::~oCTrajectory()
{
    Clear();
}

zCClassDef* oCVisualFX::_GetClassDef() const
{
    return classDef;
}

zCVob* oCVisualFX::GetOrigin() const
{
    return origin;
}

zCVob* oCVisualFX::GetTarget() const
{
    return target;
}

zCVob* oCVisualFX::GetInflictor() const
{
    return inflictor;
}

void oCVisualFX::Play(float, const zMAT4*, const zMAT4*)
{
}

void oCVisualFX::SetDuration(float duration)
{
    emFXLifeSpan = duration;
}

void oCVisualFX::GetCollisionCandidates(zCArray<zCVob*>& candidates)
{
    candidates = allowedCollisionVobList;
}

int oCVisualFX::GetNumCollisionCandidates()
{
    return allowedCollisionVobList.numInArray;
}

void oCVisualFX::SetDamage(float value)
{
    damage = value;
}

void oCVisualFX::SetDamageType(int type)
{
    damageType = type;
}

float oCVisualFX::GetDamage() const
{
    return damage;
}

int oCVisualFX::GetDamageType() const
{
    return damageType;
}

int oCVisualFX::IsASpell()
{
    return sendAssessMagic;
}

void oCVisualFX::SetSpellType(int type)
{
    spellType = type;
}

int oCVisualFX::GetSpellType() const
{
    return spellType;
}

void oCVisualFX::SetSpellCat(int category)
{
    spellCat = category;
}

int oCVisualFX::GetSpellCat() const
{
    return spellCat;
}

int oCVisualFX::GetSpellTargetTypes() const
{
    return spellTargetTypes;
}

void oCVisualFX::SetSpellTargetTypes(int types)
{
    spellTargetTypes = types;
}

int oCVisualFX::GetIsProjectile()
{
    return bIsProjectile;
}

void oCVisualFX::SetIsProjectile(int projectile)
{
    bIsProjectile = projectile;
}

oCVisualFX::~oCVisualFX()
{
    EndEffect(0);
    ReleasePFXMesh();

    for (int i = 0; i < emKeyList.numInArray; ++i)
        delete emKeyList[i];
    emKeyList.DeleteList();

    if (showTarget && target)
        target->showVisual &= ~2;
    DisposeHierachy();

    for (int i = 0; i < allowedCollisionVobList.numInArray; ++i)
        allowedCollisionVobList[i]->Release();
    allowedCollisionVobList.DeleteList();
    for (int i = 0; i < collidedVobs.numInArray; ++i)
        collidedVobs[i]->Release();
    collidedVobs.DeleteList();
    for (int i = 0; i < queuedCollisions.numInArray; ++i)
    {
        if (queuedCollisions[i].foundVob)
        {
            queuedCollisions[i].foundVob->Release();
            queuedCollisions[i].foundVob = NULL;
        }
    }
    queuedCollisions.DeleteList();
    for (int i = ignoreVobList.numInArray - 1; i >= 0; --i)
    {
        zCVob* vob = ignoreVobList[i];
        ignoreVobList.RemoveOrderIndex(i);
        if (vob)
            vob->Release();
    }
    for (int i = vobList.numInArray - 1; i >= 0; --i)
    {
        zCVob* vob = vobList[i];
        vobList.RemoveOrderIndex(i);
        if (vob)
            vob->Release();
    }

    delete fxBackup;
    fxBackup = NULL;
    CleanUpCriticalFX();
    StopInvestFX();

    if (origin && origin->GetObjectName() == "_VFX" && origin->homeWorld)
        origin->RemoveVobFromWorld();
    SetTarget(NULL, 0);
    SetInflictor(NULL);
    SetOrigin(NULL, 0);

    if (actFX == this)
        actFX = NULL;
    if (ai)
    {
        ai->Release();
        ai = NULL;
    }
}

void oCVisualFX::CleanUpCriticalFX()
{
    if (light)
    {
        light->RemoveVobFromWorld();
        light->Release();
        light = NULL;
    }
    if (earthQuake)
    {
        if (zCCamera::activeCam)
            earthQuake->OnUntrigger(NULL, NULL);
        earthQuake->RemoveVobFromWorld();
        earthQuake->Release();
        earthQuake = NULL;
    }
    if (screenFX)
    {
        screenFX->StartEffect(1.0f, zCOLOR(0, 0, 0, 0), 0, screenFX->blend);
        screenFX->RemoveVobFromWorld();
        screenFX->Release();
        screenFX = NULL;
    }
    if (sfx)
    {
        sfx->Release();
        if (sfxHnd)
            zsound->StopSound(sfxHnd);
        sfx = NULL;
    }
}

void oCVisualFX::CreateHierachy()
{
    if (emFXCreate_S.IsEmpty())
        return;
    emFXCreate_S.Upper();
    if (!fxParser->GetSymbol(emFXCreate_S))
        return;

    oCVisualFX* child = new oCVisualFX;
    child->SetSpellType(GetSpellType());
    child->SetSpellCat(GetSpellCat());
    child->SetDamage(GetDamage());
    child->SetDamageType(GetDamageType());
    child->SetSendsAssessMagic(GetSendsAssessMagic());
    child->SetByScript(emFXCreate_S);
    child->parent = this;
    child->root = root ? root : this;
    child->AddRef();
    if (child->root == this)
        fxList.InsertEnd(child);
    else
    {
        child->collisionOccured |= 4;
        childList.InsertEnd(child);
    }
}

void oCVisualFX::DisposeHierachy()
{
    for (int i = childList.numInArray - 1; i >= 0; --i)
    {
        childList[i]->parent = NULL;
        if (childList[i]->root == this)
            childList[i]->root = NULL;
        childList[i]->Release();
    }
    childList.Clear();
    if (root == this)
    {
        for (int i = fxList.numInArray - 1; i >= 0; --i)
        {
            oCVisualFX* fx = fxList[i];
            fx->root = NULL;
            fx->parent = NULL;
            for (int j = fx->childList.numInArray - 1; j >= 0; --j)
            {
                fx->childList[j]->root = NULL;
                fx->childList[j]->Release();
            }
            fx->childList.Clear();
            fx->Release();
        }
        fxList.Clear();
    }
    if (parent)
    {
        if (collisionOccured & 4)
            parent->childList.Remove(this);
        else
            parent->fxList.Remove(this);
    }
}

void oCVisualFX::SetSendsAssessMagic(int enabled)
{
    sendAssessMagic = enabled;
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->sendAssessMagic = enabled;
    if (root == this)
    {
        for (int i = 0; i < fxList.numInArray; ++i)
        {
            fxList[i]->sendAssessMagic = enabled;
            for (int j = 0; j < fxList[i]->childList.numInArray; ++j)
                fxList[i]->childList[j]->sendAssessMagic = enabled;
        }
    }
}

int oCVisualFX::GetSendsAssessMagic()
{
    if (sendAssessMagic)
        return 1;
    for (int i = 0; i < childList.numInArray; ++i)
        if (childList[i]->sendAssessMagic)
            return 1;
    if (root == this)
    {
        for (int i = 0; i < fxList.numInArray; ++i)
        {
            if (fxList[i]->sendAssessMagic)
                return 1;
            for (int j = 0; j < fxList[i]->childList.numInArray; ++j)
                if (fxList[i]->childList[j]->sendAssessMagic)
                    return 1;
        }
    }
    return 0;
}

void oCVisualFX::InitParser()
{
    if (fxParser)
        fxParser->Reset();
    else
    {
        zerr->Message("C: *** Visual FX Implementation v0.5");
        fxParser = new zCParser(100);
    }

    int oldEnableParsing = zCParser::enableParsing;
    zCParser::enableParsing = oldEnableParsing || zoptions->Parm("REPARSEVIS");

    zSTRING fileName;
    if (zgameoptions)
        fileName = zgameoptions->ReadString("FILES", "VisualEffects", "");
    if (fileName.IsEmpty())
        fileName = "System\\VisualFx";
    zerr->Report(zERR_TYPE_INFO, 0, "N: VFX: Loading file " + fileName + ".src or .dat",
        4, 1, 0x35F, 0, 0);

    fxParser->Parse(fileName + ".src");
    fxParser->CreatePCode();
    oCEmitterKey emitterKey;
    zSTRING emitterClass = "C_PARTICLEFXEMITKEY";
    fxParser->CheckClassSize(emitterClass, 0x160);
    oCVisualFX* fx = new oCVisualFX;
    fx->Release();
    zSTRING fxClass = "CFX_BASE";
    fxParser->CheckClassSize(fxClass, 0x268);
    zCParser::enableParsing = oldEnableParsing;
    parserVisualFX = fxParser;
}

void oCVisualFX::ExitParser()
{
    delete fxParser;
    fxParser = NULL;
    parserVisualFX = NULL;
}

void oCVisualFX::InitValues()
{
    visAlphaBlendFunc_S = "DEFAULT";
    targetPos = zVEC3(0.0f, 0.0f, 0.0f);
    visSize = zVEC3(0.0f, 0.0f, 0.0f);
    lastTrjDir = zVEC3(0.0f, 0.0f, 0.0f);
    keySize = zVEC3(0.0f, 0.0f, 0.0f);
    actSize = zVEC3(0.0f, 0.0f, 0.0f);
    castEndSize = zVEC3(0.0f, 0.0f, 0.0f);
    origin = NULL;
    target = NULL;
    inflictor = NULL;
    light = NULL;
    earthQuake = NULL;
    screenFX = NULL;
    sfx = NULL;
    sfxHnd = 0;
    sfxIsAmbient = 0;
    fxBackup = NULL;
    actKey = NULL;
    lastSetKey = NULL;
    fxInvestOrigin = NULL;
    fxInvestTarget = NULL;
    visual = NULL;
    lastSetVisual = NULL;
    homeWorld = NULL;
    collisionClass = NULL;
    root = this;
    parent = this;
    ai = new oCVisualFXAI;
    showVisual = 1;
    aiMode = 1;
    aiTimer = -1.0f;
    visAlpha = 1.0f;
    visTexAniFPS = 0.0f;
    visTexAniIsLooping = 0;
    emTrjNumKeys = 1;
    emTrjNumKeysVar = 0;
    emTrjAngleElevVar = 0.0f;
    emTrjAngleHeadVar = 0.0f;
    emTrjKeyDistVar = 0.0f;
    emTrjTargetRange = 5.0f;
    emTrjTargetAzi = 0.0f;
    emTrjTargetElev = 0.0f;
    emTrjDynUpdateDelay = 0.1f;
    emTrjDynUpdateTargetOnly = 0;
    emTrjEaseVel = 0.0f;
    emTrjMode = 1;
    emCheckCollision = 0;
    emAdjustShpToOrigin = 0;
    emInvestNextKeyDuration = 0.0f;
    emFXTriggerDelay = 0.0f;
    emFXCreatedOwnTrj = 0;
    emFlyGravity = 0.0f;
    emFXLifeSpan = -1.0f;
    lightRange = 0.0f;
    damage = 0.0f;
    damageType = 1;
    spellType = 0;
    spellCat = 0;
    spellTargetTypes = 1;
    sendAssessMagic = 0;
    secsPerDamage = -1.0f;
    bIsProjectile = 0;
    emActionCollStat = 0;
    emActionCollDyn = 0;
    collDetStat = 0;
    collDetDyn = 0;
    collisionOccured = 0;
    deleteTime = 0.0f;
    nextLevelTime = 0.0f;
    damageTime = 0.0f;
    easeTime = 0.0f;
    trjUpdateTime = 0.0f;
    levelTime = 0.0f;
    lifeSpanTimer = 0.0f;
    emTrjDist = 0.0f;
    trjSign = 1.0f;
    m_bAllowMovement = 1;
    age = 0.0f;
    fxState = 0;
    bPfxMeshSetByVisualFX = 0;
    orgNode = NULL;
    targetNode = NULL;
    oldFovX = 0.0f;
    oldFovY = 0.0f;
    savePpsValue = 0.0f;
    saveVisSizeStart = zVEC2(0.0f, 0.0f);
    if (!fxParser)
        InitParser();
}

oCVisualFXAI::~oCVisualFXAI()
{
}

void oCVisualFXAI::ReportCollisionToAI(const zCCollisionReport& report)
{
    if (vob)
        vob->ReportCollisionToAI(report);
}

void oCVisualFXAI::HostVobAddedToWorld(zCVob* host, zCWorld*)
{
    vob = host;
}

int oCVisualFXAI::GetIsProjectile()
{
    return 1;
}

void oCVisualFX::ParseStrings()
{
    emTrjMode = 0;
    emTrjMode_S.Upper();
    if (emTrjMode_S.Search("FIXED", 1) >= 0)
        emTrjMode |= 1;
    if (emTrjMode_S.Search("TARGET", 1) >= 0)
        emTrjMode |= 2;
    if (emTrjMode_S.Search("SPLINE", 1) >= 0)
        emTrjMode |= 8;
    else if (emTrjMode_S.Search("LINE", 1) >= 0)
        emTrjMode |= 4;
    if (emTrjMode_S.Search("RANDOM", 1) >= 0)
        emTrjMode |= 16;
    if (emTrjMode_S.Search("FOLLOW", 1) >= 0)
        emTrjMode |= 64;
    if (emTrjMode_S.Search("MISSILE", 1) >= 0)
        emTrjMode |= rand() / 32768.0f < 0.7f ? 2 : 128;

    emTrjLoopMode_S.Upper();
    if (emTrjLoopMode_S.Search("RESTART", 1) >= 0)
        emTrjLoopMode = 1;
    else if (emTrjLoopMode_S.Search("PINGPONG_ONCE", 1) >= 0)
        emTrjLoopMode = 2;
    else if (emTrjLoopMode_S.Search("PINGPONG", 1) >= 0)
        emTrjLoopMode = 3;
    else if (emTrjLoopMode_S.Search("HALT", 1) >= 0)
        emTrjLoopMode = 4;
    else
        emTrjLoopMode = 0;

    emTrjEaseFunc_S.Upper();
    if (emTrjEaseFunc_S.Search("LINEAR", 1) >= 0)
        emTrjEaseFunc = 0;
    else if (emTrjEaseFunc_S.Search("SINE", 1) >= 0)
        emTrjEaseFunc = 1;
    else if (emTrjEaseFunc_S.Search("EXP", 1) >= 0)
        emTrjEaseFunc = 2;

    emActionCollStat_S.Upper();
    emActionCollDyn_S.Upper();
    if (emActionCollStat_S.Search("BOUNCE", 1) >= 0)
        emActionCollStat = 2;
    if (emActionCollStat_S.Search("CREATE", 1) >= 0)
        emActionCollStat |= 4;
    if (emActionCollStat_S.Search("COLLIDE", 1) >= 0)
        emActionCollStat |= 1;
    if (emActionCollStat_S.Search("CREATEQUAD", 1) >= 0)
        emActionCollStat |= 32;
    if (emActionCollDyn_S.Search("BOUNCE", 1) >= 0)
        emActionCollDyn = 2;
    if (emActionCollDyn_S.Search("CREATEONCE", 1) >= 0)
        emActionCollDyn |= 8;
    else if (emActionCollDyn_S.Search("CREATE", 1) >= 0)
        emActionCollDyn |= 4;
    if (emActionCollDyn_S.Search("COLLIDE", 1) >= 0)
        emActionCollDyn |= 1;

    emFXCollStat_S.Upper();
    emFXCollDyn_S.Upper();
    emFXCollDynPerc_S.Upper();
    emFXCollStatAlign_S.Upper();
    emFXCollDynAlign_S.Upper();
    emSelfRotVel = zCParticleEmitter::String2Vec3(emSelfRotVel_S);
    visSize = zCParticleEmitter::String2Vec3(visSize_S);
}

static void ConfigureVisualMaterial(zCMaterial* material, const zSTRING& blendFunc, float alpha)
{
    material->rndAlphaBlendFunc = zrenderer->AlphaBlendFuncStringToType(blendFunc);
    material->color[3] = (unsigned char)(alpha * 255.0f);
}

void oCVisualFX::SetVisualByString(const zSTRING& visualName)
{
    if (visName_S == visualName && visual == lastSetVisual)
        return;
    visName_S = visualName;

    zCParticleEmitter* emitter = zCParticleFX::SearchParticleEmitter(visualName);
    if (emitter)
    {
        zCParticleFX* particle = new zCParticleFX;
        visual = particle;
        particle->SetAndStartEmitter(emitter, 1);
        CalcPFXMesh();
        if (!zrenderer->GetAlphaLimitReached())
        {
            for (int i = 0; i < 10 && particle->CalcIsDead(); ++i)
                particle->CreateParticles();
            if (particle->CalcIsDead())
                zerr->Report(zERR_TYPE_WARN, 0, "C: magic-pfx could not be started: " + visualName,
                    0, 0, 0x45E, 0, 0);
        }
    }
    else if (visualName.Search(".TGA", 1) >= 0)
    {
        zCDecal* decal = new zCDecal;
        decal->SetTexture(visualName);
        decal->SetDecalDim(visSize.n[0], visSize.n[1]);
        decal->decal2Sided = 1;
        decal->ignoreDayLight = 1;
        ConfigureVisualMaterial(decal->decalMaterial, visAlphaBlendFunc_S, visAlpha);
        visual = decal;
    }
    else if (visualName.Search(".3DS", 1) >= 0 || visualName.Search(".MDS", 1) >= 0)
    {
        visual = zCVisual::LoadVisual(visualName);
        zCProgMeshProto* proto = dynamic_cast<zCProgMeshProto*>(visual);
        if (proto)
        {
            for (int i = 0; i < proto->numSubMeshes; ++i)
                ConfigureVisualMaterial(proto->subMeshList[i].material, visAlphaBlendFunc_S, visAlpha);
        }
    }
    else if (visualName.Search(".TRL", 1) >= 0 || visualName.Search(".LTN", 1) >= 0)
    {
        zCPolyStrip* strip = new zCPolyStrip;
        strip->camAlign = visualName.Search(".TRL", 1) >= 0 ? zCPolyStrip::zCAM_ALIGN_STRIP :
            zCPolyStrip::zCAM_ALIGN_POLY;
        strip->material->SetTexture(visualName.PickWord(1, " ", "") + ".TGA");
        ConfigureVisualMaterial(strip->material, visAlphaBlendFunc_S, visAlpha);
        strip->AllocateResources(128);
        visual = strip;
        if (visualName.Search(".LTN", 1) >= 0)
            collisionOccured |= 0x40;
    }
    else if (visualName.Search(".MMS", 1) >= 0)
    {
        zCMorphMesh* morph = zCMorphMesh::Load(visualName);
        if (!morph)
        {
            zerr->Report(zERR_TYPE_FAULT, 0,
                "C: oCVisualFX::SetVisualByString(): MorphMesh not found: " + visualName,
                0, 0, 0x4F8, 0, 0);
            return;
        }
        if (morph->morphMesh)
        {
            for (int i = 0; i < morph->morphMesh->numSubMeshes; ++i)
                ConfigureVisualMaterial(morph->morphMesh->subMeshList[i].material,
                    visAlphaBlendFunc_S, visAlpha);
        }
        const zSTRING* animation = morph->GetAnyAnimation();
        if (animation)
            morph->StartAni(*animation, 1.0f, -2.0f);
        visual = morph;
    }
    else if (visualName.Search(".FOV", 1) >= 0)
    {
        collisionOccured |= 0x800;
        zsound->SetGlobalOcclusion(0.5f);
    }
    else if (visualName.Search(".SCX", 1) >= 0)
    {
        screenFX = new zCVobScreenFX;
        screenFX->SetPositionWorld(targetPos);
        homeWorld->AddVob(screenFX);
        if (visualName.Search("REVERSE", 1) >= 0)
            screenFXDir = 1;
    }
    else if (visualName.Search(".EQK", 1) >= 0)
    {
        earthQuake = new zCEarthquake;
        earthQuake->SetPositionWorld(targetPos);
        homeWorld->AddVob(earthQuake);
    }
    else if (visualName.Search(".SLW", 1) >= 0)
        collisionOccured |= 0x400;
    else
        visual = zCVisual::LoadVisual(visualName);

    lastSetVisual = visual;
    showVisual = visAlpha > 0.0f;
}

void oCVisualFX::SetCollisionEnabled(int enabled)
{
    if (showVisual & 0xC0)
    {
        emCheckCollision = 1;
        return;
    }

    emCheckCollision = enabled;
    collDetStat = enabled && emActionCollStat;
    collDetDyn = enabled && emActionCollDyn;
    if (!visual || !emCheckCollision)
        return;

    collisionClass = &zCCollObjectProjectile::s_oCollObjClass;
    if (GetIsProjectile())
        return;

    switch (GetSpellType() - 2)
    {
    case 0:
    case 2:
    case 4:
    case 0x11:
    case 0x12:
    case 0x15:
    case 0x16:
    case 0x18:
    case 0x1B:
    case 0x1C:
    case 0x1F:
    case 0x20:
    case 0x22:
    case 0x29:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x49:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x53:
    case 0x54:
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5A:
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
    case 0x60:
    case 0x61:
        return;
    case 0x23:
    case 0x24:
    case 0x55:
        if (visName_S.Search("_SPREAD", 1) >= 0)
            collisionClass = &zCCollObjectBoxPassThrough::s_oCollObjClass;
        return;
    default:
        collisionClass = &zCCollObjectBoxPassThrough::s_oCollObjClass;
    }
}

void oCVisualFX::SetByScript(const zSTRING& instanceName)
{
    zSTRING upperName = instanceName;
    upperName.Upper();
    if (!fxParser->GetSymbol(upperName))
    {
        zerr->Report(zERR_TYPE_FAULT, 0, "C:oSpellVis.h(oCVisualFX::SetByScript):Unknown ID: " +
            instanceName, 0, 0, 0x589, 0, 0);
        fxName = "";
        return;
    }
    fxParser->CreateInstance(upperName, this);
    ParseStrings();
    fxName = upperName;
    zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: Loading Visual FX Instance: " + fxName,
        10, 0, 0x593, 0, 0);

    const char* keyNames[] = { "_KEY_OPEN", "_KEY_INIT", "_KEY_CAST", "_KEY_STOP" };
    for (int i = 0; i < 4; ++i)
    {
        zSTRING keyName = fxName + keyNames[i];
        if (fxParser->GetSymbol(keyName))
        {
            oCEmitterKey* key = new oCEmitterKey;
            key->SetByScript(keyName);
            emKeyList.InsertEnd(key);
        }
    }
    for (int i = 1;; ++i)
    {
        zSTRING number;
        number = i;
        zSTRING keyName = fxName + "_KEY_INVEST_" + number;
        if (!fxParser->GetSymbol(keyName))
            break;
        oCEmitterKey* key = new oCEmitterKey;
        key->SetByScript(keyName);
        emKeyList.InsertEnd(key);
    }
    zSTRING collideKey = fxName + "_KEY_COLLIDE";
    if (fxParser->GetSymbol(collideKey))
    {
        oCEmitterKey* key = new oCEmitterKey;
        key->SetByScript(collideKey);
        emKeyList.InsertEnd(key);
    }

    visName_S.Upper();
    if (!lightPresetName.IsEmpty())
    {
        lightPresetName.Upper();
        light = new zCVobLight;
        if (!light->SetByPreset(lightPresetName))
            zerr->Report(zERR_TYPE_FAULT, 0,
                "C: oCVisualFX::SetByScript(): lightpresetName " + lightPresetName + " unknown",
                0, 0, 0x5D2, 0, 0);
        light->lightData.m_bCanMove = 1;
        lightRange = light->lightData.range;
    }
    if (!sfxID.IsEmpty())
        sfx = zsound->LoadSoundFXScript(sfxID);
    CreateHierachy();
    CreateInvestFX();
}

void oCVisualFX::UpdateFXByEmitterKey(oCEmitterKey* key)
{
    if (!key || lastSetKey == key)
        return;
    keySize = actSize;
    key->visName_S.Upper();
    if (!key->visName_S.IsEmpty())
        SetVisualByString(key->visName_S);

    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
    if (particle && particle->emitter)
    {
        zCParticleEmitter* emitter = particle->emitter;
        emitter->shpLineBoxDim = zCParticleEmitter::String2Vec3(key->pfx_shpDim_S);
        if (key->pfx_ppsValue != 0.0f)
            emitter->ppsValue = key->pfx_ppsValue;
        if (key->pfx_dirMode)
            emitter->dirMode = (zTPFX_EmitterDirMode)key->pfx_dirMode;
        if (key->pfx_dirFOR)
            emitter->dirFOR = key->pfx_dirFOR;
        if (key->pfx_shpDistribType)
            emitter->shpDistribType = key->pfx_shpDistribType;
        if (key->pfx_dirModeTargetFOR)
            emitter->dirModeTargetFOR = key->pfx_dirModeTargetFOR;
        if (key->pfx_shpDistribWalkSpeed != 0.0f)
            emitter->shpDistribWalkSpeed = key->pfx_shpDistribWalkSpeed;
        if (key->pfx_shpScaleFPS != 0.0f)
            emitter->shpScaleFPS = key->pfx_shpScaleFPS;
        if (key->pfx_velAvg != 0.0f)
            emitter->velAvg = key->pfx_velAvg;
        if (key->pfx_lspPartAvg != 0.0f)
            emitter->lspPartAvg = key->pfx_lspPartAvg;
        if (key->pfx_visAlphaStart != 0.0f)
            emitter->visAlphaStart = key->pfx_visAlphaStart;
        if (key->pfx_scTime != 0.0f)
            particle->timeScale = key->pfx_scTime;
        if (key->pfx_shpIsVolumeChg)
            emitter->shpIsVolume = !emitter->shpIsVolume;
        if (key->pfx_ppsIsSmoothChg)
            emitter->ppsIsSmooth = !emitter->ppsIsSmooth;
        if (key->pfx_ppsIsLoopingChg)
        {
            if (emitter->ppsIsLooping)
                particle->StopEmitterOutput();
            emitter->ppsIsLooping = !emitter->ppsIsLooping;
        }
        emitter->flyGravity = key->pfx_flyGravity;
        emitter->shpOffsetVec = key->pfx_shpOffsetVec;
        emitter->dirModeTargetPos = key->pfx_dirModeTargetPos;
    }

    if (light && light->homeWorld && !key->lightPresetName.IsEmpty() &&
        key->lightPresetName != lightPresetName)
    {
        light->RemoveVobFromWorld();
        light->Release();
        light = new zCVobLight;
        key->lightPresetName.Upper();
        if (!light->SetByPreset(key->lightPresetName))
            zerr->Report(zERR_TYPE_FAULT, 0,
                "C: oCVisualFX::UpdateFXByEmitterKey(): lightpresetName " + key->lightPresetName +
                    " unknown", 0, 0, 0x623, 0, 0);
        lightPresetName = key->lightPresetName;
        light->lightData.m_bCanMove = 1;
        light->SetPositionWorld(targetPos);
        homeWorld->AddVobAsChild(light, (zCVob*)this);
    }
    if (light && key->lightRange != 0.0f)
    {
        light->SetRange(key->lightRange, 1);
        lightRange = light->lightData.range;
    }

    if (key->sfx && !zsound->IsSoundActive(key->sfxHnd))
    {
        zTSound3DParams params;
        params.SetDefaults();
        params.isAmbient3D = key->sfxIsAmbient;
        key->sfxHnd = zsound->PlaySound3D(key->sfx, (zCVob*)this, 7, &params);
    }

    SetCollisionEnabled(key->emCheckCollision);
    if (key->emFlyGravity != 0.0f)
        emFlyGravity = key->emFlyGravity;
    if (key->emSelfRotVel.Length2() != 0.0f)
        emSelfRotVel = key->emSelfRotVel;
    if (key->emTrjEaseVel != 0.0f)
        emTrjEaseVel = key->emTrjEaseVel;
    if (!key->emTrjMode_S.IsEmpty())
        emTrjMode = key->emTrjMode;
    if (key->emFXLifeSpan != 0.0f)
        emFXLifeSpan = key->emFXLifeSpan;
    if (!(collisionOccured & 4))
    {
        int flags = emTrjDynUpdateTargetOnly;
        CalcTrajectory(flags);
    }
    if (!key->emCreateFXID.IsEmpty())
    {
        zCVob* effectOrigin = origin;
        if (!m_bAllowMovement && (!origin || !origin->homeWorld))
            effectOrigin = (zCVob*)this;
        oCVisualFX* created = CreateAndCastFX(key->emCreateFXID, effectOrigin, NULL);
        if (created)
        {
            created->SetInflictor(inflictor);
            created->Release();
        }
    }
    lastSetKey = key;
    actKey = key;
}

void oCVisualFX::CreateBackup()
{
}

int oCVisualFX::FindKey(const zSTRING& keyType)
{
    zSTRING searchName = "_KEY_" + keyType;
    for (int i = 0; i < emKeyList.numInArray; ++i)
        if (emKeyList[i]->name.Search(searchName, 1) >= 0)
            return i;
    return -1;
}

oCVisualFX* oCVisualFX::CreateAndPlay(const zSTRING& name, zCVob* origin, zCVob* target,
    int killAfter, float time, int flags, int slot)
{
    zSTRING upperName = name;
    upperName.Upper();
    if (upperName.IsEmpty())
        return NULL;
    if (!fxParser->GetSymbol(upperName))
    {
        zerr->Report(zERR_TYPE_WARN, 0, "C: oCVisualFX::CreateAndPlay(): VisualFX ID " + name +
            " not found in visualfxinst.d", 0, 0, 0x69D, 0, 0);
        return NULL;
    }
    oCVisualFX* fx = new oCVisualFX;
    fx->SetDuration(time);
    fx->root = fx;
    fx->parent = fx;
    fx->SetByScript(upperName);
    fx->SetOrigin(origin);
    fx->SetTarget(target);
    fx->SetInflictor(target);
    fx->Init(origin, target, 0);
    fx->Cast(killAfter);
    return fx;
}

void oCVisualFX::SetOrigin(zCVob* vob, int recalcTrajectory)
{
    if (origin == vob)
        return;
    if (origin && origin != this)
        origin->Release();
    origin = vob;
    if (origin && origin != this)
    {
        if (collisionOccured & 0x400)
        {
            zCModel* model = dynamic_cast<zCModel*>(origin->visual);
            if (model)
                model->masterAniSpeed = atof(userString[1].ToChar()) / ztimer.factorMotion;
        }
        origin->AddRef();
    }
    if (recalcTrajectory)
    {
        int flags = 0;
        CalcTrajectory(flags);
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetOrigin(vob, 1);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetOrigin(vob, 1);
}

void oCVisualFX::SetTarget(zCVob* vob, int recalcTrajectory)
{
    if (target == vob)
        return;
    if (target)
    {
        if (showTarget)
            target->showVisual &= ~2;
        if (target != this)
            target->Release();
    }
    target = vob;
    if (target)
    {
        if (target != this)
            target->AddRef();
        targetPos = target->GetPositionWorld();
        zCModel* model = dynamic_cast<zCModel*>(target->visual);
        if (model && !emTrjTargetNode_S.IsEmpty())
        {
            emTrjTargetNode_S.Upper();
            targetNode = model->SearchNode(emTrjTargetNode_S);
        }
    }
    if (recalcTrajectory)
    {
        int flags = 0;
        CalcTrajectory(flags);
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetTarget(vob, 1);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetTarget(vob, 1);
}

void oCVisualFX::SetTarget(zVEC3& position, int recalcTrajectory)
{
    targetPos = position;
    if (target)
    {
        target->Release();
        target = NULL;
    }
    if (recalcTrajectory)
    {
        int flags = 0;
        CalcTrajectory(flags);
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetTarget(position, 1);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetTarget(position, 1);
}

void oCVisualFX::SetInflictor(zCVob* vob)
{
    if (inflictor == vob)
        return;
    if (inflictor)
        inflictor->Release();
    inflictor = vob;
    if (inflictor)
        inflictor->AddRef();
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetInflictor(vob);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetInflictor(vob);
}

oCVisualFX* oCVisualFX::CreateAndCastFX(const zSTRING& name, const zCVob* source,
    const zCVob* destination)
{
    zSTRING upperName = name;
    upperName.Upper();
    if (upperName.IsEmpty() || !fxParser->GetSymbol(upperName))
        return NULL;
    oCVisualFX* fx = new oCVisualFX;
    fx->root = fx;
    fx->parent = fx;
    fx->SetSpellType(GetSpellType());
    fx->SetSpellCat(GetSpellCat());
    fx->SetDamage(GetDamage());
    fx->SetDamageType(GetDamageType());
    fx->SetLevel(GetLevel(), 0);
    fx->SetByScript(upperName);
    fx->SetOrigin(const_cast<zCVob*>(source));
    fx->SetTarget(const_cast<zCVob*>(destination));
    fx->SetInflictor(inflictor);
    fx->Init(fx->origin, fx->target, fx->inflictor);
    fx->Cast(1);
    return fx;
}

oCVisualFX* oCVisualFX::CreateAndCastFX(const zSTRING& name, const zMAT4& trafo)
{
    zSTRING upperName = name;
    upperName.Upper();
    if (upperName.IsEmpty())
        return NULL;
    if (!fxParser->GetSymbol(upperName))
    {
        zerr->Report(zERR_TYPE_WARN, 0, "C: oCVisualFX::CreateAndCastFX(): VisualFX ID " + name +
            " not found in visualfxinst.d", 0, 0, 0x721, 0, 0);
        return NULL;
    }
    oCVisualFX* fx = new oCVisualFX;
    fx->root = fx;
    fx->parent = fx;
    fx->SetSpellType(GetSpellType());
    fx->SetSpellCat(GetSpellCat());
    fx->SetDamage(GetDamage());
    fx->SetDamageType(GetDamageType());
    fx->SetLevel(GetLevel(), 0);
    fx->SetByScript(upperName);

    zCVob* source = new zCVob;
    source->SetVobName("_VFX");
    source->SetTrafoObjToWorld(trafo);
    source->SetSleeping(1);
    homeWorld->AddVob(source);
    fx->SetOrigin(source);
    fx->SetInflictor(inflictor);
    fx->Init(source, NULL, inflictor);
    fx->Cast(1);
    return fx;
}

void oCVisualFX::Reset()
{
    if (light)
        light->RemoveVobFromWorld();
    if (sfx && sfxHnd)
        zsound->StopSound(sfxHnd);
    if (fxInvestOrigin)
        fxInvestOrigin->Reset();
    if (fxInvestTarget)
        fxInvestTarget->Reset();
    SetTarget((zCVob*)NULL, 1);
    SetOrigin(NULL, 1);
    SetInflictor(NULL);
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->Reset();
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->Reset();
    collisionTime = 0.0f;
    deleteTime = 0.0f;
    nextLevelTime = 0.0f;
    damageTime = 0.0f;
    easeTime = 0.0f;
    trjUpdateTime = 0.0f;
    levelTime = 0.0f;
    lifeSpanTimer = 0.0f;
    fxState = 0;
}

void oCVisualFX::Init(const zCVob* source, const zVEC3& position)
{
    if (fxState > 1)
        return;
    SetTarget(const_cast<zVEC3&>(position), 1);
    Init(const_cast<zCVob*>(source), NULL, 0);
}

void oCVisualFX::Init(zCVob* source, zCVob* destination, int flags)
{
    if (fxState >= 3)
        return;

    zerr->Report(zERR_TYPE_INFO, 0, "C: Spell init with vob as target " + visName_S,
        10, 0, 0x8DE, 0, 0);
    if (target && target != this && showTarget)
        target->showVisual &= ~2;
    if (fxInvestTarget && fxState >= 1)
    {
        fxInvestTarget->Stop(1);
        fxInvestTarget->Release();
        fxInvestTarget = NULL;
        CreateInvestFX();
    }
    SetOrigin(source, 0);
    SetTarget(destination, 0);
    SetInflictor(flags ? (zCVob*)(unsigned long)flags : origin);

    zCModel* originModel = origin ? dynamic_cast<zCModel*>(origin->visual) : NULL;
    if (originModel && !emTrjOriginNode_S.IsEmpty())
    {
        emTrjOriginNode_S.Upper();
        orgNode = originModel->SearchNode(emTrjOriginNode_S);
    }
    zCModel* targetModel = target ? dynamic_cast<zCModel*>(target->visual) : NULL;
    if (targetModel && !emTrjTargetNode_S.IsEmpty())
    {
        emTrjTargetNode_S.Upper();
        targetNode = targetModel->SearchNode(emTrjTargetNode_S);
        if (showTarget)
            target->showVisual |= 2;
    }

    InitEffect();
    if (!origin)
    {
        zerr->Report(zERR_TYPE_WARN, 0, "C: oCVisualFX::Init() origin vob is NULL",
            0, 0, 0x914, 0, 0);
        zCVob* sourceVob = new zCVob;
        sourceVob->SetVobName("_VFX");
        sourceVob->SetPositionWorld(targetPos);
        sourceVob->SetSleeping(1);
        homeWorld->AddVob(sourceVob);
        SetOrigin(sourceVob, 1);
    }
    for (int i = 0; i < emKeyList.numInArray; ++i)
    {
        emKeyList[i]->vob = reinterpret_cast<zCVob*>(this);
        emKeyList[i]->targetPos = targetPos;
    }
    if (fxState < 1)
    {
        int keyIndex = FindKey("INIT");
        if (keyIndex >= 0)
        {
            actKey = emKeyList[keyIndex];
            UpdateFXByEmitterKey(actKey);
        }
        if (age >= emFXTriggerDelay && sfx)
        {
            zTSound3DParams params;
            params.SetDefaults();
            params.isAmbient3D = sfxIsAmbient;
            sfxHnd = zsound->PlaySound3D(sfx, reinterpret_cast<zCVob*>(this), 7, &params);
        }
        collisionOccured = (collisionOccured & ~0x3FFF) | 0x2000;
        fxState = 1;
    }

    if (collisionOccured & 4)
        reinterpret_cast<zCVob*>(this)->SetTrafoObjToWorld(parent->trafoObjToWorld);
    else
    {
        int trajectoryFlags = 0;
        CalcTrajectory(trajectoryFlags);
        reinterpret_cast<zCVob*>(this)->SetTrafoObjToWorld(trajectory.Interpolate(0.0f));
    }

    oCNpc* originNpc = dynamic_cast<oCNpc*>(origin);
    oCNpc* inflictorNpc = dynamic_cast<oCNpc*>(inflictor);
    if (originNpc && sendAssessMagic && origin != inflictor)
        originNpc->AssessMagic_S(inflictorNpc, GetSpellType());
    AdjustShapeToOrigin();

    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->Init(source, destination, flags);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->Init(source, destination, flags);
}

void oCVisualFX::Init(zCArray<zCVob*>& targets)
{
    for (int i = 0; i < vobList.numInArray; ++i)
        vobList[i]->Release();
    vobList = targets;
    for (int i = 0; i < vobList.numInArray; ++i)
        vobList[i]->AddRef();
    SetOrigin(targets.numInArray ? targets[0] : NULL, 1);
    if (targets.numInArray > 1)
        Init(targets[0], targets[1], 0);
    else if (targets.numInArray)
        Init(targets[0], NULL, 0);
}

void oCVisualFX::InvestNext()
{
    if (fxState > 2)
        return;
    SetLevel(GetLevel() + 1, 0);
    fxState = 2;
}

void oCVisualFX::SetLevel(int level, int force)
{
    if (!level || (fxState > 2 && !force))
        return;
    if (!homeWorld)
    {
        collisionOccured = (collisionOccured & ~0x3E00000) | ((level << 21) & 0x3E00000);
        return;
    }
    collisionOccured = (collisionOccured & ~0x3e000) | ((level << 13) & 0x3e000);
    if (GetLevel() > 20)
        collisionOccured = (collisionOccured & ~0x3e000) | (20 << 13);
    levelTime = 0.0f;
    zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: Spell invest next level:" + zSTRING(GetLevel()) +
        " : " + visName_S, 10, 0, 0x9B3, 0, 0);
    InitInvestFX();
    if (fxInvestTarget)
        fxInvestTarget->SetLevel(level, 0);
    if (fxInvestOrigin)
        fxInvestOrigin->SetLevel(level, 0);
    zSTRING number;
    number = level;
    int keyIndex = FindKey("INVEST_" + number);
    if (keyIndex >= 0)
    {
        actKey = emKeyList[keyIndex];
        UpdateFXByEmitterKey(actKey);
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetLevel(level, 0);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetLevel(level, 0);
    if (!force)
        fxState = 2;
}

int oCVisualFX::GetLevel() const
{
    return (collisionOccured >> 13) & 0x1f;
}

void oCVisualFX::CreateInvestFX()
{
    if (!fxInvestOrigin && !emFXInvestOrigin_S.IsEmpty())
    {
        emFXInvestOrigin_S.Upper();
        if (fxParser->GetSymbol(emFXInvestOrigin_S))
        {
            fxInvestOrigin = new oCVisualFX;
            fxInvestOrigin->root = fxInvestOrigin;
            fxInvestOrigin->parent = fxInvestOrigin;
            fxInvestOrigin->SetByScript(emFXInvestOrigin_S);
        }
    }
    if (!fxInvestTarget && !emFXInvestTarget_S.IsEmpty())
    {
        emFXInvestTarget_S.Upper();
        if (fxParser->GetSymbol(emFXInvestTarget_S))
        {
            fxInvestTarget = new oCVisualFX;
            fxInvestTarget->root = fxInvestTarget;
            fxInvestTarget->parent = fxInvestTarget;
            fxInvestTarget->SetByScript(emFXInvestTarget_S);
        }
    }
}

void oCVisualFX::InitInvestFX()
{
    if (fxInvestOrigin)
    {
        fxInvestOrigin->SetDamage(GetDamage());
        fxInvestOrigin->SetDamageType(GetDamageType());
        fxInvestOrigin->SetSpellType(GetSpellType());
        fxInvestOrigin->SetSpellCat(GetSpellCat());
        fxInvestOrigin->Init(origin, target, inflictor);
        collisionOccured |= 0x80;
    }
    if (fxInvestTarget && target && !(collisionOccured & 0x100))
    {
        fxInvestTarget->SetDamage(GetDamage());
        fxInvestTarget->SetDamageType(GetDamageType());
        fxInvestTarget->SetSpellType(GetSpellType());
        fxInvestTarget->SetSpellCat(GetSpellCat());
        fxInvestTarget->Init(target, origin, origin);
        collisionOccured |= 0x100;
    }
}

void oCVisualFX::StopInvestFX()
{
    if (fxState < 2)
        return;
    if (fxInvestOrigin)
    {
        fxInvestOrigin->Stop(1);
        fxInvestOrigin->Release();
        fxInvestOrigin = NULL;
    }
    if (fxInvestTarget)
    {
        fxInvestTarget->Stop(1);
        fxInvestTarget->Release();
        fxInvestTarget = NULL;
    }
    collisionOccured |= 0x200;
}

void oCVisualFX::Open()
{
    if (fxState >= 1)
        return;
    InitEffect();
    int keyIndex = FindKey("OPEN");
    if (keyIndex >= 0)
    {
        actKey = emKeyList[keyIndex];
        UpdateFXByEmitterKey(actKey);
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->Open();
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->Open();
    fxState = 1;
}

void oCVisualFX::Cast(int killAfter)
{
    if (fxState >= 3)
        return;
    zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: Spell cast " + visName_S, 10, 0, 0x9D0, 0, 0);
    InitEffect();
    int keyIndex = FindKey("CAST");
    if (keyIndex >= 0)
    {
        actKey = emKeyList[keyIndex];
        UpdateFXByEmitterKey(actKey);
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->Cast(killAfter);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->Cast(killAfter);
    keySize = actSize;
    levelTime = 0.0f;
    if (actKey)
        castEndSize = actSize * actKey->visSizeScale;
    if (age >= emFXTriggerDelay)
    {
        if (earthQuake)
            earthQuake->OnTrigger(NULL, NULL);
        if (screenFX)
        {
            zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: starting blend effect", 10, 0, 0x9E9, 0, 0);
            screenFX->blend.texName = userString[3];
            screenFX->blend.texAniFPS = userString[4].ToFloat();
            screenFX->blend.blendFunc = zrenderer->AlphaBlendFuncStringToType(visAlphaBlendFunc_S);
            screenFX->StartEffect(userString[2].ToFloat(), zCOLOR(255, 255, 255, 255), 0,
                screenFX->blend);
        }
    }
    StopInvestFX();
    fxState = 3;
}

void oCVisualFX::Collide(int killAfter)
{
    for (int i = 0; i < childList.numInArray; ++i)
    {
        int oldCount = childList.numInArray;
        childList[i]->Collide(killAfter);
        if (childList.numInArray != oldCount)
            i = -1;
    }
    int keyIndex = FindKey("COLLIDE");
    if (keyIndex >= 0)
    {
        actKey = emKeyList[keyIndex];
        UpdateFXByEmitterKey(actKey);
    }
    fxState = 4;
    EndEffect(killAfter);
}

void oCVisualFX::Stop(int killAfter)
{
    if (fxState >= 5)
        return;
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->Stop(killAfter);
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->Stop(killAfter);
    int keyIndex = FindKey("STOP");
    if (keyIndex >= 0)
    {
        actKey = emKeyList[keyIndex];
        UpdateFXByEmitterKey(actKey);
    }
    StopInvestFX();
    fxState = 5;
    EndEffect(killAfter);
}

void oCVisualFX::Kill()
{
    for (int i = 0; i < childList.numInArray; ++i)
    {
        childList[i]->Kill();
        childList[i]->Release();
    }
    for (int i = 0; i < fxList.numInArray; ++i)
    {
        fxList[i]->Kill();
        fxList[i]->Release();
    }
    childList.Clear();
    fxList.Clear();
    if (fxInvestOrigin)
    {
        fxInvestOrigin->Kill();
        fxInvestOrigin->Release();
        fxInvestOrigin = NULL;
    }
    if (fxInvestTarget)
    {
        fxInvestTarget->Kill();
        fxInvestTarget->Release();
        fxInvestTarget = NULL;
    }
    collisionOccured |= 0x20;
    CheckDeletion();
    CleanUpCriticalFX();
    if (homeWorld)
        homeWorld->RemoveVob(reinterpret_cast<zCVob*>(this));
}

int oCVisualFX::CanBeDeleted()
{
    if (!(collisionOccured & 0x20))
        return 0;
    for (int i = 0; i < fxList.numInArray; ++i)
        if (!(fxList[i]->collisionOccured & 0x20))
            return 0;
    return 1;
}

int oCVisualFX::IsFinished()
{
    if (collisionOccured & 0x28)
        return 1;
    if (visual)
    {
        zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
        if (particle)
            return particle->CalcIsDead();
        return visual->GetVisualDied();
    }
    return visName_S.IsEmpty() && !earthQuake && !screenFX;
}

int oCVisualFX::IsLooping()
{
    if (IsFinished())
        return 0;
    return emFXLifeSpan == -1.0f;
}

void oCVisualFX::InitEffect()
{
    zCVob* self = reinterpret_cast<zCVob*>(this);
    if (!(self->sleepingMode & 3))
        self->SetSleeping(0);
    if (collisionOccured & 0x10)
        return;

    if (!homeWorld)
    {
        zMAT4 transform = trafoObjToWorld;
        if (origin && origin->homeWorld)
        {
            transform = orgNode ? origin->GetTrafoModelNodeToWorld(orgNode) : origin->trafoObjToWorld;
            homeWorld = origin->homeWorld;
        }
        else if (parent && parent->homeWorld)
        {
            transform = parent->trafoObjToWorld;
            homeWorld = parent->homeWorld;
        }
        else if (root && root->homeWorld)
        {
            transform = root->trafoObjToWorld;
            homeWorld = root->homeWorld;
        }
        else if (zCCamera::activeCam && zCCamera::activeCam->connectedVob &&
            zCCamera::activeCam->connectedVob->homeWorld)
        {
            zCVob* cameraVob = zCCamera::activeCam->connectedVob;
            transform = cameraVob->trafoObjToWorld;
            transform.SetTranslation(cameraVob->GetPositionWorld() +
                cameraVob->trafoObjToWorld.GetAtVector() * 300.0f);
            homeWorld = cameraVob->homeWorld;
            zerr->Report(zERR_TYPE_WARN, 0, "C: oCVisualFX :: InitEffect(): vob not in world!",
                0, 0, 0xABD, 0, 0);
        }
        else if (ogame->GetWorld())
        {
            homeWorld = ogame->GetWorld();
            if (origin)
                transform = orgNode ? origin->GetTrafoModelNodeToWorld(orgNode) :
                    origin->trafoObjToWorld;
        }
        else
            zerr->Report(zERR_TYPE_FATAL, 0, "C: oVisFX.cpp(oCVisualFX::Open): vob not in world",
                0, 0, 0xAD5, 0, 0);

        if (homeWorld)
        {
            self->SetTrafoObjToWorld(transform);
            trafoObjToWorld = transform;
            homeWorld->AddVob(self);
        }
    }

    self->SetVobName(fxName + " Vob");
    self->SetSleepingMode(1);
    self->SetPhysicsEnabled(0);
    SetVisualByString(visName_S);
    if (light)
    {
        light->SetPositionWorld(trafoObjToWorld.GetTranslation());
        homeWorld->AddVobAsChild(light, self);
    }
    collisionOccured |= 0x10;

    switch (GetSpellType() - 2)
    {
    case 0:
    case 4:
    case 0x11:
    case 0x12:
    case 0x15:
    case 0x18:
    case 0x1C:
    case 0x1F:
    case 0x20:
    case 0x22:
    case 0x29:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x54:
        SetIsProjectile(1);
        break;
    case 0x24:
    case 0x55:
        if (visName_S.Search("_SPREAD", 1) < 0)
            SetIsProjectile(1);
        break;
    }
    SetCollisionEnabled(emCheckCollision);

    int queuedLevel = (collisionOccured >> 21) & 0x1F;
    if (queuedLevel != 0x1F)
    {
        if (queuedLevel != 0x1E)
            SetLevel(queuedLevel, 0);
        else
            InvestNext();
        collisionOccured |= 0x3E00000;
    }
    fxStartTime = ztimer.totalTimeFloat / ztimer.factorMotion;
}

void oCVisualFX::EndEffect(int killAfter)
{
    collDetStat = 0;
    collDetDyn = 0;
    if (!killAfter)
    {
        SetShowVisual(0);
        return;
    }
    collisionOccured |= 8;
    if (homeWorld)
        homeWorld->RemoveVob(reinterpret_cast<zCVob*>(this));
}

void oCVisualFX::SetShowVisual(int enabled)
{
    if (showVisual != enabled)
    {
        if (enabled)
            SetVisualByString(visName_S);
        else
        {
            zSTRING empty;
            SetVisualByString(empty);
        }
    }
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetShowVisual(enabled);
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetShowVisual(enabled);
    showVisual = enabled;
}

void oCVisualFX::ReportCollision(const zCCollisionReport& report)
{
    if (!parent || !root)
        return;
    if (fxState <= 2)
        zerr->Report(zERR_TYPE_WARN, 0,
            " C: oCVisualFX::ReportCollision(): collision before INVESTNEXT state (Name:" +
                visName_S + " )!!", 0, 0, 0xB34, 0, 0);
    if (parent != root || (collisionOccured & 4))
    {
        parent->ReportCollision(report);
        lastTrjDir = parent->lastTrjDir;
        collisionOccured = (collisionOccured & ~0x1C0000) |
            (parent->collisionOccured & 0x1C0000);
    }

    collisionOccured |= 1;
    zCVob* self = reinterpret_cast<zCVob*>(this);
    zCCollisionObject* other = self->GetCollisionObject() == report.m_poCollObject1 ?
        report.m_poCollObject0 : report.m_poCollObject1;
    zSVisualFXColl collision;
    collision.foundVob = other ? static_cast<zCVob*>(other->m_pvClientData) : NULL;
    collision.foundContactPoint = targetPos;
    collision.foundNormal = lastTrjDir;

    if (other && other->GetCollObjClass() == &zCCollObjectLevelPolys::s_oCollObjClass)
    {
        zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: collision (level)", 10, 0, 0xB6F, 0, 0);
        int alignNormal = emFXCollStatAlign_S == "COLLISIONNORMAL";
        if (emActionCollStat & 2)
            CollisionResponse(collision.foundNormal, alignNormal);
        else if (emActionCollStat & 1)
            self->ResetToOldMovementState();

        if (emActionCollStat & 0x20)
        {
            zCQuadMark* mark = new zCQuadMark;
            mark->material->SetTexture(userString[0]);
            mark->material->rndAlphaBlendFunc =
                zrenderer->AlphaBlendFuncStringToType(userString[2]);
            mark->CreateQuadMark(targetPos, collision.foundNormal, zVEC2(10.0f, 10.0f),
                userString[0], NULL);
            zCVob* quadVob = new zCVob;
            quadVob->SetVobName("_VFX_Quad");
            quadVob->SetPositionWorld(targetPos);
            quadVob->SetVisual(mark);
            homeWorld->AddVob(quadVob);
            quadVob->Release();
            mark->Release();
        }

        if ((emActionCollStat & 4) && !emFXCollStat_S.IsEmpty())
        {
            if (!(emActionCollStat & 2))
                self->ResetToOldMovementState();
            zMAT4 pose = GetPoseOfHeading(targetPos, collision.foundNormal);
            oCVisualFX* created = CreateAndCastFX(emFXCollStat_S, pose);
            if (created)
            {
                if (created->IsLooping())
                {
                    zerr->Report(zERR_TYPE_WARN, 0,
                        "C: oCVisualFX::ReportCollision(), created (stat) collision fx " +
                            emFXCollStat_S + " has infinite duration", 0, 0, 0xBA5, 0, 0);
                    created->SetDuration(1000.0f);
                }
                created->Release();
            }
        }
        return;
    }

    if (collision.foundVob && !(collision.foundVob->isInMovementMode & 3))
    {
        if (ProcessCollision(collision))
            Collide(1);
        return;
    }

    zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: queueing vob collision...", 10, 0, 0xB67, 0, 0);
    if (collision.foundVob)
        collision.foundVob->AddRef();
    queuedCollisions.InsertEnd(collision);
}

int oCVisualFX::ProcessQueuedCollisions()
{
    int result = 0;
    for (int i = 0; i < queuedCollisions.numInArray; ++i)
    {
        result |= ProcessCollision(queuedCollisions[i]);
        if (queuedCollisions[i].foundVob)
        {
            queuedCollisions[i].foundVob->Release();
            queuedCollisions[i].foundVob = NULL;
        }
    }
    queuedCollisions.Clear();
    if (result)
        Collide(1);
    return result;
}

int oCVisualFX::ProcessCollision(zSVisualFXColl& collision)
{
    zCVob* found = collision.foundVob;
    if (origin == found)
        return 0;

    if (found)
        zerr->Report(zERR_TYPE_INFO, 0, "C: VIS: collision (vob) " + visName_S +
            " with vob:" + found->GetObjectName(), 10, 0, 0xBD7, 0, 0);
    if (emActionCollDyn & 2)
        CollisionResponse(collision.foundNormal, emFXCollDynAlign_S == "COLLISIONNORMAL");
    if (found && !collidedVobs.Contains(found))
    {
        collidedVobs.InsertEnd(found);
        found->AddRef();
    }
    if (found && (emActionCollDyn & 8))
    {
        found->AddRef();
        ignoreVobList.InsertEnd(found);
    }
    if (!emFXCollDyn_S.IsEmpty())
    {
        oCVisualFX* fx = CreateAndPlay(emFXCollDyn_S, found, origin, 0, 0.0f, 0, 0);
        if (fx)
        {
            fx->SetInflictor(inflictor);
            fx->Release();
        }
    }

    oCNpc* targetNpc = dynamic_cast<oCNpc*>(target);
    if (targetNpc)
    {
        parser.SetInstance("SELF", targetNpc);
        parser.SetInstance("OTHER", dynamic_cast<oCNpc*>(origin));
        int function = parser.GetIndex("C_CANNPCCOLLIDEWITHSPELL");
        if (function > 0 && !parser.CallFunc(function, GetSpellType()))
            return 0;
        if (function <= 0)
            zerr->Report(zERR_TYPE_WARN, 0, "C: Skript Func C_CANNPCCOLLIDEWITHSPELL unknown!",
                0, 0, 0xC0E, 0, 0);
    }

    if (!emFXCollDynPerc_S.IsEmpty())
    {
        oCVisualFX* fx = CreateAndCastFX(emFXCollDynPerc_S, origin, found);
        if (fx)
        {
            if (fx->IsLooping())
            {
                zCPar_Symbol* symbol = parser.GetSymbol("NPC_BURN_TICKS_PER_DAMAGE_POINT");
                int ticks = 0;
                if (symbol)
                    symbol->GetValue(ticks, 0);
                fx->SetDuration((float)ticks);
            }
            fx->Release();
        }
    }

    oCNpc* hitNpc = dynamic_cast<oCNpc*>(found);
    if (hitNpc && hitNpc->IsConditionValid() && emCheckCollision != 2)
    {
        float multiplier = 1.0f;
        if (damageType & 4)
            multiplier = 0.5f;
        else if (damageType & 8)
            multiplier = 2.0f;

        oCNpc::oSDamageDescriptor descriptor = {};
        descriptor.pVobAttacker = inflictor;
        descriptor.pNpcAttacker = dynamic_cast<oCNpc*>(inflictor);
        descriptor.pVobHit = found;
        descriptor.nSpellID = GetSpellType();
        descriptor.nSpellCat = GetSpellCat();
        descriptor.nSpellLevel = GetLevel();
        descriptor.fDamageTotal = GetDamage() * GetLevel() * multiplier;
        descriptor.SetFXHit(this);
        hitNpc->GetEM(0)->OnMessage(new oCMsgDamage(0, descriptor), hitNpc);
    }
    return emActionCollDyn & 1;
}

void oCVisualFX::CollisionResponse(const zVEC3& normal, int flags)
{
    if (collisionOccured & 4)
        return;

    collisionOccured += 0x40000;
    reinterpret_cast<zCVob*>(this)->ResetToOldMovementState();
    int collisionCount = (collisionOccured >> 18) & 7;
    if (collisionCount > 20)
    {
        if (fxState >= 3)
        {
            zerr->Report(zERR_TYPE_WARN, 0, "C: Emergency Stop: " + zSTRING(collisionCount),
                0, 0, 0xC78, 0, 0);
            Stop(1);
        }
        else
        {
            zerr->Report(zERR_TYPE_WARN, 0, "C: collisionCtr > zVFX_MAX_CONTINOUS_COLL, reset! ",
                0, 0, 0xC7C, 0, 0);
            collisionOccured &= ~0x1C0000;
        }
    }
    if ((collisionOccured & 0x40) == 0)
    {
        for (int i = 0; i < 3; ++i)
            lastTrjDir.n[i] += rand() / 32768.0f * 0.1f;
        lastTrjDir.Normalize();
    }
    lastTrjDir = trajectory.RecalcByColl(targetPos, lastTrjDir, normal, flags);

    zCPolyStrip* strip = dynamic_cast<zCPolyStrip*>(visual);
    if ((collisionOccured & 0x40) && strip)
    {
        strip->FreeResources();
        strip->AllocateResources(128);
        for (int i = 0; i < trajectory.GetNumKeys(); ++i)
        {
            zMAT4 key = trajectory.GetKey(i);
            zVEC3 center = key.GetTranslation();
            zVEC3 extent = key.GetUpVector() * emTrjEaseVel;
            strip->AddSegment(center + extent, center - extent);
        }
    }
    collisionOccured &= ~1;
}

void oCVisualFX::SetCollisionCandidates(zCArray<zCVob*>& targets)
{
    for (int i = 0; i < allowedCollisionVobList.numInArray; ++i)
        allowedCollisionVobList[i]->Release();
    allowedCollisionVobList = targets;
    for (int i = 0; i < allowedCollisionVobList.numInArray; ++i)
        allowedCollisionVobList[i]->AddRef();
}

int oCVisualFX::GetCollidedCandidates(zCArray<zCVob*>& targets)
{
    if (!collidedVobs.numInArray)
        return 0;
    targets = collidedVobs;
    return 1;
}

int oCVisualFX::CanThisCollideWith(zCVob* vob)
{
    if (!emCheckCollision || !vob || !vob->visual || vob == origin || vob == inflictor ||
        vob == reinterpret_cast<zCVob*>(this) || ignoreVobList.Contains(vob))
        return 0;
    if (allowedCollisionVobList.numInArray)
        return allowedCollisionVobList.Contains(vob);
    if (!oCSpell::IsTargetTypeValid(vob, spellTargetTypes))
        return 0;
    if (dynamic_cast<oCVisualFX*>(vob) || vob->GetObjectName() == "_VFX")
        return 0;
    if (dynamic_cast<oCItem*>(vob) && dynamic_cast<oCAIArrow*>(vob->callback_ai))
        return 0;

    if (inflictor != oCNpc::player && target && target != vob)
    {
        oCNpc* targetNpc = dynamic_cast<oCNpc*>(target);
        oCNpc* otherNpc = dynamic_cast<oCNpc*>(vob);
        if (targetNpc && otherNpc && otherNpc->IsAPlayer())
            return 1;
    }

    for (int i = 0; i < childList.numInArray; ++i)
        if (childList[i]->emCheckCollision && !childList[i]->CanThisCollideWith(vob))
            return 0;
    for (int i = 0; i < fxList.numInArray; ++i)
        if (fxList[i]->emCheckCollision && !fxList[i]->CanThisCollideWith(vob))
            return 0;
    return 1;
}

void oCVisualFX::CalcTrajectory(const int& flags)
{
    if (flags)
    {
        zMAT4 targetTrafo;
        if (target && targetNode)
            targetTrafo = target->GetTrafoModelNodeToWorld(targetNode);
        else if (target)
            targetTrafo = target->trafoObjToWorld;
        else
            targetTrafo.SetTranslation(targetPos);

        if (trajectory.GetNumKeys())
        {
            trajectory.keyList[trajectory.GetNumKeys() - 1]->SetMat(targetTrafo);
            if (trajectory.spl)
                trajectory.spl->Changed();
        }
        return;
    }

    trajectory.Clear();
    trajectory.mode = emTrjMode;
    if (!origin)
        return;

    zMAT4 originTrafo = orgNode ? origin->GetTrafoModelNodeToWorld(orgNode) : origin->trafoObjToWorld;
    if (vobList.numInArray == 1)
    {
        trajectory.InsertKey(vobList[0]->trafoObjToWorld);
        return;
    }
    if (vobList.numInArray > 1)
    {
        for (int i = 1; i < vobList.numInArray; ++i)
        {
            zVEC3 direction = vobList[i]->GetPositionWorld() - vobList[i - 1]->GetPositionWorld();
            direction.Normalize();
            trajectory.InsertKey(GetPoseOfHeading(vobList[i - 1]->GetPositionWorld(), direction));
        }
        trajectory.InsertKey(vobList[vobList.numInArray - 1]->trafoObjToWorld);
        return;
    }

    zMAT4 targetTrafo;
    if (target && targetNode)
        targetTrafo = target->GetTrafoModelNodeToWorld(targetNode);
    else if (target)
        targetTrafo = target->trafoObjToWorld;
    else
        targetTrafo.SetTranslation(targetPos);

    if ((GetIsProjectile() || collisionClass == &zCCollObjectProjectile::s_oCollObjClass) &&
        origin != target && target)
    {
        zVEC3 direction = target->GetPositionWorld() - origin->GetPositionWorld();
        if (direction.Length2() != 0.0f)
        {
            direction.Normalize();
            targetTrafo.SetTranslation(target->GetPositionWorld() + direction * 50.0f);
        }
    }

    if ((emTrjMode & 2) && (emTrjMode & 0x10))
    {
        int numKeys = (int)(emTrjNumKeys + emTrjNumKeysVar * rand() / 32768.0f) + 1;
        zVEC3 start = originTrafo.GetTranslation();
        zVEC3 end = targetTrafo.GetTranslation();
        zVEC3 heading = end - start;
        heading.Normalize();
        for (int i = 0; i < numKeys; ++i)
        {
            float fraction = numKeys > 1 ? (float)i / (numKeys - 1) : 0.0f;
            trajectory.InsertKey(GetPoseOfHeading(start + (end - start) * fraction, heading));
        }
    }
    else
    {
        trajectory.InsertKey(originTrafo);
        if (emTrjMode & 2)
        {
            zVEC3 direction = targetTrafo.GetTranslation() - originTrafo.GetTranslation();
            if (direction.Length2() != 0.0f)
            {
                direction.Normalize();
                targetTrafo = GetPoseOfHeading(targetTrafo.GetTranslation(), direction);
            }
        }
        trajectory.InsertKey(targetTrafo);
    }

    emTrjCreatedOwnTrj = emTrjMode;
    trajectory.Calc();

    zCPolyStrip* strip = dynamic_cast<zCPolyStrip*>(visual);
    if ((collisionOccured & 0x40) && strip)
    {
        strip->FreeResources();
        strip->AllocateResources(128);
        for (int i = 0; i < trajectory.GetNumKeys(); ++i)
        {
            zMAT4 key = trajectory.GetKey(i);
            zVEC3 center = key.GetTranslation();
            zVEC3 extent = key.GetUpVector() * visSize.n[0];
            strip->AddSegment(center + extent, center - extent);
        }
    }
}

int oCVisualFX::CheckDeletion()
{
    if (collisionOccured & 0x20)
    {
        oCNpc* originNpc = dynamic_cast<oCNpc*>(origin);
        if (originNpc && sendAssessMagic && origin != inflictor)
            originNpc->AssessStopMagic_S(dynamic_cast<oCNpc*>(inflictor), GetSpellType());
        if (sfx)
        {
            sfx->Release();
            if (sfxHnd)
                zsound->StopSound(sfxHnd);
            sfx = NULL;
        }
        for (int i = 0; i < emKeyList.numInArray; ++i)
            delete emKeyList[i];
        emKeyList.Clear();
        if (ai)
            ai->delete_it = 1;
        return 1;
    }

    if (!(collisionOccured & 8) && IsFinished())
    {
        if (fxState >= 3)
            collisionOccured |= 8;
        else
            zerr->Report(zERR_TYPE_WARN, 0,
                "C: VIS: (oCVisualFX :: CheckDeletion()): Visual died before spell was casted. Name:" +
                    visName_S, 0, 0, 0xD18, 0, 0);
    }
    if (emFXLifeSpan != -1.0f)
    {
        lifeSpanTimer += frameTime;
        if (lifeSpanTimer >= emFXLifeSpan)
        {
            emFXLifeSpan = -1.0f;
            collisionOccured |= 8;
        }
    }
    if (!(collisionOccured & 8))
        return 0;
    if (dynamic_cast<zCDecal*>(visual))
    {
        collisionOccured |= 0x20;
        return 1;
    }

    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
    if (deleteTime == 0.0f)
    {
        if (particle)
            particle->StopEmitterOutput();
        if (screenFX)
            screenFX->StartEffect(userString[2].ToFloat(), zCOLOR(255, 255, 255, 255), 0,
                screenFX->blend);
        if (collisionOccured & 0x400)
        {
            ztimer.factorMotion = 1.0f;
            zCModel* model = origin ? dynamic_cast<zCModel*>(origin->visual) : NULL;
            if (model)
                model->masterAniSpeed = 1.0f;
        }
        deleteTime = 0.000001f;
    }

    if (!visual || (particle && particle->CalcIsDead()))
    {
        deleteTime += frameTime;
        if (deleteTime >= 2.0f)
        {
            for (int i = 0; i < childList.numInArray; ++i)
                childList[i]->Stop(1);
            collisionOccured |= 0x20;
        }

        float fade = (zSinApprox(deleteTime * 3.14159274f - 1.57079637f) + 1.0f) * 0.5f;
        if (light)
            light->SetRange((1.0f - fade) * lightRange, 1);
        if (collisionOccured & 0x1800)
        {
            float fovX;
            float fovY;
            zCCamera::activeCam->GetFOV(fovX, fovY);
            zCCamera::activeCam->SetFOV(oldFovX * fade + fovX * (1.0f - fade),
                oldFovY * fade + fovY * (1.0f - fade));
            zCCamera::activeCam->Activate();
            zsound->SetGlobalOcclusion((1.0f - fade) * 0.5f);
        }
        if (sfxHnd)
        {
            zTSound3DParams params;
            params.SetDefaults();
            if (zsound->GetSound3DProps(sfxHnd, params))
            {
                params.volume = 1.0f - fade;
                zsound->UpdateSound3D(sfxHnd, &params);
            }
        }
        if (visual)
            visual->visualAlpha = 1.0f - fade;
    }
    return 0;
}

void oCVisualFX::UpdateActKey()
{
    if (!actKey)
        return;
    float fraction = actKey->scaleDuration > 0.0f ? levelTime / actKey->scaleDuration : 1.0f;
    if (fraction > 1.0f)
        fraction = 1.0f;
    if (fxState >= 3 && actKey->visSizeScale != 0.0f)
        actSize = keySize + (castEndSize - keySize) * fraction;
    else
        actSize = keySize + (visSize * actKey->visSizeScale - keySize) * fraction;
    zCDecal* decal = dynamic_cast<zCDecal*>(visual);
    if (decal)
        decal->SetDecalDim(actSize.n[0], actSize.n[1]);
    if (emInvestNextKeyDuration > 0.0f && fxState == 2)
    {
        nextLevelTime += frameTime;
        if (nextLevelTime >= emInvestNextKeyDuration)
        {
            UpdateFXByEmitterKey(fxBackup);
            nextLevelTime = 0.0f;
        }
    }
}

static zVEC3 lastTrjPos;

void oCVisualFX::DoMovements()
{
    zCVob* self = reinterpret_cast<zCVob*>(this);
    lastTrjPos = trafoObjToWorld.GetTranslation();
    if (!origin || !origin->homeWorld || !m_bAllowMovement)
        return;

    self->BeginMovement();
    if (collisionOccured & 4)
    {
        if (parent)
        {
            self->SetPositionWorld(parent->trafoObjToWorld.GetTranslation());
            trafoObjToWorld = parent->trafoObjToWorld;
        }
        else if (!emTrjMode)
            self->Move(0.0f, emFlyGravity * frameTime, 0.0f);
    }
    else if (emTrjMode & 1)
        self->Move(0.0f, emFlyGravity * frameTime, 0.0f);
    else if (emTrjMode)
    {
        if (emFlyGravity != 0.0f)
            trajectory.ApplyGrav(emFlyGravity * frameTime);
        if (target)
            targetPos = target->GetPositionWorld();

        zMAT4 matrix;
        if (emTrjEaseFunc == 1)
        {
            easeTime += frameTime / emTrjEaseVel * trjSign;
            if (easeTime >= 1.0f || easeTime <= 0.0f)
            {
                if (emTrjLoopMode == 1)
                    easeTime = 0.0f;
                else if (emTrjLoopMode == 2 || emTrjLoopMode == 3)
                    trjSign = -trjSign;
                else if (emTrjLoopMode == 4 && trjSign < 0.0f)
                    trjSign = -trjSign;
                else
                    easeTime = easeTime < 0.0f ? 0.0f : 1.0f;
            }
            float fraction = (zSinApprox(easeTime * 3.14159274f - 1.57079637f) + 1.0f) * 0.5f;
            matrix = trajectory.Interpolate(trajectory.length * fraction);
        }
        else
        {
            emTrjDist += emTrjEaseVel * frameTime * trjSign;
            if (emTrjDist >= trajectory.length || emTrjDist <= 0.0f)
            {
                if (emTrjLoopMode == 1)
                    emTrjDist = 0.0f;
                else if (emTrjLoopMode == 2 || emTrjLoopMode == 3)
                    trjSign = -trjSign;
                else if (emTrjLoopMode == 4 && trjSign < 0.0f)
                    trjSign = -trjSign;
                else
                    emTrjDist = emTrjDist < 0.0f ? 0.0f : trajectory.length;
            }
            matrix = trajectory.Interpolate(emTrjDist);
        }

        targetPos = matrix.GetTranslation();
        if (emSelfRotVel.Length2() != 0.0f || dynamic_cast<zCDecal*>(visual))
        {
            self->SetTrafoObjToWorld(matrix);
            trafoObjToWorld = matrix;
        }
        else
        {
            self->SetPositionWorld(targetPos);
            trafoObjToWorld.SetTranslation(targetPos);
        }
    }

    if (emSelfRotVel.n[0] != 0.0f)
        self->RotateLocalX(emSelfRotVel.n[0] * frameTime);
    if (emSelfRotVel.n[1] != 0.0f)
        self->RotateLocalY(emSelfRotVel.n[1] * frameTime);
    if (emSelfRotVel.n[2] != 0.0f)
        self->RotateLocalZ(emSelfRotVel.n[2] * frameTime);
    self->EndMovement(1);

    if (earthQuake)
        earthQuake->SetPositionWorld(trafoObjToWorld.GetTranslation());
    if (screenFX)
        screenFX->SetPositionWorld(trafoObjToWorld.GetTranslation());
    if (!(collisionOccured & 1))
        lastTrjDir = (trafoObjToWorld.GetTranslation() - lastTrjPos).NormalizeSafe();
    trjUpdateTime += frameTime;
}

zVEC3& zVEC3::NormalizeSafe()
{
    float length = sqrt(Length2());
    if (length == 0.0f)
        length = 0.000001f;
    float inverseLength = 1.0f / length;
    n[0] *= inverseLength;
    n[1] *= inverseLength;
    n[2] *= inverseLength;
    return *this;
}

zVEC3 operator^(const zVEC3& first, const zVEC3& second)
{
    return zVEC3(first.n[1] * second.n[2] - first.n[2] * second.n[1],
        first.n[2] * second.n[0] - first.n[0] * second.n[2],
        first.n[0] * second.n[1] - first.n[1] * second.n[0]);
}

zMAT4 zCPositionKey::GetMat()
{
    zMAT4 matrix = {};
    for (int i = 0; i < 4; ++i)
        matrix.m[i][i] = 1.0f;
    rot.QuatToMatrix4(matrix);
    matrix.SetTranslation(p);
    return matrix;
}

void zCKBSpline::InterpolateN(float time, zMAT4& matrix, int mode)
{
    float start = polynom->tmin;
    float range = polynom[numPolynomials - 1].tmax - start;
    Interpolate0(start + range * time, matrix, mode);
}

int oCVisualFX::CheckDelayedTrigger()
{
    age += frameTime;
    if (!emFXTriggerDelay)
        return 1;
    if (age < emFXTriggerDelay)
        return 0;
    SetVisualByString(visName_S);
    if (sfx && fxState >= 1)
    {
        zTSound3DParams params = {};
        params.isAmbient3D = sfxIsAmbient;
        sfxHnd = zsound->PlaySound3D(sfx, reinterpret_cast<zCVob*>(this), 7, &params);
    }
    if (fxState >= 3)
    {
        if (earthQuake)
            earthQuake->OnTrigger(NULL, NULL);
        if (screenFX)
            screenFX->StartEffect(1.0f, zCOLOR(255, 255, 255, 255), 0, screenFX->blend);
    }
    emFXTriggerDelay = 0.0f;
    return 1;
}

void oCVisualFX::AdjustShapeToOrigin()
{
    if (!emAdjustShpToOrigin || !origin || !visual || !origin->visual)
        return;

    zTBBox3D box = origin->visual->GetBBox3D();
    zVEC3 dimensions = box.maxs - box.mins;
    dimensions.n[0] = fabs(dimensions.n[0]);
    dimensions.n[1] = fabs(dimensions.n[1]);
    dimensions.n[2] = fabs(dimensions.n[2]);
    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
    if (particle && particle->emitter)
    {
        if (particle->emitter->shpType == 2)
        {
            float largest = dimensions.n[0];
            if (dimensions.n[1] > largest)
                largest = dimensions.n[1];
            if (dimensions.n[2] > largest)
                largest = dimensions.n[2];
            float scale = largest > 5.0f ? 5.0f : largest;
            if (scale < 1.0f)
                scale = 1.0f;
            particle->emitter->shpLineBoxDim = dimensions * scale;
        }
        else if (particle->emitter->shpType != 4)
            particle->emitter->shpLineBoxDim = dimensions;
    }
    zCDecal* decal = dynamic_cast<zCDecal*>(visual);
    if (decal)
    {
        if (dynamic_cast<zCModel*>(origin->visual))
            decal->SetDecalDim(dimensions.n[0] > 350.0f ? dimensions.n[0] : 350.0f,
                dimensions.n[1]);
        else
            decal->SetDecalDim(dimensions.n[0], dimensions.n[2]);
    }
}

zVEC2 operator*(const zVEC2& vector, float value)
{
    zVEC2 result;
    result.n[0] = vector.n[0] * value;
    result.n[1] = vector.n[1] * value;
    return result;
}

void oCVisualFX::UpdateEffects()
{
    if (sfx && sfxHnd)
    {
        zTSound3DParams params = {};
        zsound->UpdateSound3D(sfxHnd, params);
    }
    if (collisionOccured & 8)
        return;

    if (!(collisionOccured & 4) && trjUpdateTime >= emTrjDynUpdateDelay)
    {
        trjUpdateTime = 0.0f;
        CalcTrajectory(emTrjDynUpdateTargetOnly);
    }

    zCPolyStrip* strip = dynamic_cast<zCPolyStrip*>(visual);
    if (strip && !(collisionOccured & 0x40))
    {
        zVEC3 center = trafoObjToWorld.GetTranslation();
        zVEC3 extent = trafoObjToWorld.GetUpVector() * (GetLevel() * 20.0f);
        strip->AddSegment(center + extent, center - extent);
        strip->CalcBBox3D();
    }

    if (screenFX)
    {
        screenFXTime += frameTime;
        if (screenFXTime >= userString[0].ToFloat())
        {
            screenFXTime = 0.0f;
            if (screenFXDir)
            {
                screenFXDir = 0;
                screenFX->blend.texName = userString[3];
                screenFX->blend.texAniFPS = userString[4].ToFloat();
                screenFX->blend.blendFunc =
                    zrenderer->AlphaBlendFuncStringToType(visAlphaBlendFunc_S);
                screenFX->StartEffect(userString[2].ToFloat(), zCOLOR(255, 255, 255, 255), 0,
                    screenFX->blend);
            }
            else
            {
                screenFXDir = 1;
                screenFX->StartEffect(userString[2].ToFloat(), zCOLOR(255, 255, 255, 255), 1,
                    screenFX->blend);
            }
        }
    }

    if (collisionOccured & 0x1800)
    {
        float elapsed = ztimer.totalTimeFloat / ztimer.factorMotion - fxStartTime;
        float remaining = 1.0f - elapsed * 0.0005f;
        if (remaining < 0.0f)
            remaining = 0.0f;
        float fade = (zSinApprox(remaining * 3.14159274f - 1.57079637f) + 1.0f) * 0.5f;
        zCCamera::activeCam->SetFOV(oldFovX * fade, oldFovY * fade);
        zCCamera::activeCam->Activate();
    }
}

void oCVisualFX::OnTick()
{
    frameTime = ztimer.frameTimeFloat * 0.001f;
    if (aiTimer > 0.0f)
    {
        aiTimer -= frameTime;
        if (aiTimer < 0.0f)
            aiTimer = 0.0f;
    }
    if (ProcessQueuedCollisions() || CheckDeletion() || !CheckDelayedTrigger())
        return;
    AdjustShapeToOrigin();
    levelTime += frameTime;
    oCNpc* npc = dynamic_cast<oCNpc*>(origin);
    if (secsPerDamage != -1.0f && npc)
    {
        damageTime += frameTime;
        if (damageTime >= secsPerDamage)
        {
            npc->ChangeAttribute(0, -1);
            damageTime = 0.0f;
        }
    }
    UpdateActKey();
    UpdateEffects();
    DoMovements();
}

void oCVisualFX::Archive(zCArchiver& archiver)
{
    zCObject::Archive(archiver);
}

void oCVisualFX::Unarchive(zCArchiver& archiver)
{
    zCObject::Unarchive(archiver);
}

void oCVisualFX::CalcPFXMesh()
{
    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
    if (!particle || !particle->emitter || !emAdjustShpToOrigin || !origin ||
        particle->emitter->shpType != 5)
        return;
    bPfxMeshSetByVisualFX = 1;
    ReleasePFXMesh();
    zCVisual* source = origin->visual;
    if (!source)
        return;
    if (dynamic_cast<zCMesh*>(source))
    {
        particle->emitter->shpMesh = (zCMesh*)source;
        source->AddRef();
    }
    else if (dynamic_cast<zCProgMeshProto*>(source))
    {
        particle->emitter->shpProgMesh = (zCProgMeshProto*)source;
        source->AddRef();
    }
    else if (dynamic_cast<zCModel*>(source))
    {
        particle->emitter->shpModel = (zCModel*)source;
        source->AddRef();
    }
    else if (zCMorphMesh* morph = dynamic_cast<zCMorphMesh*>(source))
    {
        particle->emitter->shpProgMesh = morph->morphMesh;
        if (morph->morphMesh)
            morph->morphMesh->AddRef();
    }
}

void oCVisualFX::ReleasePFXMesh()
{
    if (!emAdjustShpToOrigin || !bPfxMeshSetByVisualFX || !visual)
        return;
    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
    if (!particle || !particle->emitter)
        return;
    if (particle->emitter->shpMesh)
    {
        particle->emitter->shpMesh->Release();
        particle->emitter->shpMesh = NULL;
    }
    if (particle->emitter->shpProgMesh)
    {
        particle->emitter->shpProgMesh->Release();
        particle->emitter->shpProgMesh = NULL;
    }
    if (particle->emitter->shpModel)
    {
        particle->emitter->shpModel->Release();
        particle->emitter->shpModel = NULL;
    }
}

void oCVisualFX::SetPFXShapeVisual(zCVisual* shape, int killAfter)
{
    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(visual);
    if (!particle || !particle->emitter || ((!killAfter && !emAdjustShpToOrigin) ||
        !bPfxMeshSetByVisualFX))
        return;

    zCMorphMesh* morph = dynamic_cast<zCMorphMesh*>(shape);
    if (particle->emitter->shpMesh == shape || particle->emitter->shpProgMesh == shape ||
        particle->emitter->shpModel == shape ||
        (morph && particle->emitter->shpProgMesh == morph->morphMesh))
        return;
    ReleasePFXMesh();
    if (!shape)
        return;

    if (dynamic_cast<zCMesh*>(shape))
    {
        particle->emitter->shpMesh = (zCMesh*)shape;
        shape->AddRef();
    }
    else if (dynamic_cast<zCProgMeshProto*>(shape))
    {
        particle->emitter->shpProgMesh = (zCProgMeshProto*)shape;
        shape->AddRef();
    }
    else if (dynamic_cast<zCModel*>(shape))
    {
        particle->emitter->shpModel = (zCModel*)shape;
        shape->AddRef();
    }
    else if (morph)
    {
        particle->emitter->shpProgMesh = morph->morphMesh;
        if (morph->morphMesh)
            morph->morphMesh->AddRef();
    }
}

void oCVisualFXAI::DoAI(zCVob*, int& deleteAI)
{
    deleteAI = delete_it;
}

// EXE: 0x0049A230
zCObject* oCVisualFX::_CreateNewInstance()
{
    oCVisualFX* visual = new oCVisualFX;
    zCClassDef::ObjectCreated(visual, classDef);
    return visual;
}

static zCConsole* fxEditConsole;
static zCConsole* emEditConsole;

void oCVisualFX::Edit()
{
    actFX = this;
    zcon.Hide();
    zcon.Register("VFX_REPARSE", "reparses the vfx scripts");
    delete fxEditConsole;
    fxEditConsole = new zCConsole;
    fxEditConsole->SetPos(0, 0);
    fxEditConsole->SetParser(fxParser);
    fxEditConsole->SetChangedFunc(FxConsole_ParameterChanged);
    fxEditConsole->AddEvalFunc(FxConsole_EvalFunc);
    fxEditConsole->AddEvalFunc(EmConsole_EvalFunc);
    fxEditConsole->Register("VFX_REPARSE", "reparses the vfx scripts");
    fxEditConsole->Register("EDIT FX", "edits a special effect");
    fxEditConsole->Register("VFX_RESETKEY", "resets act emitter key");
    fxEditConsole->Register("VFX_RESETALL", "resets all emitter keys");
    fxEditConsole->Register("VFX_SETDRAWBBOX", "shows all bboxes");
    fxEditConsole->Register("VFX_SETDRAWTRJ", "shows trajectories");
    fxEditConsole->Register("VFX_SHOWTARGET", "shows targets");
    fxEditConsole->EditInstance("CFX_BASE", this);
    if (emKeyList.numInArray)
        emKeyList[0]->Edit();
    fxEditConsole->SetFocus();
}

void oCVisualFX::CreateNewScriptFX(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();
    zoptions->ChangeDir(1);
    zFILE* file = zfactory->CreateZFile("System\\VisualFx\\VisualFxInst.d");
    file->Open(0);
    file->Append();
    file->Write(zSTRING("\nINSTANCE ") + upperName + "(CFx_Base)\n");
    file->Write("{\n");
    file->Write("};\n");
    file->Close();
    delete file;
    InitParser();
}

void oCEmitterKey::Edit()
{
    delete emEditConsole;
    emEditConsole = new zCConsole;
    emEditConsole->SetPos(0x1004, 0);
    emEditConsole->SetParser(oCVisualFX::fxParser);
    emEditConsole->SetChangedFunc(oCVisualFX::EmConsole_ParameterChanged);
    emEditConsole->AddEvalFunc(oCVisualFX::FxConsole_EvalFunc);
    emEditConsole->AddEvalFunc(oCVisualFX::EmConsole_EvalFunc);
    emEditConsole->Register("EDIT FX", "edits a special effect");
    emEditConsole->Register("VFX_RESETKEY", "resets act emitter key");
    emEditConsole->Register("VFX_RESETALL", "resets all emitter keys");
    emEditConsole->Register("VFX_SETDRAWBBOX", "shows all bboxes");
    emEditConsole->Register("VFX_SETDRAWTRJ", "shows trajectories");
    emEditConsole->Register("VFX_SHOWTARGET", "shows targets");
    emEditConsole->Register("VFX_REPARSE", "reparses the vfx scripts");
    emEditConsole->EditInstance("C_PARTICLEFXEMITKEY", this);
    emEditConsole->SetFocus();
}

void oCEmitterKey::CreateNewScriptKey(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();
    zoptions->ChangeDir(1);
    zFILE* file = zfactory->CreateZFile("System\\VisualFx\\VisualFxInst.d");
    file->Open(0);
    file->Append();
    file->Write(zSTRING("\nINSTANCE ") + upperName + "(C_ParticleFXEmitKey)\n");
    file->Write("{\n");
    file->Write("};\n");
    file->Close();
    delete file;
    oCVisualFX::InitParser();
}

oCVisualFX* oCVisualFX::actFX;
static int drawBBox;
static int drawTrj;
static float playTime;

void oCVisualFX::FxConsole_ParameterChanged(const zSTRING& parameter)
{
    zSTRING name = parameter;
    name.Upper();
    if (name == "VFX_REPARSE")
    {
        InitParser();
        if (fxEditConsole)
            fxEditConsole->SetParser(fxParser);
        if (emEditConsole)
            emEditConsole->SetParser(fxParser);
        return;
    }
    if (!actFX)
        return;

    if (name == "EMTRJNUMKEYS")
    {
        while (actFX->emKeyList.numInArray < actFX->emTrjNumKeys)
        {
            oCEmitterKey* key = new oCEmitterKey;
            key->SetDefaultByFX(actFX);
            key->name = actFX->fxName + "_KEY" + zSTRING(actFX->emKeyList.numInArray + 1);
            actFX->emKeyList.InsertEnd(key);
        }
        while (actFX->emKeyList.numInArray > actFX->emTrjNumKeys)
        {
            int index = actFX->emKeyList.numInArray - 1;
            oCEmitterKey* key = actFX->emKeyList[index];
            actFX->emKeyList.RemoveOrderIndex(index);
            delete key;
        }
        actFX->ParseStrings();
        actFX->CalcTrajectory(actFX->emTrjDynUpdateTargetOnly);
    }
    else if (name == "VISNAME")
    {
        if (!actFX->visual)
            actFX->SetupEmitterKeysByVisual();
        actFX->visName_S.Upper();
        actFX->SetVisualByString(actFX->visName_S);
    }
    else if (name == "LIGHTPRESETNAME")
    {
        if (actFX->light)
        {
            actFX->light->RemoveVobFromWorld();
            actFX->light->Release();
            actFX->light = NULL;
        }
        if (!actFX->lightPresetName.IsEmpty())
        {
            actFX->lightPresetName.Upper();
            actFX->light = new zCVobLight;
            actFX->light->SetByPreset(actFX->lightPresetName);
            actFX->light->lightData.m_bCanMove = 1;
            actFX->light->SetPositionWorld(actFX->trafoObjToWorld.GetTranslation());
            actFX->homeWorld->AddVobAsChild(actFX->light, (zCVob*)actFX);
        }
    }
    else if (name == "SFXID")
    {
        if (actFX->sfx)
        {
            actFX->sfx->Release();
            actFX->sfx = NULL;
            actFX->sfxHnd = 0;
        }
        if (!actFX->sfxID.IsEmpty())
        {
            actFX->sfx = zsound->LoadSoundFXScript(actFX->sfxID);
            zTSound3DParams params;
            params.SetDefaults();
            params.isAmbient3D = actFX->sfxIsAmbient;
            actFX->sfxHnd = zsound->PlaySound3D(actFX->sfx, (zCVob*)actFX, 0, &params);
        }
    }
    else if (name == "EMCHECKCOLLISION")
        actFX->SetCollisionEnabled(actFX->emCheckCollision);
    else if (name == "EMTRJMODE_S")
    {
        actFX->ParseStrings();
        actFX->CalcTrajectory(actFX->emTrjDynUpdateTargetOnly);
    }

    if (!actFX->target && zCCamera::activeCam && zCCamera::activeCam->connectedVob)
    {
        zCVob* cameraVob = zCCamera::activeCam->connectedVob;
        actFX->targetPos = cameraVob->GetPositionWorld() +
            cameraVob->trafoObjToWorld.GetAtVector() * 400.0f;
    }
    int flags = 0;
    actFX->CalcTrajectory(flags);
}

void oCVisualFX::EmConsole_ParameterChanged(const zSTRING& parameter)
{
    zSTRING name = parameter;
    name.Upper();
    if (actFX && actFX->actKey && name == "SFXID")
    {
        if (actFX->actKey->sfx)
        {
            actFX->actKey->sfx->Release();
            actFX->actKey->sfx = NULL;
            actFX->actKey->sfxHnd = 0;
        }
        if (!actFX->actKey->sfxID.IsEmpty())
            actFX->actKey->sfx = zsound->LoadSoundFXScript(actFX->actKey->sfxID);
    }
    if (actFX && actFX->actKey)
    {
        actFX->actKey->ParseStrings();
        actFX->UpdateFXByEmitterKey(actFX->actKey);
        int flags = 0;
        actFX->CalcTrajectory(flags);
    }
}

int oCVisualFX::FxConsole_EvalFunc(const zSTRING& input, zSTRING& message)
{
    if (input.Search("VFX_REPARSE", 1) >= 0)
    {
        InitParser();
        if (fxEditConsole)
            fxEditConsole->SetParser(fxParser);
        if (emEditConsole)
            emEditConsole->SetParser(fxParser);
        return 1;
    }
    if (input.Search("VFX_SETDRAWBBOX", 1) >= 0)
    {
        drawBBox = !drawBBox;
        message = drawBBox ? "enabled drawing fx bounding boxes" : "disabled drawing fx bounding boxes";
        return 1;
    }
    if (input.Search("VFX_SETDRAWTRJ", 1) >= 0)
    {
        drawTrj = !drawTrj;
        message = drawTrj ? "enabled drawing fx trajectory" : "disabled drawing fx trajectory";
        return 1;
    }
    if (input.Search("VFX_SHOWTARGET", 1) >= 0)
    {
        showTarget = !showTarget;
        message = showTarget ? "bbox for vfx target activated" : "bbox for vfx target deactivated";
        return 1;
    }

    zSTRING command = input.PickWord(1, " ", "");
    command.Upper();
    if (command == "EDIT")
    {
        zSTRING fxName = input.PickWord(2, " ", "");
        fxName.Upper();
        if (fxName.IsEmpty())
        {
            message = "fx id invalid";
            return 1;
        }
        if (actFX && actFX->fxName == fxName)
        {
            actFX->Edit();
            return 1;
        }
        if (actFX)
        {
            reinterpret_cast<zCVob*>(actFX)->RemoveVobFromWorld();
            actFX->Release();
            message = "deleted old fx";
        }
        actFX = new oCVisualFX;
        if (fxParser->GetSymbol(fxName))
            actFX->SetByScript(fxName);
        else
        {
            actFX->fxName = fxName;
            CreateNewScriptFX(fxName);
            InitParser();
            actFX->SetByScript(fxName);
            message = "unknown fx identifier: " + fxName + " creating new";
        }
        actFX->Open();
        actFX->Edit();
        return 1;
    }
    if (!actFX)
        return 0;
    if (input.Search("VFX_RESETKEY", 1) >= 0)
    {
        if (actFX->actKey)
            actFX->actKey->SetDefaultByFX(actFX);
        message = "act key reseted";
        return 1;
    }
    if (input.Search("VFX_RESETALL", 1) >= 0)
    {
        actFX->SetupEmitterKeysByVisual();
        message = "all keys reseted";
        return 1;
    }
    if (input.Search("VFX_STOP", 1) >= 0)
    {
        reinterpret_cast<zCVob*>(actFX)->RemoveVobFromWorld();
        actFX->Release();
        actFX = NULL;
        message = "deleted";
        return 1;
    }
    return 0;
}

int oCVisualFX::EmConsole_EvalFunc(const zSTRING& input, zSTRING& message)
{
    if (!actFX)
    {
        message = "no act fx!";
        return 1;
    }
    if (input.Search("VFX_RESETKEY", 1) >= 0 && actFX->actKey)
    {
        actFX->actKey->SetDefaultByFX(actFX);
        message = "act key reseted";
        return 1;
    }
    if (input.Search("VFX_RESETALL", 1) >= 0)
    {
        actFX->SetupEmitterKeysByVisual();
        message = "all keys reseted";
        return 1;
    }
    if (input.Search("PLAY", 1) >= 0)
    {
        actFX->Play(playTime, NULL, NULL);
        return 1;
    }
    if (input.Search("STOP", 1) >= 0)
    {
        actFX->Stop(1);
        actFX = NULL;
        message = "stopped";
        return 1;
    }

    zSTRING command = input.PickWord(1, " ", "");
    command.Upper();
    if (command == "OPEN" || command == "INIT" || command == "INVEST")
    {
        zSTRING suffix = command;
        if (command == "INVEST")
            suffix += "_" + input.PickWord(3, " ", "");
        actFX->ResetForEditing();
        actFX->Open();

        zSTRING keyName = actFX->fxName + "_KEY_" + suffix;
        int keyIndex = actFX->FindKey(suffix);
        oCEmitterKey* key;
        if (keyIndex >= 0)
            key = actFX->emKeyList[keyIndex];
        else
        {
            key = new oCEmitterKey;
            key->name = keyName;
            actFX->emKeyList.InsertEnd(key);
        }
        if (!fxParser->GetSymbol(keyName))
        {
            message = "key not found... creating";
            oCEmitterKey::CreateNewScriptKey(keyName);
            key->SetDefaultByFX(actFX);
            InitParser();
        }
        actFX->actKey = key;
        return 1;
    }
    return FxConsole_EvalFunc(input, message);
}

void oCEmitterKey::SetDefaultByFX(oCVisualFX* fx)
{
    if (fx->fxBackup)
        fx->UpdateFXByEmitterKey(fx->fxBackup);
    pfx_scTime = 1.0f;
    visName_S = fx->visName_S;
    lightPresetName = fx->lightPresetName;
    lightRange = fx->light ? fx->light->lightData.range : 0.0f;
    emFlyGravity = fx->emFlyGravity;
    emSelfRotVel_S = fx->emSelfRotVel_S;
    emTrjEaseVel = fx->emTrjEaseVel;
    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(fx->visual);
    if (particle && particle->emitter)
    {
        zCParticleEmitter* emitter = particle->emitter;
        visSizeScale = emitter->visSizeEndScale;
        pfx_ppsValue = emitter->ppsValue;
        pfx_ppsIsSmoothChg = 0;
        pfx_ppsIsLoopingChg = 0;
        pfx_shpIsVolumeChg = 0;
        pfx_shpDistribWalkSpeed = emitter->shpDistribWalkSpeed;
        pfx_velAvg = emitter->velAvg;
        pfx_lspPartAvg = emitter->lspPartAvg;
        pfx_visAlphaStart = emitter->visAlphaStart;
        pfx_flyGravity_S = emitter->flyGravity_S;
        pfx_shpOffsetVec_S = emitter->shpOffsetVec_S;
        pfx_shpDistribType_S = emitter->shpDistribType_S;
        pfx_dirMode_S = emitter->dirMode_S;
        pfx_dirFOR_S = emitter->dirFOR_S;
        pfx_dirModeTargetFOR_S = emitter->dirModeTargetFOR_S;
        pfx_dirModeTargetPos_S = emitter->dirModeTargetPos_S;
        pfx_shpIsVolumeChg = emitter->shpIsVolume;
        pfx_shpScaleFPS = emitter->shpScaleFPS;
        pfx_shpDim_S = emitter->shpDim_S;
    }
    ParseStrings();
}

void oCVisualFX::SetupEmitterKeysByVisual()
{
    for (int i = 0; i < emKeyList.numInArray; ++i)
        emKeyList[i]->SetDefaultByFX(this);
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->SetupEmitterKeysByVisual();
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->SetupEmitterKeysByVisual();
}

void oCVisualFX::ResetForEditing()
{
    if (light)
        light->RemoveVobFromWorld();
    if (sfx && sfxHnd)
        zsound->StopSound(sfxHnd);
    if (fxInvestOrigin)
        fxInvestOrigin->ResetForEditing();
    if (fxInvestTarget)
        fxInvestTarget->ResetForEditing();
    for (int i = 0; i < fxList.numInArray; ++i)
        fxList[i]->ResetForEditing();
    for (int i = 0; i < childList.numInArray; ++i)
        childList[i]->ResetForEditing();
    collisionTime = 0.0f;
    deleteTime = 0.0f;
    nextLevelTime = 0.0f;
    damageTime = 0.0f;
    easeTime = 0.0f;
    trjUpdateTime = 0.0f;
    levelTime = 0.0f;
    lifeSpanTimer = 0.0f;
    fxState = 0;
    collisionOccured &= 0xfc1fffff;
}

oCVisFX_MultiTarget::oCVisFX_MultiTarget()
{
}

oCVisFX_MultiTarget::~oCVisFX_MultiTarget()
{
    for (int i = 0; i < visList.numInArray; ++i)
    {
        visList[i]->Stop(1);
        visList[i]->Release();
    }
}

zCClassDef* oCVisFX_MultiTarget::_GetClassDef() const
{
    return classDef;
}

void oCVisFX_MultiTarget::Play(float, const zMAT4*, const zMAT4*)
{
}

void oCVisFX_MultiTarget::Open()
{
    for (int i = 0; i < visList.numInArray; ++i)
        visList[i]->Open();
    oCVisualFX::Open();
}

void oCVisFX_MultiTarget::SetLevel(int level, int force)
{
    for (int i = 0; i < visList.numInArray; ++i)
        visList[i]->SetLevel(level, force);
    oCVisualFX::SetLevel(level, force);
}

void oCVisFX_MultiTarget::Init(zCArray<zCVob*>& targets)
{
    int sameTargets = vobList.numInArray == targets.numInArray;
    if (sameTargets)
    {
        for (int i = 0; i < targets.numInArray; ++i)
        {
            if (vobList[i] != targets[i])
            {
                sameTargets = 0;
                break;
            }
        }
    }
    if (sameTargets)
        return;

    for (int i = visList.numInArray - 1; i >= 0; --i)
    {
        oCVisualFX* fx = visList[i];
        visList.RemoveOrderIndex(i);
        fx->Stop(1);
        fx->Release();
    }
    if (!targets.numInArray)
    {
        oCVisualFX::Init((zCVob*)NULL, (zCVob*)NULL, 0);
        return;
    }
    if (targets.numInArray == 1)
    {
        oCVisualFX::Init(targets[0], NULL, 0);
        return;
    }
    for (int i = 1; i < targets.numInArray; ++i)
    {
        oCVisualFX* fx = new oCVisualFX;
        fx->root = fx;
        fx->parent = fx;
        zSTRING number;
        number = GetLevel();
        zSTRING subName = fxName + "_SUB" + number;
        fx->SetByScript(fxParser->GetSymbol(subName) ? subName : fxName + "_SUB");
        fx->Init(targets[0], targets[i], inflictor);
        if (GetLevel())
            fx->SetLevel(GetLevel(), 0);
        fx->AddRef();
        visList.InsertEnd(fx);
    }
    oCVisualFX::Init(targets);
}

void oCVisFX_MultiTarget::InvestNext()
{
    for (int i = 0; i < visList.numInArray; ++i)
        visList[i]->InvestNext();
    oCVisualFX::InvestNext();
}

void oCVisFX_MultiTarget::Cast(int killAfter)
{
    for (int i = visList.numInArray - 1; i >= 0; --i)
    {
        oCVisualFX* fx = visList[i];
        visList.RemoveIndex(i);
        fx->Cast(killAfter);
        fx->Release();
    }
    oCVisualFX::Cast(killAfter);
}

void oCVisFX_MultiTarget::Stop(int killAfter)
{
    for (int i = visList.numInArray - 1; i >= 0; --i)
    {
        oCVisualFX* fx = visList[i];
        visList.RemoveIndex(i);
        fx->Stop(killAfter);
        fx->Release();
    }
    oCVisualFX::Stop(killAfter);
}

void oCVisFX_MultiTarget::SetByScript(const zSTRING& instanceName)
{
    zSTRING upperName = instanceName;
    upperName.Upper();
    oCVisualFX::SetByScript(upperName);
}

void oCVisFX_MultiTarget::Reset()
{
    for (int i = 0; i < visList.numInArray; ++i)
        visList[i]->Reset();
    oCVisualFX::Reset();
}

int oCVisFX_MultiTarget::IsFinished()
{
    for (int i = 0; i < visList.numInArray; ++i)
        if (!visList[i]->IsFinished())
            return 0;
    return oCVisualFX::IsFinished() != 0;
}

void oCVisFX_MultiTarget::SetDamage(float value)
{
    for (int i = 0; i < visList.numInArray; ++i)
        visList[i]->SetDamage(value);
    damage = value;
}

void oCVisFX_MultiTarget::SetDamageType(int type)
{
    for (int i = 0; i < visList.numInArray; ++i)
        visList[i]->SetDamageType(type);
    damageType = type;
}

void oCVisFX_MultiTarget::Archive(zCArchiver&)
{
}

void oCVisFX_MultiTarget::Unarchive(zCArchiver&)
{
}

// EXE: 0x0049F750
zCObject* oCVisFX_MultiTarget::_CreateNewInstance()
{
    oCVisFX_MultiTarget* visual = new oCVisFX_MultiTarget;
    zCClassDef::ObjectCreated(visual, classDef);
    return visual;
}

oCVisualFX* oCVisualFX::CreateAndPlay(const zSTRING& name, const zVEC3& position,
    zCVob* target, int killAfter, float time, int flags, int slot)
{
    zSTRING upperName = name;
    upperName.Upper();
    if (upperName.IsEmpty())
        return NULL;
    if (!fxParser->GetSymbol(upperName))
    {
        zerr->Report(zERR_TYPE_WARN, 0, "C: oCVisualFX::CreateAndPlay(): VisualFX ID " + name +
            " not found in visualfxinst.d", 0, 0, 0x6C9, 0, 0);
        return NULL;
    }
    zCVob* origin = new zCVob;
    origin->SetVobName("_VFX");
    origin->SetPositionWorld(position);
    origin->SetSleeping(1);
    if (zCCamera::activeCam && zCCamera::activeCam->connectedVob &&
        zCCamera::activeCam->connectedVob->homeWorld)
        zCCamera::activeCam->connectedVob->homeWorld->AddVob(origin);

    oCVisualFX* fx = new oCVisualFX;
    fx->SetDuration(time);
    fx->root = fx;
    fx->parent = fx;
    fx->SetByScript(upperName);
    fx->SetOrigin(origin);
    fx->SetTarget(target);
    fx->SetInflictor(target);
    fx->Init(origin, target, 0);
    fx->Cast(killAfter);
    return fx;
}
