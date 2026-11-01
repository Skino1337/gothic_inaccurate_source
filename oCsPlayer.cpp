#include "include/oCsPlayer.h"

static int classDefState = -1;
static zSTRING PLAYER_MOB_MISSING_KEY             = "PLAYER_MOB_MISSING_KEY";
static zSTRING PLAYER_MOB_MISSING_LOCKPICK        = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING PLAYER_MOB_MISSING_KEY_OR_LOCKPICK = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING PLAYER_MOB_NEVER_OPEN              = "PLAYER_MOB_NEVER_OPEN";
static zSTRING PLAYER_MOB_TOO_FAR_AWAY            = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING PLAYER_MOB_WRONG_SIDE              = "PLAYER_MOB_WRONG_SIDE";
static zSTRING PLAYER_MOB_MISSING_ITEM            = "PLAYER_MOB_MISSING_ITEM";
static zSTRING PLAYER_MOB_ANOTHER_IS_USING        = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING PLAYER_PLUNDER_IS_EMPTY            = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING PLAYER_RANGED_NO_AMMO              = "PLAYER_RANGED_NO_AMMO";

static zCClassDef oCCSPlayerClassDef("oCCSPlayer", "zCCSPlayer",
    oCCSPlayer::_CreateNewInstance, 0, 0x68, 0, "");
zCClassDef* oCCSPlayer::classDef = &oCCSPlayerClassDef;

void zCCSPlayer::ProcessCutscene(zCCSCutsceneContext*, const zVEC3&)
{
}

void zCCSPlayer::OnCSTrigger(const zSTRING&, zCVob*)
{
}

void zCCSPlayer::OnCSUntrigger(const zSTRING&, zCVob*)
{
}

oCCSPlayer::oCCSPlayer()
{
}

oCCSPlayer::~oCCSPlayer()
{
}

zCObject* oCCSPlayer::_CreateNewInstance()
{
    oCCSPlayer* player = new oCCSPlayer;
    zCClassDef::ObjectCreated(player, classDef);
    return player;
}

void oCCSPlayer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCCSPlayer::_GetClassDef() const
{
    return classDef;
}

void oCCSPlayer::ProcessCutscene(zCCSCutsceneContext* context, const zVEC3& position)
{
    zCCSPlayer::ProcessCutscene(context, position);
    switch (context->lastPlayResult) {
    case CS_PLAY_STARTEDPLAYING:
        if (zerr->filter_level >= 7)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene started playing", 7, 0, 0x27, 0, 0);
        if (context->properties && context->properties->globalCutscene) {
            oCWorld* world = dynamic_cast<oCWorld*>(ownerWorld);
            if (world)
                world->ClearNpcPerceptionVobLists();
        }
        break;
    case CS_PLAY_STOPPED:
        if (zerr->filter_level >= 7)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene stopped", 7, 0, 0x2D, 0, 0);
        break;
    case CS_PLAY_INTERRUPTED:
        if (zerr->filter_level >= 7)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene interrupted", 7, 0, 0x32, 0, 0);
        break;
    case CS_PLAY_RESUMED:
        if (zerr->filter_level >= 7)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene resumed", 7, 0, 0x37, 0, 0);
        break;
    }
}
