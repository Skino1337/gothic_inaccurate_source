#include "include/zVob.h"
#include "include/zBuffer.h"
#include "include/zCsMessages.h"
#include "include/zCsCamera.h"
#include "include/oObjFactory.h"

static int classDefState = -1;
static zCClassDef zCCSBlockBaseClassDef("zCCSBlockBase", "zCObject", NULL, 0, 0x24, 0, "");
zCClassDef* zCCSBlockBase::classDef = &zCCSBlockBaseClassDef;
static zCClassDef zCCSAtomicBlockClassDef("zCCSAtomicBlock", "zCCSBlockBase",
    zCCSAtomicBlock::_CreateNewInstance, 0, 0x34, 0, "");
zCClassDef* zCCSAtomicBlock::classDef = &zCCSAtomicBlockClassDef;
static zCClassDef zCCSBlockClassDef("zCCSBlock", "zCCSBlockBase",
    zCCSBlock::_CreateNewInstance, 0, 0x44, 0, "");
zCClassDef* zCCSBlock::classDef = &zCCSBlockClassDef;
static zCClassDef zCCSSyncBlockClassDef("zCCSSyncBlock", "zCCSBlock",
    zCCSSyncBlock::_CreateNewInstance, 0, 0x50, 0, "");
zCClassDef* zCCSSyncBlock::classDef = &zCCSSyncBlockClassDef;

zCCSBlockBase::~zCCSBlockBase()
{
}

void zCCSBlockBase::Archive(zCArchiver&)
{
}

void zCCSBlockBase::Unarchive(zCArchiver&)
{
}

zCCSBlockPosition::zCCSBlockPosition(zCCSBlockBase* blockValue)
    : position(0.0f), block(blockValue)
{
}

zCCSBlockPosition::zCCSBlockPosition(float positionValue, zCCSBlockBase* blockValue)
    : position(positionValue), block(blockValue)
{
}

zCCSBlockPosition::~zCCSBlockPosition()
{
}

zCCSBlockPosition& zCCSBlockPosition::operator=(const zCCSBlockPosition& other)
{
    position = other.position;
    block = other.block;
    return *this;
}

void zCCSBlockBase::InsertBlock(zCCSBlockBase*, int)
{
}

zCCSBlockBase* zCCSBlockBase::GetChild(int)
{
    return NULL;
}

int zCCSBlockBase::NewBlock(int, int)
{
    return 0;
}

int zCCSBlockBase::GetNumOfChilds()
{
    return 0;
}

int zCCSBlockBase::GetRoleNumOfChild(int)
{
    return -1;
}

void zCCSBlockBase::BlockCorrection()
{
}

void zCCSBlockBase::BlockAsFocus(int)
{
}

zSTRING zCCSBlockBase::GetRoleName()
{
    return zSTRING("");
}

void zCCSBlockBase::SetRoleName(zSTRING&)
{
}

void zCCSBlockBase::DeleteChild(int)
{
}

zCCSBlockPosition* zCCSBlockBase::GetChildPos(int)
{
    return NULL;
}

zCClassDef* zCCSBlockBase::_GetClassDef() const
{
    return classDef;
}

zCCSAtomicBlock::zCCSAtomicBlock()
    : roleVob(NULL), commandRef(NULL), command(NULL), commandState(0)
{
}

void zCCSAtomicBlock::Archive(zCArchiver& archiver)
{
    archiver.WriteObject(command);
}

void zCCSAtomicBlock::Unarchive(zCArchiver& archiver)
{
    command = dynamic_cast<zCEventMessage*>(archiver.ReadObject(NULL));
}

zCCSAtomicBlock::~zCCSAtomicBlock()
{
    if (command) {
        command->Release();
        command = NULL;
    }
    if (commandRef) {
        commandRef->Release();
        commandRef = NULL;
    }
}

void zCCSAtomicBlock::OrganizeRoles(zCArray<zCVob*>& vobs, zCArray<zCCSRole*>& roles)
{
    commandState = 0;
    if (!command)
        return;
    zSTRING referenceName = command->MD_GetVobRefName();
    if (referenceName.IsEmpty())
        return;
    referenceName.Upper();
    for (int i = 0; i < vobs.numInArray; ++i) {
        int matches = 0;
        if (i < roles.numInArray && roles.parray[i])
            matches = roles.parray[i]->roleName == referenceName;
        if (!matches && vobs.parray[i])
            matches = vobs.parray[i]->GetObjectName() == referenceName;
        if (matches) {
            roleVob = vobs.parray[i];
            return;
        }
    }
    commandState = 2;
}

void zCCSAtomicBlock::Play(const zCArray<zCEventManager*>& managers)
{
    Play(managers.parray[0]);
}

void zCCSAtomicBlock::Play(const zCArray<zCEventManager*>& managers, float first, float second)
{
    Play(managers.parray[0], first, second);
}

void zCCSAtomicBlock::Play(zCEventManager* manager)
{
    if (!manager || IsFinished(manager))
        commandState = 2;
    if (commandState || !command)
        return;
    if (commandRef) {
        commandRef->Release();
        commandRef = NULL;
    }
    commandRef = static_cast<zCEventMessage*>(command->CreateCopy());
    commandRef->AddRef();
    commandRef->MD_SetVobParam(roleVob);
    manager->OnMessage(commandRef, NULL);
    commandState = 1;
}

void zCCSAtomicBlock::Play(zCEventManager*, float, float)
{
}

float zCCSAtomicBlock::GetMinTime()
{
    return command ? command->MD_GetMinTime() : 0.0f;
}

int zCCSAtomicBlock::IsFinished(zCEventManager* manager)
{
    if (!commandState)
        return 0;
    if (commandState == 2)
        return 1;
    if (manager && commandRef && manager->IsRunning(commandRef) &&
        commandRef->MD_GetTimeBehavior() != zCEventMessage::TBAssembled)
        return 0;
    commandState = 2;
    if (commandRef) {
        commandRef->Release();
        commandRef = NULL;
    }
    return 1;
}

zCObject* zCCSAtomicBlock::_CreateNewInstance()
{
    zCCSAtomicBlock* block = new zCCSAtomicBlock;
    zCClassDef::ObjectCreated(block, classDef);
    return block;
}

void zCCSAtomicBlock::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSAtomicBlock::_GetClassDef() const
{
    return classDef;
}

int zCCSAtomicBlock::IsFinished(const zCArray<zCEventManager*>&)
{
    return 1;
}

zCEventMessage* zCCSAtomicBlock::GetCommand()
{
    return command;
}

void zCCSBlock::Archive(zCArchiver& archiver)
{
    archiver.WriteString(roleName);
    archiver.WriteInt(blocks.numInArray);
    for (int i = 0; i < blocks.numInArray; ++i) {
        archiver.WriteFloat(blocks.parray[i].position);
        archiver.WriteObject(blocks.parray[i].block);
    }
}

void zCCSBlock::Unarchive(zCArchiver& archiver)
{
    roleName = archiver.ReadString();
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i) {
        zCCSBlockPosition position;
        blocks.InsertEnd(position);
        blocks.parray[i].position = archiver.ReadFloat();
        blocks.parray[i].block = dynamic_cast<zCCSBlockBase*>(archiver.ReadObject(NULL));
    }
}

void zCCSSyncBlock::Archive(zCArchiver& archiver)
{
    zCCSBlock::Archive(archiver);
    archiver.WriteInt(roleAss.numInArray);
    for (int i = 0; i < roleAss.numInArray; ++i)
        archiver.WriteInt(roleAss.parray[i]);
}

void zCCSSyncBlock::Unarchive(zCArchiver& archiver)
{
    zCCSBlock::Unarchive(archiver);
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
        roleAss.InsertEnd(archiver.ReadInt());
}

void zCCutscene::Archive(zCArchiver& archiver)
{
    zCCSBlock::Archive(archiver);
    archiver.WriteObject(properties);
    archiver.WriteInt(roleList.numInArray);
    for (int i = 0; i < roleList.numInArray; ++i)
        archiver.WriteObject(roleList.parray[i]);
    archiver.WriteInt(roleVobList.numInArray);
    for (int i = 0; i < roleVobList.numInArray; ++i)
        archiver.WriteObject(roleVobList.parray[i]);
    archiver.WriteObject(mainRoleVob);
}

void zCCutscene::Unarchive(zCArchiver& archiver)
{
    zCCSBlock::Unarchive(archiver);
    ClearCutscene();
    properties = dynamic_cast<zCCSProps*>(archiver.ReadObject(NULL));
    if (properties)
        SetObjectName(properties->GetName());
    int roleCount = archiver.ReadInt();
    for (int i = 0; i < roleCount; ++i)
        roleList.InsertEnd(dynamic_cast<zCCSRole*>(archiver.ReadObject(NULL)));
    int vobCount = archiver.ReadInt();
    for (int i = 0; i < vobCount; ++i) {
        roleVobList.InsertEnd(dynamic_cast<zCVob*>(archiver.ReadObject(NULL)));
        roleEvMList.InsertEnd(NULL);
    }
    mainRoleVob = dynamic_cast<zCVob*>(archiver.ReadObject(NULL));
}

void zCCSBlock::InsertBlock(zCCSBlockBase* block, int index)
{
    zCCSBlockPosition position;
    position.block = block;
    position.position = (float)index;
    blocks.InsertEnd(position);
}

int zCCSBlock::NewBlock(int, int)
{
    blocks.InsertEnd(zCCSBlockPosition());
    return blocks.numInArray - 1;
}

void zCCSBlock::BlockAsFocus(int index)
{
    zCCSBlockPosition position = blocks.parray[0];
    blocks.parray[0] = blocks.parray[index];
    blocks.parray[index] = position;
}

void zCCSBlock::BlockCorrection()
{
    if (blocks.numInArray < 1)
        return;
    float firstTime = blocks.parray[0].block->GetMinTime();
    if (firstTime <= 0.0f)
        firstTime = 1.0f;
    int focus = 0;
    for (int i = 1; i < blocks.numInArray; ++i) {
        if (blocks.parray[i].position > blocks.parray[focus].position)
            focus = i;
    }
    float focusTime = blocks.parray[focus].block->GetMinTime();
    if (focusTime <= 0.0f)
        focusTime = 1.0f;
    for (int i = 0; i < blocks.numInArray; ++i)
        blocks.parray[i].position = firstTime * blocks.parray[i].position / focusTime;
    BlockAsFocus(focus);
    float firstPosition = blocks.parray[0].position;
    for (int i = 0; i < blocks.numInArray; ++i)
        blocks.parray[i].position -= firstPosition;
}

void zCCSBlock::Play(zCEventManager* manager)
{
    for (int i = 0; i < blocks.numInArray; ++i) {
        if (blocks.parray[i].position >= 0.0f)
            blocks.parray[i].block->Play(manager);
    }
}

void zCCSBlock::Play(zCEventManager* manager, float first, float second)
{
    for (int i = 0; i < blocks.numInArray; ++i) {
        float blockTime = blocks.parray[0].block->GetMinTime();
        if (blockTime <= 0.0f)
            blockTime = 1.0f;
        blockTime *= blocks.parray[i].position;
        if (first > blockTime && blockTime <= second)
            blocks.parray[i].block->Play(manager);
        if (first >= blockTime)
            blocks.parray[i].block->Play(manager, first, second);
    }
}

void zCCSBlock::Play(const zCArray<zCEventManager*>&)
{
    zerr->Message("zCCSBlock::Play(Array): not allowed!");
}

void zCCSBlock::Play(const zCArray<zCEventManager*>&, float, float)
{
    zerr->Message("zCCSBlock::Play(Array): not allowed!");
}

float zCCSBlock::GetMinTime()
{
    if (blocks.numInArray <= 0)
        return 0.0f;
    const float firstTime = blocks.parray[0].block->GetMinTime();
    float minTime = firstTime;
    for (int i = 1; i < blocks.numInArray; ++i) {
        const float blockTime = blocks.parray[i].block->GetMinTime()
            + firstTime * blocks.parray[i].position;
        if (blockTime > minTime)
            minTime = blockTime;
    }
    return minTime;
}

int zCCSBlock::IsFinished(zCEventManager* manager)
{
    for (int i = 0; i < blocks.numInArray; ++i) {
        if (!blocks.parray[i].block->IsFinished(manager))
            return 0;
    }
    return 1;
}

int zCCSBlock::IsFinished(const zCArray<zCEventManager*>&)
{
    zerr->Message("zCCSBlock::IsFinished(Array): not allowed!");
    return 1;
}

void zCCSBlock::OrganizeRoles(zCArray<zCVob*>& vobs, zCArray<zCCSRole*>& roles)
{
    for (int i = 0; i < blocks.numInArray; ++i)
        blocks.parray[i].block->OrganizeRoles(vobs, roles);
}

zCCSBlock::~zCCSBlock()
{
    for (int i = 0; i < blocks.numInArray; ++i) {
        if (blocks.parray[i].block) {
            blocks.parray[i].block->Release();
            blocks.parray[i].block = NULL;
        }
    }
}

zCCSBlockBase* zCCSBlock::GetChild(int index)
{
    if (index < 0 || index >= blocks.numInArray)
        return NULL;
    return blocks.parray[index].block;
}

zCCSBlockPosition* zCCSBlock::GetChildPos(int index)
{
    if (index < 0 || index >= blocks.numInArray)
        return NULL;
    return &blocks.parray[index];
}

int zCCSBlock::GetNumOfChilds()
{
    return blocks.numInArray;
}

void zCCSBlock::DeleteChild(int index)
{
    if (index < 0 || index >= blocks.numInArray)
        return;
    if (blocks.parray[index].block) {
        blocks.parray[index].block->Release();
        blocks.parray[index].block = NULL;
    }
    for (int i = index; i < blocks.numInArray - 1; ++i)
        blocks.parray[i] = blocks.parray[i + 1];
    --blocks.numInArray;
    BlockCorrection();
}

zSTRING zCCSBlock::GetRoleName()
{
    return roleName;
}

void zCCSBlock::SetRoleName(zSTRING& name)
{
    roleName = name;
}

zCObject* zCCSBlock::_CreateNewInstance()
{
    zCCSBlock* block = new zCCSBlock;
    zCClassDef::ObjectCreated(block, classDef);
    return block;
}

void zCCSBlock::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSBlock::_GetClassDef() const
{
    return classDef;
}

zCCSSyncBlock::~zCCSSyncBlock()
{
}

int zCCSSyncBlock::IsFinished(const zCArray<zCEventManager*>& managers)
{
    for (int i = 0; i < blocks.numInArray; ++i) {
        if (!blocks.parray[i].block->IsFinished(managers.parray[roleAss.parray[i]]))
            return 0;
    }
    return 1;
}

void zCCSSyncBlock::Play(zCEventManager* manager)
{
    zCCSBlock::Play(manager);
}

void zCCSSyncBlock::Play(const zCArray<zCEventManager*>& managers)
{
    if (managers.numInArray == 1) {
        zCCSBlock::Play(managers.parray[0]);
        return;
    }
    for (int i = 0; i < blocks.numInArray; ++i) {
        if (blocks.parray[i].position >= 0.0f)
            blocks.parray[i].block->Play(managers.parray[roleAss.parray[i]]);
    }
}

void zCCSSyncBlock::Play(const zCArray<zCEventManager*>& managers, float first, float second)
{
    if (managers.numInArray == 1) {
        zCCSBlock::Play(managers.parray[0], first, second);
        return;
    }
    for (int i = 0; i < blocks.numInArray; ++i) {
        float blockTime = blocks.parray[0].block->GetMinTime();
        if (blockTime <= 0.0f)
            blockTime = 1.0f;
        blockTime *= blocks.parray[i].position;
        if (first > blockTime && blockTime <= second)
            blocks.parray[i].block->Play(managers.parray[roleAss.parray[i]]);
    }
}

int zCCSSyncBlock::GetRoleNumOfChild(int index)
{
    if (index >= roleAss.numInArray)
        return -1;
    return roleAss.parray[index];
}

void zCCSSyncBlock::InsertBlock(zCCSBlockBase* block, int index)
{
    roleAss.InsertEnd(index);
    zCCSBlock::InsertBlock(block, 0);
}

int zCCSSyncBlock::NewBlock(int first, int second)
{
    roleAss.InsertEnd(first);
    return zCCSBlock::NewBlock(first, second);
}

void zCCSSyncBlock::BlockAsFocus(int index)
{
    zCCSBlock::BlockAsFocus(index);
    int role = roleAss.parray[0];
    roleAss.parray[0] = roleAss.parray[index];
    roleAss.parray[index] = role;
}

void zCCSSyncBlock::DeleteChild(int index)
{
    if (index >= 0) {
        for (int i = index; i < roleAss.numInArray - 1; ++i)
            roleAss.parray[i] = roleAss.parray[i + 1];
        if (index < roleAss.numInArray)
            --roleAss.numInArray;
    }
    zCCSBlock::DeleteChild(index);
}

void zCCSSyncBlock::AssignerCorrection(int index)
{
    for (int i = 0; i < roleAss.numInArray; ++i) {
        if (roleAss.parray[i] > index)
            --roleAss.parray[i];
    }
}

zCObject* zCCSSyncBlock::_CreateNewInstance()
{
    zCCSSyncBlock* block = new zCCSSyncBlock;
    zCClassDef::ObjectCreated(block, classDef);
    return block;
}

void zCCSSyncBlock::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSSyncBlock::_GetClassDef() const
{
    return classDef;
}

static zCClassDef zCCutsceneClassDef("zCCutscene", "zCCSBlock",
    zCCutscene::_CreateNewInstance, 3, 0x80, 0, "");
zCClassDef* zCCutscene::classDef = &zCCutsceneClassDef;
static zCClassDef zCCSRoleClassDef("zCCSRole", "zCObject",
    zCCSRole::_CreateNewInstance, 3, 0x44, 0, "");
zCClassDef* zCCSRole::classDef = &zCCSRoleClassDef;
static int zCCSPropsClassDefState = -1;
static zCClassDef zCCSPropsClassDef("zCCSProps", "zCObject",
    zCCSProps::_CreateNewInstance, 3, 0x84, 0, "");
zCClassDef* zCCSProps::classDef = &zCCSPropsClassDef;

const zSTRING CS_PLAYER_ROLENAME = "_PLAYER";
const zSTRING CS_MAIN_ROLENAME = "_MAINROLE";

zCCutscene::zCCutscene()
    : lastCheckResult(0), mainRoleVob(NULL), properties(NULL), ownerPlayer(NULL),
      actualBlockNum(0), actualBlock(NULL)
{
    properties = new zCCSProps;
    zCClassDef::ObjectCreated(properties, zCCSProps::classDef);
}

zCCutscene::~zCCutscene()
{
    Stop();
    ClearCutscene();
    if (ownerPlayer) {
        ownerPlayer->Release();
        ownerPlayer = NULL;
    }
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene destructed.", 8, 0, 0x2FB, 0, 0);
}

zCClassDef* zCCutscene::_GetClassDef() const
{
    return classDef;
}

int zCCutscene::IsCutscene()
{
    return 1;
}

void zCCutscene::SetOwnerPlayer(zCCSPlayer* player)
{
    if (ownerPlayer) {
        ownerPlayer->Release();
        ownerPlayer = NULL;
    }
    ownerPlayer = player;
    if (ownerPlayer)
        ownerPlayer->AddRef();
}

int zCCutscene::IsMember(zCVob* vob)
{
    return roleVobList.IsInList(vob);
}

int zCCutscene::ResumeActBlock()
{
    const int mainRoleActive = mainRoleVob && mainRoleVob->GetEM(0)->GetCutscene() == this;
    for (int i = 0; i < roleVobList.numInArray; ++i) {
        roleEvMList.parray[i] = roleVobList.parray[i]->GetEM(0);
        if (roleEvMList.parray[i]->GetCutscene() && roleEvMList.parray[i]->GetCutscene() != this)
            return 0;
    }
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        if (i >= roleVobList.numInArray || !roleVobList.parray[i])
            continue;
        zCEventManager* manager = SolveEventManager(i);
        if (!manager || !manager->IsRunning() || manager->GetCutscene() != this || !mainRoleActive)
            continue;
        zCEvMsgCutscene* message = new zCEvMsgCutscene(zCEvMsgCutscene::EV_CS_RESUME);
        zCClassDef::ObjectCreated(message, zCEvMsgCutscene::classDef);
        message->SetCutsceneName(properties ? properties->GetName() : zSTRING("??"));
        message->isOutputUnit = !IsCutscene();
        message->isGlobalCutscene = properties && properties->globalCutscene;
        message->isMainRole = roleVobList.parray[i] == mainRoleVob;
        roleVobList.parray[i]->GetEM(0)->OnMessage(message, NULL);
    }
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        if (i < roleVobList.numInArray && roleVobList.parray[i])
            roleEvMList.parray[i] = roleVobList.parray[i]->GetEM(1);
        if (roleEvMList.parray[i])
            roleEvMList.parray[i]->SetCutscene(this);
    }
    if (actualBlockNum < blocks.numInArray)
        PrepareBlock();
    if (!actualBlock)
        return 0;
    if (roleEvMList.numInArray == 1)
        actualBlock->Play(roleEvMList.parray[0]);
    else
        actualBlock->Play(roleEvMList);
    return 1;
}

void zCCutscene::ResumeAtBlock(int index)
{
    actualBlockNum = index;
}

int zCCutscene::PlaySub(float, float)
{
    if (IsFinished())
        return 0;
    if (roleEvMList.numInArray == 1)
        actualBlock->Play(roleEvMList.parray[0]);
    else
        actualBlock->Play(roleEvMList);
    return CheckPlayState();
}

int zCCutscene::IsFinished()
{
    return actualBlockNum >= blocks.numInArray;
}

int zCCutscene::NewBlock(int first, int second)
{
    if (first != -1)
        return zCCSBlock::NewBlock(first, second);
    zCCSSyncBlock* block = new zCCSSyncBlock;
    zCClassDef::ObjectCreated(block, zCCSSyncBlock::classDef);
    if (second >= blocks.numInArray) {
        zCCSBlockPosition position;
        position.block = block;
        blocks.InsertEnd(position);
        return blocks.numInArray - 1;
    }
    blocks.InsertEnd(zCCSBlockPosition());
    for (int i = blocks.numInArray - 1; i > second; --i)
        blocks.parray[i] = blocks.parray[i - 1];
    blocks.parray[second] = zCCSBlockPosition();
    blocks.parray[second].block = block;
    return second;
}

void zCCutscene::NewCS(zCCSManager*)
{
    zCCSRole* role = new zCCSRole;
    zCClassDef::ObjectCreated(role, zCCSRole::classDef);
    role->roleName = "_MAINROLE";
    roleList.InsertEnd(role);
}

void zCCutscene::StopRole(zCVob* vob)
{
    zCCSCamera* camera = vob ? dynamic_cast<zCCSCamera*>(vob) : NULL;
    if (!camera)
        return;
    zCCSCamera_EventMsg* message = new zCCSCamera_EventMsg(zCCSCamera_EventMsg::EV_STOP);
    zCClassDef::ObjectCreated(message, zCCSCamera_EventMsg::classDef);
    camera->GetEM(0)->OnMessage(message, NULL);
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Stopping CSCamera", 8, 0, 0x548, 0, 0);
}

zCEventManager* zCCutscene::SolveEventManager(int index)
{
    if (index < 0 || index >= roleVobList.numInArray || !roleVobList.parray[index])
        return NULL;
    roleEvMList.parray[index] = roleVobList.parray[index]->GetEM(1);
    return roleEvMList.parray[index];
}

void zCCutscene::Stop()
{
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Stopping cutscene ...", 8, 0, 0x55A, 0, 0);
    if (!mainRoleVob) {
        if (zerr->filter_level >= 8)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Stopping cutscene failed ... mainrole is NULL",
                8, 0, 0x55D, 0, 0);
        return;
    }
    const int mainRoleActive = mainRoleVob->GetEM(0)->GetCutscene() == this;
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        zCEventManager* manager = SolveEventManager(i);
        if (!manager || !manager->IsRunning() || manager->GetCutscene() != this
            || !roleVobList.parray[i] || !mainRoleActive)
            continue;
        zCEvMsgCutscene* message = new zCEvMsgCutscene(zCEvMsgCutscene::EV_CS_STOP);
        zCClassDef::ObjectCreated(message, zCEvMsgCutscene::classDef);
        message->SetCutsceneName(properties ? properties->GetName() : zSTRING("??"));
        message->isOutputUnit = !IsCutscene();
        message->isGlobalCutscene = properties && properties->globalCutscene;
        message->isMainRole = roleVobList.parray[i] == mainRoleVob;
        roleVobList.parray[i]->GetEM(0)->OnMessage(message, NULL);
    }
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        zCEventManager* manager = SolveEventManager(i);
        if (manager && manager->IsRunning()) {
            if (manager->GetCutscene() != this) {
                if (zerr->filter_level >= 7)
                    zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + (properties ? properties->GetName() : zSTRING("??"))
                        + "\": Release of role \"" + manager->hostVob->GetObjectName()
                        + "\" failed: Wrong cutscene.", 7, 0, 0x596, 0, 0);
            } else {
                zCVob* roleVob = roleVobList.parray[i];
                if (roleVob)
                    roleVob->AddRef();
                manager->Clear();
                manager->SetCutscene(NULL);
                StopRole(roleVob);
                if (roleVob)
                    roleVob->Release();
            }
        }
        roleEvMList.parray[i] = NULL;
        if (roleVobList.parray[i]) {
            roleVobList.parray[i]->Release();
            roleVobList.parray[i] = NULL;
        }
    }
    actualBlockNum = blocks.numInArray;
    if (mainRoleVob) {
        mainRoleVob->Release();
        mainRoleVob = NULL;
    }
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene stopped.", 8, 0, 0x5AA, 0, 0);
}

void zCCutscene::Interrupt()
{
    const int mainRoleActive = mainRoleVob && mainRoleVob->GetEM(0)->GetCutscene() == this;
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        if (i >= roleVobList.numInArray || !roleVobList.parray[i])
            continue;
        zCEventManager* manager = SolveEventManager(i);
        if (!manager || !manager->IsRunning() || manager->GetCutscene() != this || !mainRoleActive)
            continue;
        zCEvMsgCutscene* message = new zCEvMsgCutscene(zCEvMsgCutscene::EV_CS_INTERRUPT);
        zCClassDef::ObjectCreated(message, zCEvMsgCutscene::classDef);
        message->SetCutsceneName(properties ? properties->GetName() : zSTRING("??"));
        message->isOutputUnit = !IsCutscene();
        message->isGlobalCutscene = properties && properties->globalCutscene;
        message->isMainRole = roleVobList.parray[i] == mainRoleVob;
        roleVobList.parray[i]->GetEM(0)->OnMessage(message, NULL);
    }
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        zCEventManager* manager = SolveEventManager(i);
        if (!manager || !manager->IsRunning())
            continue;
        if (manager->GetCutscene() != this) {
            if (zerr->filter_level >= 7)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + (properties ? properties->GetName() : zSTRING("??"))
                    + "\": Release of role \"" + manager->hostVob->GetObjectName()
                    + "\" failed: Wrong cutscene.  (interrupt)", 7, 0, 0x5DC, 0, 0);
            continue;
        }
        zCVob* roleVob = roleVobList.parray[i];
        if (roleVob)
            roleVob->AddRef();
        manager->Clear();
        manager->SetCutscene(NULL);
        StopRole(roleVob);
        if (roleVob)
            roleVob->Release();
    }
}

void zCCutscene::NewRole(zSTRING& name)
{
    zCCSRole* role = new zCCSRole(name);
    zCClassDef::ObjectCreated(role, zCCSRole::classDef);
    roleList.InsertEnd(role);
}

void zCCutscene::DeleteRole(int index)
{
    if (index < 0 || index >= roleList.numInArray)
        return;
    for (int i = index; i < roleList.numInArray - 1; ++i)
        roleList.parray[i] = roleList.parray[i + 1];
    --roleList.numInArray;
    for (int i = 0; i < GetNumOfChilds(); ++i) {
        zCCSSyncBlock* block = dynamic_cast<zCCSSyncBlock*>(GetChild(i));
        if (!block)
            continue;
        for (int j = 0; j < block->roleAss.numInArray;) {
            if (block->roleAss.parray[j] == index)
                block->DeleteChild(j);
            else
                ++j;
        }
        block->AssignerCorrection(index);
    }
}

void zCCutscene::RegisterAdditionalVob(zCTree<zCVob>* node)
{
    roleVobList.InsertEnd(node->data);
    if (node->data) {
        node->data->AddRef();
        node->data->showVisual &= 0xfe;
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        RegisterAdditionalVob(child);
}

zCVob* zCCutscene::FirstRoleUsed()
{
    if (roleEvMList.numInArray > 0 && roleEvMList.parray[0]->GetCutsceneMode())
        return roleVobList.parray[0];
    return NULL;
}

int zCCSProps::GetCheckRoleResult()
{
    return 2;
}

int zCCutscene::CheckRoles(int& reaction)
{
    reaction = 0;
    if (!properties)
        return 1;
    int result = 1;
    for (int i = 0; i < roleVobList.numInArray; ++i) {
        if (roleVobList.parray[i] && !properties->CheckRole(roleList.parray[i], roleVobList.parray[i])) {
            result = 0;
            reaction = properties->GetCheckRoleResult();
            break;
        }
    }
    if (!result)
        lastCheckResult = 6;
    return result;
}

int zCCutscene::GetLastCheckResult(zSTRING& text)
{
    static const char* names[] = {
        "none", "O.K.", "RoleErr", "RoleMiss", "PlayerMiss", "PlayerErr", "RoleCheck",
        "NoStage", "NoWorld"
    };
    if (lastCheckResult >= 0 && lastCheckResult < 9)
        text = names[lastCheckResult];
    else
        text = "unknown";
    return lastCheckResult;
}

int zCCutscene::TryToCancel()
{
    if (!properties || !properties->globalCutscene || actualBlockNum < properties->numLockedBlocks)
        return 0;
    Stop();
    return 1;
}

void zCCutscene::SerializeBlock(zCCSBlockBase* block, zTCSBlockPosition& position)
{
    position.flatData.InsertEnd(block);
    for (int i = 0; i < block->GetNumOfChilds(); ++i)
        SerializeBlock(block->GetChild(i), position);
}

zCEventMessage* zCCutscene::GetNextMessage(zTCSBlockPosition& position)
{
    while (position.index < position.numInList) {
        zCCSBlockBase* block = position.flatData.parray[position.index++];
        if (block && block->GetCommand())
            return block->GetCommand();
    }
    return NULL;
}

zCCSBlockBase* zCCutscene::GetNextBlock(zTCSBlockPosition& position)
{
    while (position.index < position.numInList) {
        zCCSBlockBase* block = position.flatData.parray[position.index++];
        if (block)
            return block;
    }
    return NULL;
}

void zCCutscene::SerializeCutscene(zCCutscene* cutscene, zTCSBlockPosition& position)
{
    position.flatData.InsertEnd(cutscene);
    for (int i = 0; i < cutscene->GetNumOfChilds(); ++i)
        SerializeBlock(cutscene->GetChild(i), position);
    position.index = 0;
    position.numInList = position.flatData.numInArray;
}

zTCSBlockPosition zCCutscene::GetFirstBlockPosition()
{
    zTCSBlockPosition result;
    SerializeBlock(this, result);
    result.index = 0;
    result.numInList = result.flatData.numInArray;
    return result;
}

zCVob* zCCutscene::LoadStage()
{
    if (!properties || properties->stageName.IsEmpty() || !GetStage())
        return NULL;
    zCWorld* world = GetStage()->homeWorld;
    zCVob* stage = world->MergeVobSubtree(zSTRING("VOBTREES\\") + properties->stageName + ".zen",
        NULL, 0);
    if (stage && stage->globalVobTreeNode)
        RegisterAdditionalVob(stage->globalVobTreeNode);
    return stage;
}

zCVob* zCCutscene::CatchPlayer(zCVob* vob)
{
    if (!vob)
        return NULL;
    zCNetVobControl* control = vob->GetEM(0)->GetNetVobControl(1);
    if (control)
        return control->ctrlPlayer->hostVob;
    zCPlayerInfo* player = zCPlayerInfo::GetActivePlayer();
    return player ? player->hostVob : NULL;
}

int zCCutscene::StartPlaying()
{
    if (!roleEvMList.numInArray)
        return 0;
    int canStart = 1;
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        if (i < roleVobList.numInArray && roleVobList.parray[i])
            roleEvMList.parray[i] = roleVobList.parray[i]->GetEM(1);
        zCEventManager* manager = roleEvMList.parray[i];
        if (!manager || !manager->GetCutscene())
            continue;
        zCCutscene* playing = manager->GetCutscene();
        if (properties && properties->globalCutscene
            && (!playing->properties || !playing->properties->globalCutscene)) {
            if (zerr->filter_level >= 8)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + properties->GetName()
                    + "\": Stopping ambient cutscene of role: " + roleList.parray[i]->roleName,
                    8, 0, 0x4EF, 0, 0);
            playing->Stop();
        } else {
            if (zerr->filter_level >= 8)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + (properties ? properties->GetName() : zSTRING("??"))
                    + "\": Could not start playing: role \"" + roleList.parray[i]->roleName
                    + "\" is playing another cutscene!", 8, 0, 0x4FA, 0, 0);
            canStart = 0;
        }
    }
    if (!canStart)
        return 0;
    Reset();
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        if (!roleVobList.parray[i])
            continue;
        zCEvMsgCutscene* message = new zCEvMsgCutscene(zCEvMsgCutscene::EV_CS_STARTPLAY);
        zCClassDef::ObjectCreated(message, zCEvMsgCutscene::classDef);
        message->SetCutsceneName(properties ? properties->GetName() : zSTRING("??"));
        message->isOutputUnit = !IsCutscene();
        message->isGlobalCutscene = properties && properties->globalCutscene;
        message->isMainRole = roleVobList.parray[i] == mainRoleVob;
        roleVobList.parray[i]->GetEM(0)->OnMessage(message, NULL);
    }
    if (roleEvMList.numInArray == 1)
        actualBlock->Play(roleEvMList.parray[0]);
    else
        actualBlock->Play(roleEvMList);
    return 1;
}

void zCCutscene::Reset()
{
    actualBlockNum = 0;
    PrepareBlock();
}

void zCCutscene::ClearCutscene()
{
    for (int i = 0; i < roleList.numInArray; ++i) {
        if (roleList.parray[i]) {
            roleList.parray[i]->Release();
            roleList.parray[i] = NULL;
        }
    }
    for (int i = 0; i < roleVobList.numInArray; ++i) {
        if (roleVobList.parray[i]) {
            roleVobList.parray[i]->Release();
            roleVobList.parray[i] = NULL;
        }
        roleEvMList.parray[i] = NULL;
    }
    roleList.numInArray = 0;
    roleVobList.numInArray = 0;
    roleEvMList.numInArray = 0;
    if (mainRoleVob) {
        mainRoleVob->Release();
        mainRoleVob = NULL;
    }
    if (properties) {
        properties->Release();
        properties = NULL;
    }
}

void zCCutscene::PrepareBlock()
{
    actualBlock = blocks.parray[actualBlockNum].block;
    for (int i = 0; i < roleEvMList.numInArray; ++i) {
        if (i < roleVobList.numInArray && roleVobList.parray[i])
            roleEvMList.parray[i] = roleVobList.parray[i]->GetEM(1);
        if (roleEvMList.parray[i] && !roleEvMList.parray[i]->GetCutscene())
            roleEvMList.parray[i]->SetCutscene(this);
    }
    actualBlock->OrganizeRoles(roleVobList, roleList);
}

void zCCutscene::AddRoleOU(zCVob* vob)
{
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + (properties ? properties->GetName() : zSTRING("??"))
            + "\": Adding vob \"" + vob->GetObjectName() + "\" (refCtr before is "
            + zSTRING(vob->refCtr) + ") to outputunit", 7, 0, 0x3B7, 0, 0);
    if (roleVobList.numInArray <= 0)
        roleVobList.InsertEnd(NULL);
    if (roleEvMList.numInArray <= 0)
        roleEvMList.InsertEnd(NULL);
    roleVobList.parray[0] = vob;
    if (vob) {
        vob->AddRef();
        roleEvMList.parray[0] = vob->GetEM(0);
    } else {
        roleEvMList.parray[0] = NULL;
    }
    if (vob)
        vob->AddRef();
    if (mainRoleVob) {
        mainRoleVob->Release();
        mainRoleVob = NULL;
    }
    mainRoleVob = vob;
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + (properties ? properties->GetName() : zSTRING("??"))
            + "\": Added vob \"" + vob->GetObjectName() + "\" (refCtr after is "
            + zSTRING(vob->refCtr) + ") to outputunit", 7, 0, 0x3C6, 0, 0);
}

int zCCutscene::AddMainRole(zCVob* vob)
{
    const int roleIndex = CheckMainRole(vob);
    if (roleIndex < 0)
        return 0;
    const zSTRING cutsceneName = properties ? properties->GetName() : zSTRING("??");
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + cutsceneName + "\": Added vob \""
            + vob->GetObjectName() + "\" (refCtr before is " + zSTRING(vob->refCtr)
            + ") to cutscene", 7, 0, 0x3D0, 0, 0);
    if (vob)
        vob->AddRef();
    if (roleVobList.parray[roleIndex]) {
        roleVobList.parray[roleIndex]->Release();
        roleVobList.parray[roleIndex] = NULL;
    }
    roleVobList.parray[roleIndex] = vob;
    roleEvMList.parray[roleIndex] = vob ? vob->GetEM(0) : NULL;
    if (vob)
        vob->AddRef();
    if (mainRoleVob) {
        mainRoleVob->Release();
        mainRoleVob = NULL;
    }
    mainRoleVob = vob;
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + cutsceneName + "\": Added vob \""
            + vob->GetObjectName() + "\" (refCtr after is " + zSTRING(vob->refCtr)
            + ") to cutscene", 7, 0, 0x3DE, 0, 0);
    return 1;
}

void zCCutscene::RemoveRole(zCVob* vob)
{
    for (int i = 0; i < roleVobList.numInArray; ++i) {
        if (roleVobList.parray[i] != vob)
            continue;
        if (vob == mainRoleVob) {
            Stop();
            return;
        }
        roleEvMList.parray[i] = NULL;
        if (roleVobList.parray[i]) {
            roleVobList.parray[i]->Release();
            roleVobList.parray[i] = NULL;
        }
        return;
    }
}

int zCCutscene::CSCheck(int& reaction)
{
    lastCheckResult = CS_CHECK_OK;
    int checkPassed = 1;
    zCVob* stage = GetStage();
    if (!stage)
        lastCheckResult = CS_CHECK_NOSTAGE;
    else if (!stage->homeWorld)
        lastCheckResult = CS_CHECK_NOWORLD;
    else {
        for (int i = 0; i < roleList.numInArray; ++i) {
            if (i < roleVobList.numInArray && roleVobList.parray[i])
                roleEvMList.parray[i] = roleVobList.parray[i]->GetEM(1);
            zCCSRole* role = roleList.parray[i];
            if (roleEvMList.parray[i]) {
                if (properties && !properties->CheckRole(role, roleVobList.parray[i])) {
                    if (roleVobList.parray[i] != mainRoleVob) {
                        roleEvMList.parray[i] = NULL;
                        if (roleVobList.parray[i]) {
                            roleVobList.parray[i]->Release();
                            roleVobList.parray[i] = NULL;
                        }
                    } else {
                        Stop();
                    }
                    checkPassed = 0;
                }
                continue;
            }

            zCVob* roleVob = role->roleVob;
            int roleIndex = -1;
            for (int j = 0; j < roleVobList.numInArray; ++j) {
                if (roleVobList.parray[j] == roleVob) {
                    roleIndex = j;
                    break;
                }
            }
            if (roleVob && roleIndex < 0) {
                stage->homeWorld->AddVob(roleVob);
                roleVob->SetPhysicsEnabled(0);
                roleVobList.parray[i] = roleVob;
                roleEvMList.parray[i] = roleVob->GetEM(0);
            } else if (role->roleName == CS_PLAYER_ROLENAME) {
                zCVob* player = CatchPlayer(NULL);
                if (player && (!properties || properties->AcceptRole(role, player))) {
                    player->AddRef();
                    if (roleVobList.parray[i]) {
                        roleVobList.parray[i]->Release();
                        roleVobList.parray[i] = NULL;
                    }
                    roleVobList.parray[i] = player;
                    roleEvMList.parray[i] = player->GetEM(0);
                } else {
                    roleIndex = -1;
                    for (int j = 0; j < roleVobList.numInArray; ++j) {
                        if (roleVobList.parray[j] == player) {
                            roleIndex = j;
                            break;
                        }
                    }
                    lastCheckResult = roleIndex >= 0 ? CS_CHECK_PLAYER_ERROR : CS_CHECK_PLAYER_MISSING;
                    if (!player)
                        Stop();
                    checkPassed = 0;
                }
            }
            if (!roleEvMList.parray[i] && !role->rFlag) {
                lastCheckResult = CS_CHECK_ROLE_MISSING;
                checkPassed = 0;
            }
        }
        if (checkPassed && CheckRoles(reaction))
            return 1;
    }
    if (zerr->filter_level >= 7) {
        zSTRING reason;
        GetLastCheckResult(reason);
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + (properties ? properties->GetName() : zSTRING("??"))
            + "\": CSCheck ... Start disabled, because of " + reason,
            zerr->filter_level, 0, 0x4CA, 0, 0);
    }
    return 0;
}

int zCCutscene::CheckMainRole(zCVob* vob)
{
    if (!vob)
        return -1;
    const zSTRING objectName = vob->GetObjectName();
    const zSTRING cutsceneName = properties ? properties->GetName() : zSTRING("??");
    zCPlayerInfo* activePlayer = zCPlayerInfo::GetActivePlayer();
    const int isPlayer = activePlayer && vob == activePlayer->hostVob;
    int namedRole = -1;
    int mainRole = -1;
    int playerRole = -1;
    for (int i = 0; i < roleList.numInArray; ++i) {
        const zSTRING& roleName = roleList.parray[i]->roleName;
        if (roleName == CS_MAIN_ROLENAME)
            mainRole = i;
        if (roleName == CS_PLAYER_ROLENAME)
            playerRole = i;
        if (roleName == objectName)
            namedRole = i;
    }
    if (namedRole < 0) {
        if (!isPlayer) {
            if (mainRole >= 0)
                return mainRole;
            zerr->Report(zERR_TYPE_WARN, 0, "B: CS \"" + cutsceneName + "\": Mainrole \"" + objectName
                + "\" was not found in cutscenes", 0, 0, 0x370, 0, 0);
            return -1;
        }
        if (playerRole >= 0) {
            if (mainRole < 0)
                return playerRole;
            zerr->Report(zERR_TYPE_WARN, 0, "B: CS \"" + cutsceneName + "\": Mainrole \"" + objectName
                + "\" is _PLAYER but his rolename was found in the cutscene, too", 0, 0, 0x36A, 0, 0);
            return -1;
        }
        zerr->Report(zERR_TYPE_WARN, 0, "B: CS \"" + cutsceneName + "\": Mainrole \"" + objectName
            + "\" is the player but no " + CS_MAIN_ROLENAME + " or " + CS_PLAYER_ROLENAME
            + " is defined as a role in the cutscene", 0, 0, 0x369, 0, 0);
        return -1;
    }
    if (isPlayer && playerRole >= 0) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: CS \"" + cutsceneName + "\": Mainrole \"" + objectName
            + "\" is the player but " + CS_MAIN_ROLENAME + " and(!) " + CS_PLAYER_ROLENAME
            + " are defined as a role in the cutscene", 0, 0, 0x362, 0, 0);
        return -1;
    }
    if (mainRole >= 0) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: CS \"" + cutsceneName + "\": Mainrole \"" + objectName
            + "\" found as a role of the cutscene but " + CS_MAIN_ROLENAME
            + " is defined as a role, too", 0, 0, 0x363, 0, 0);
        return -1;
    }
    return namedRole;
}

int zCCutscene::CheckPlayState()
{
    if (roleVobList.numInArray > 0 && roleVobList.parray[0])
        roleEvMList.parray[0] = roleVobList.parray[0]->GetEM(1);
    int result;
    if (roleEvMList.numInArray == 1)
        result = actualBlock->IsFinished(roleEvMList.parray[0]);
    else
        result = actualBlock->IsFinished(roleEvMList);
    if (result != 1)
        return result;
    ++actualBlockNum;
    if (actualBlockNum >= blocks.numInArray) {
        if (!properties || !properties->csLoop) {
            Stop();
            return 2;
        }
        actualBlock = NULL;
        actualBlockNum = 0;
    }
    if (actualBlockNum < blocks.numInArray) {
        PrepareBlock();
        if (roleEvMList.numInArray == 1)
            actualBlock->Play(roleEvMList.parray[0]);
        else
            actualBlock->Play(roleEvMList);
    }
    return result;
}

zCVob* zCCutscene::GetStage()
{
    return NULL;
}

void zCCutscene::SetRoleListForDebugging(zCArray<zCCSRole*>& roles)
{
    for (int i = 0; i < roles.numInArray; ++i) {
        roleVobList.InsertEnd(NULL);
        roleEvMList.InsertEnd(NULL);
        zCCSRole* source = roles.parray[i];
        zCCSRole* role = new zCCSRole;
        zCClassDef::ObjectCreated(role, zCCSRole::classDef);
        role->roleName = source->roleName;
        role->mustBeAlive = source->mustBeAlive;
        role->rFlag = source->rFlag;
        roleList.InsertEnd(role);
        if (source->roleVob) {
            zCVob* vob = static_cast<zCVob*>(source->roleVob->CreateCopy());
            role->SetRoleVob(vob);
        }
    }
}

void zCCutscene::SaveRolesVobs(zCBuffer& buffer)
{
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(&buffer, 0, 0, 1);
    for (int i = 0; i < roleList.numInArray; ++i) {
        if (!roleList.parray[i]->roleVob) {
            const zSTRING entryName = "roleName" + zSTRING(i);
            archiver->WriteString(entryName, roleList.parray[i]->roleName);
            archiver->WriteObject(roleVobList.parray[i]);
        }
    }
    zCBuffer* archiveBuffer = archiver->GetBuffer();
    if (archiveBuffer)
        buffer = *archiveBuffer;
    archiver->Close();
    archiver->Release();
}

void zCCutscene::RestoreRoleVobs(zCBuffer& buffer)
{
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(&buffer, 0);
    for (int i = 0; i < roleList.numInArray; ++i) {
        if (!roleList.parray[i]->roleVob) {
            const zSTRING entryName = "roleName" + zSTRING(i);
            archiver->ReadString(entryName);
            if (roleVobList.parray[i]) {
                roleVobList.parray[i]->Release();
                roleVobList.parray[i] = NULL;
            }
            roleVobList.parray[i] = dynamic_cast<zCVob*>(archiver->ReadObject(NULL));
        }
    }
    archiver->Close();
    archiver->Release();
}

void zCCSProps::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCObject* zCCutscene::_CreateNewInstance()
{
    zCCutscene* cutscene = new zCCutscene;
    zCClassDef::ObjectCreated(cutscene, classDef);
    return cutscene;
}

void zCCutscene::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCCSRole::zCCSRole()
    : mustBeAlive(0), rFlag(0), roleVob(NULL)
{
}

zCCSRole::zCCSRole(zSTRING& name)
    : roleName(name), mustBeAlive(0), rFlag(0), roleVob(NULL)
{
}

zCCSRole::~zCCSRole()
{
    if (roleVob) {
        roleVob->Release();
        roleVob = NULL;
    }
}

zCObject* zCCSRole::_CreateNewInstance()
{
    zCCSRole* role = new zCCSRole;
    zCClassDef::ObjectCreated(role, classDef);
    return role;
}

void zCCSRole::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSRole::_GetClassDef() const
{
    return classDef;
}

void zCCSRole::SetRoleVob(zCVob* vob)
{
    if (vob) {
        roleName = vob->GetObjectName();
        vob->AddRef();
    }
    if (roleVob) {
        roleVob->Release();
        roleVob = NULL;
    }
    roleVob = vob;
}

zCVob* zCCSRole::GetRoleVob()
{
    return roleVob;
}

void zCCSRole::Archive(zCArchiver& archiver)
{
    archiver.WriteBool(mustBeAlive);
    archiver.WriteString(roleName);
    archiver.WriteEnum(rFlag);
    if (!archiver.InProperties())
        archiver.WriteObject(roleVob);
}

void zCCSRole::Unarchive(zCArchiver& archiver)
{
    mustBeAlive = archiver.ReadBool();
    roleName = archiver.ReadString();
    roleName.Upper();
    rFlag = archiver.ReadEnum();
    if (!archiver.InProperties()) {
        if (roleVob)
            roleVob->Release();
        roleVob = dynamic_cast<zCVob*>(archiver.ReadObject(NULL));
    }
}
