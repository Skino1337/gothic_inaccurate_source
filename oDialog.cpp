#include "include/oDialog.h"

static zSTRING fontDefaultTexture("FONT_DEFAULT.TGA");
static zSTRING textFontInventory("TEXT_FONT_INVENTORY");
static zSTRING inventoryBackTexture("INV_BACK.TGA");
static zSTRING inventoryBackContainerTexture("INV_BACK_CONTAINER.TGA");
static zSTRING inventoryBackPlunderTexture("INV_BACK_PLUNDER.TGA");
static zSTRING inventoryBackStealTexture("INV_BACK_STEAL.TGA");
static zSTRING inventoryBackBuyTexture("INV_BACK_BUY.TGA");
static zSTRING inventoryBackSellTexture("INV_BACK_SELL.TGA");
static zSTRING inventoryTitleTexture("INV_TITLE.TGA");
static zSTRING inventorySlotTexture("INV_SLOT.TGA");
static zSTRING inventorySlotEquippedTexture("INV_SLOT_EQUIPPED.TGA");
static zSTRING inventorySlotEquippedHighlightedTexture("INV_SLOT_EQUIPPED_HIGHLIGHTED.TGA");
static zSTRING inventorySlotHighlightedTexture("INV_SLOT_HIGHLIGHTED.TGA");
static zSTRING inventoryDescTexture("INV_DESC.tga");
static zSTRING inventoryOTexture("O.TGA");
static zSTRING inventoryUTexture("U.TGA");
static int npcTalkState = -1;
static zSTRING npcTalkPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcTalkPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcTalkPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcTalkPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcTalkPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcTalkPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcTalkPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcTalkPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcTalkPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcTalkPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float npcTalkFarDistance = 250000.0f * 0.0166666675f;
static float npcTalkNearDistance = npcTalkFarDistance * 0.0166666675f;
static float npcTalkFarPlaneDistance = 250000.0f * 24.0f;
const zSTRING NPC_TALK_ANI("VISEME");
static zCOLOR npcTalkColor(255, 255, 255, 255);
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/oNpcMessages.h"
#include "include/zSound.h"
#include "include/oHelp.h"
#include "include/oMission.h"
#include "include/oItemReact.h"
#include <cstdlib>

extern oCMissionManager misMan;

// EXE: 0x00AB1124
zCGfx* oCNpcInventory::gfx_title = NULL;

oCAskBox::oCAskBox()
    : fadeout(0), answer(0), alpha(0), askvob(0), npc1(0), npc2(0)
{
    thumb[0] = 0;
    thumb[1] = 0;
    thumb[2] = 0;
}

oCAskBox::~oCAskBox()
{
    npc1 = 0;
    npc2 = 0;
    if (askvob) {
        askvob->Release();
        askvob = 0;
    }
    thumb[0] = 0;
    thumb[1] = 0;
    thumb[2] = 0;
}

void oCAskBox::Init(oCNpc* firstNpc, oCNpc* secondNpc)
{
    npc1 = secondNpc;
    npc2 = firstNpc;
    askvob = new zCVob;
    zCClassDef::ObjectCreated(askvob, zCVob::classDef);
    askvob->SetCollDetStat(0);
    askvob->SetCollDetDyn(0);
    askvob->SetPhysicsEnabled(0);
    askvob->SetSleeping(0);
    askvob->SetAI(this);
    fadeout = 0;
    alpha = 0;
    SetNeutral();
    if (!askvob->GetHomeWorld())
        npc1->GetHomeWorld()->AddVob(askvob);
    SetPosition();
}

void oCAskBox::Delete()
{
    fadeout = 1;
}

void oCAskBox::DoAI(zCVob* vob, int& deleteAI)
{
    if (askvob)
        SetPosition();
    if (fadeout) {
        alpha -= 20;
        if (alpha < 0)
            alpha = 0;
        ((zCDecal*)askvob->visual)->decalMaterial->color[3] = alpha;
        if (!alpha) {
            askvob->SetAI(0);
            askvob = 0;
            deleteAI = 1;
            Release();
        }
    } else if (alpha < 255) {
        alpha += 20;
        if (alpha > 255)
            alpha = 255;
        ((zCDecal*)askvob->visual)->decalMaterial->color[3] = alpha;
    }
}

static void SetAskTexture(oCAskBox* box, const zSTRING& texture, float dimension)
{
    box->askvob->SetVisual(texture);
    zCDecal* visual = (zCDecal*)box->askvob->visual;
    visual->decalMaterial->rndAlphaBlendFunc = 3;
    visual->decalMaterial->matGroup = 5;
    visual->decalMaterial->color[3] = box->alpha;
    visual->SetDecalDim(dimension, dimension);
}

void oCAskBox::SetNeutral()
{
    if (answer != 2) {
        SetAskTexture(this, "Ask_Neutral.tga", 3.0f);
        answer = 2;
    }
}

void oCAskBox::SetYes()
{
    if (answer != 1) {
        SetAskTexture(this, "Ask_Yes.tga", 3.0f);
        answer = 1;
    }
}

void oCAskBox::SetNo()
{
    if (answer) {
        SetAskTexture(this, "Ask_No.tga", 3.0f);
        answer = 0;
    }
}

void oCAskBox::SetPosition()
{
    if (!askvob || !zCCamera::activeCam || !zCCamera::activeCam->connectedVob)
        return;
    zCCamera* camera = ogame->GetCamera();
    zCVob* cameraVob = ogame->GetCameraVob();
    zVEC3 local = {
        (zrenderer->vid_xdim / 2 - camera->vpData.xcenter) * camera->viewDistanceXInv * 10.0f,
        (camera->vpData.ycenter - (zrenderer->vid_ydim - zrenderer->vid_ydim / 5)) *
            camera->viewDistanceYInv * 10.0f,
        10.0f
    };
    zVEC3 position;
    for (int i = 0; i < 3; ++i) {
        position.n[i] = cameraVob->trafoObjToWorld.m[i][3] +
            cameraVob->trafoObjToWorld.m[i][0] * local.n[0] +
            cameraVob->trafoObjToWorld.m[i][1] * local.n[1] +
            cameraVob->trafoObjToWorld.m[i][2] * local.n[2];
    }
    askvob->SetPositionWorld(position);
}

int oCNpc::IsWaitingForAnswer()
{
    oCMsgConversation* message = dynamic_cast<oCMsgConversation*>(GetEM(0)->GetActiveMessage());
    return message && (message->subType == 8 || message->subType == 9);
}

int oCNpc::SetCurrentAnswer(int answer, int waitForQuestion, oCNpc* other)
{
    zCView* view = ogame->array_view[4];
    oCMsgConversation* message = dynamic_cast<oCMsgConversation*>(GetEM(0)->GetActiveMessage());
    if (!message)
        return 0;
    if (message->subType == 8) {
        if (!waitForQuestion) {
            if (zinput->GetToggled(5))
                view->Open();
            else if (zinput->GetState(5) == 0.0f) {
                view->Close();
                return 0;
            }
        }
        zCOLOR yesColor(0, 255, 0, 255);
        zCOLOR noColor(255, 0, 0, 255);
        view->DialogSelection(message->text, message->name, answer >= 0 ? noColor : yesColor,
            answer > 0 ? yesColor : noColor, -2.0f);
        if (message->MD_GetMinTime() && (answer == 1 || answer == -1)) {
            int wasMoving = (other->isInMovementMode & 3) != 0;
            if (wasMoving)
                other->EndMovement(1);
            message->AddRef();
            parser.SetInstance("SELF", this);
            parser.SetInstance("OTHER", other);
            state.CloseCutscenes();
            int function = answer > 0 ? message->f_yes : message->f_no;
            if (function)
                parser.CallFunc(function);
            state.SetStateTime(0);
            view->Close();
            message->Delete();
            message->Release();
            if (wasMoving)
                other->BeginMovement();
        }
        return 1;
    }
    if (message->subType == 9 && answer > 0) {
        int wasMoving = (other->isInMovementMode & 3) != 0;
        if (wasMoving)
            other->EndMovement(1);
        message->AddRef();
        parser.SetInstance("SELF", this);
        parser.SetInstance("OTHER", other);
        state.CloseCutscenes();
        parser.CallFunc(message->f_yes);
        state.SetStateTime(0);
        message->Delete();
        message->Release();
        if (wasMoving)
            other->BeginMovement();
        return 1;
    }
    return 0;
}

int oCNpc::EV_Ask(oCMsgConversation* message)
{
    oCNpc* playerNpc = ogame->GetSelfPlayerVob();
    if (playerNpc->GetFocusNpc() == this && IsInPerceptionRange(19, this, playerNpc)) {
        message->SetInUse(1);
        return 0;
    }
    if (message->IsInUse() && zinput->GetState(5) == 0.0f) {
        ogame->array_view[4]->Close();
        return 1;
    }
    return 0;
}

int oCNpc::EV_WaitForQuestion(oCMsgConversation* message)
{
    message->timer -= ztimer.frameTimeFloat;
    if (message->timer > 0.0f)
        return 0;
    parser.SetInstance("SELF", this);
    parser.CallFunc("B_NPCBye");
    return 1;
}

int oCNpc::CanTalk()
{
    return canTalk != 0.0f;
}

void oCNpc::SetCanTalk(int enabled)
{
    canTalk = enabled * 1000.0f;
}

void oCNpc::SetTalkingWith(oCNpc* other, int active)
{
    if (!other) {
        talkOther = 0;
        return;
    }
    if (active)
        talkOther = other;
    else if (talkOther == other)
        talkOther = 0;
}

oCNpc* oCNpc::GetTalkingWith()
{
    return talkOther;
}

void oCNpc::StartTalkingWith(oCNpc* other)
{
    if (other) {
        talkOther = other;
        other->talkOther = this;
    }
}

void oCNpc::StopTalkingWith()
{
    if (talkOther)
        talkOther->talkOther = 0;
    talkOther = 0;
}

zCEventMessage* oCNpc::GetTalkingWithMessage(oCNpc* other)
{
    zCEventManager* manager = GetEM(0);
    zCEventMessage* result = 0;
    int found = 0;
    for (int i = manager->GetNumMessages() - 1; i >= 0; --i) {
        zCEventMessage* message = manager->GetEventMessage(i);
        if (!result && !message->IsOverlay())
            result = message;
        oCMsgConversation* conversation = dynamic_cast<oCMsgConversation*>(message);
        if (!conversation || (conversation->subType != 4 && conversation->subType != 5) ||
            conversation->target != other)
            continue;
        found = 1;
        if (result)
            return result;
    }
    return found ? result : 0;
}

int oCNpc::CanBeTalkedTo()
{
    if (IsWaitingForAnswer())
        return 1;
    if (state.IsInState(parser.GetIndex("ZS_TALK"))) {
        StopRunningOU();
        return 1;
    }
    return GetEM(0)->IsEmpty() && !GetEM(0)->GetCutsceneMode();
}

void oCNpc::StopRunningOU()
{
    zCEventManager* manager = GetEM(0);
    for (int i = 0; i < manager->GetNumMessages(); ++i) {
        oCMsgConversation* message = dynamic_cast<oCMsgConversation*>(manager->GetEventMessage(i));
        if (message && message->IsOverlay() && message->subType == 0) {
            zsound->StopSound(message->handle);
            message->Delete();
            if (ogame->array_view[5])
                ogame->array_view[5]->Close();
            if (ogame->array_view[1])
                ogame->array_view[1]->Close();
            if (ogame->array_view[3])
                ogame->array_view[3]->Close();
            manager->SetCutscene(0);
        }
    }
    zCModel* npcModel = GetModel();
    if (npcModel) {
        for (int i = 0; i < npcModel->numActiveAnis; ++i) {
            zCModelAniActive* activeAni = npcModel->aniChannels[i];
            if (activeAni && activeAni->protoAni->aniName.Search("DIALOGGESTURE_") >= 0)
                npcModel->FadeOutAni(activeAni);
        }
        StopFaceAni(NPC_TALK_ANI);
    }
}

int oCNpc::AssessGivenItem(oCItem* item, oCNpc* givenBy)
{
    if (GetPerceptionFunc(20) < 0)
        return 0;
    if (item) {
        item->AddRef();
        if (item->HasFlag(0x1000)) {
            oCMission* mission = misMan.GetAvailableMission(this, givenBy, 0);
            if (mission && mission->SuccessConditions()) {
                ExchangeTradeModules(givenBy);
                parser.SetInstance("ITEM", item);
                mission->Success();
                item->Release();
                return 1;
            }
        }
    }
    AssessGivenItem_S(item, givenBy);
    if (item)
        item->Release();
    return 1;
}

oCNpc* oCNpc::GetTradeNpc()
{
    return tradeNpc;
}

void oCNpc::SetTradeNpc(oCNpc* other)
{
    tradeNpc = other;
}

zCList<oCItemContainer> oCItemContainer::contList;
zCList<oCItemContainer> oCItemContainer::s_openContainers;

oCItemContainer::oCItemContainer()
    : open(0), npc(0), invMode(0), selectedItem(0), offset(0), marginTop(0), marginLeft(0),
      maxSlotsCol(GetInvMaxColumns()), maxSlotsColScr(1), maxSlotsRow(GetInvMaxRows()),
      maxSlotsRowScr(1), frame(0), right(0), ownList(1), prepared(0), passive(0), viewTitle(0),
      viewBack(0), viewItem(0), viewItemActive(0), viewItemHightlighted(0),
      viewItemActiveHighlighted(0), viewItemInfo(0), viewItemInfoItem(0), viewArrowAtTop(0),
      viewArrowAtBottom(0), textView(0), rndWorld(0), TransferCount(1), m_bManipulateItemsDisabled(0),
      m_bCanTransferMoreThanOneItem(1), posx(0), posy(0)
{
    contents.data = 0;
    contents.next = 0;
    maxSlots = maxSlotsCol * maxSlotsRow;
    contList.Insert(this);
}

zCList<oCItem>* oCItemContainer::GetContents()
{
    return &contents;
}

int oCItemContainer::IsOpen() const
{
    for (zCList<oCItemContainer>* node = s_openContainers.next; node; node = node->next) {
        if (node->data == this)
            return 1;
    }
    return 0;
}

void oCItemContainer::Close()
{
    if (IsOpen()) {
        delete viewTitle;
        delete viewBack;
        delete viewItem;
        delete viewItemActive;
        delete viewItemHightlighted;
        delete viewItemActiveHighlighted;
        delete viewItemInfoItem;
        delete viewItemInfo;
        delete viewArrowAtTop;
        delete viewArrowAtBottom;
        delete textView;
        viewTitle = 0;
        viewBack = 0;
        viewItem = 0;
        viewItemActive = 0;
        viewItemHightlighted = 0;
        viewItemActiveHighlighted = 0;
        viewItemInfoItem = 0;
        viewItemInfo = 0;
        viewArrowAtTop = 0;
        viewArrowAtBottom = 0;
        textView = 0;
        if (rndWorld) {
            rndWorld->Release();
            rndWorld = 0;
        }
        zCSoundFX* sound = zsound->LoadSoundFXScript("INV_CLOSE");
        zsound->PlaySound(sound, 0);
        if (sound)
            sound->Release();
    }
    prepared = 0;
    zCList<oCItemContainer>* previous = &s_openContainers;
    while (previous->next) {
        zCList<oCItemContainer>* node = previous->next;
        if (node->data == this) {
            previous->next = node->next;
            delete node;
            break;
        }
        previous = node;
    }
    Deactivate();
    invMode = 0;
    open = 0;
}

void oCItemContainer::Activate()
{
    for (zCList<oCItemContainer>* node = contList.next; node; node = node->next) {
        if (node->data != this)
            node->data->Deactivate();
    }
    SetEnableHandleEvent(1);
    SetHandleEventTop();
    frame = 1;
    if (oCNpc::player)
        oCNpc::player->SetMovLock(1);
    CheckSelectedItem();
}

void oCItemContainer::Deactivate()
{
    SetEnableHandleEvent(0);
    frame = 0;
    if (oCNpc::player && !passive)
        oCNpc::player->SetMovLock(0);
}

oCItem* oCItemContainer::GetSelectedItem()
{
    if (selectedItem < 0)
        return 0;
    int index = 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next, ++index) {
        if (index == selectedItem)
            return node->data;
    }
    return 0;
}

int oCItemContainer::GetSelectedItemCount()
{
    oCItem* item = GetSelectedItem();
    return item ? item->amount : 0;
}

oCItem* oCItemContainer::Insert(oCItem* item)
{
    if (!item)
        return 0;
    if (item->MultiSlot()) {
        for (zCList<oCItem>* node = contents.next; node; node = node->next) {
            oCItem* current = node->data;
            if (current->MultiSlot() && current->GetInstance() == item->GetInstance()) {
                current->amount += item->amount;
                ogame->GetGameWorld()->RemoveVob(item);
                return current;
            }
        }
    }
    contents.Insert(item);
    CheckSelectedItem();
    ++item->refCtr;
    ogame->GetGameWorld()->RemoveVob(item);
    open = 1;
    return item;
}

void oCItemContainer::Remove(oCItem* item)
{
    zCList<oCItem>* previous = &contents;
    while (previous->next) {
        zCList<oCItem>* node = previous->next;
        if (node->data == item) {
            previous->next = node->next;
            delete node;
            break;
        }
        previous = node;
    }
    CheckSelectedItem();
    ogame->GetGameWorld()->AddVob(item);
    item->Release();
}

oCItem* oCItemContainer::RemoveByPtr(oCItem* item, int amount)
{
    return Remove(item, amount);
}

oCItem* oCItemContainer::Remove(oCItem* item, int amount)
{
    if (!item)
        return 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next) {
        if (node->data != item)
            continue;
        if (item->amount <= amount) {
            Remove(item);
            return item;
        }
        item->amount -= amount;
        oCItem* result = (oCItem*)ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM,
            item->GetInstance());
        result->amount = amount;
        ogame->GetGameWorld()->AddVob(result);
        result->Release();
        return result;
    }
    return 0;
}

void oCItemContainer::DeleteContents()
{
    if (ownList)
        contents.DeleteListDatas();
    contents.data = 0;
    contents.next = 0;
}

void oCItemContainer::Remove2(oCItem* item)
{
    if (contents.data == item)
        contents.data = 0;
    for (zCList<oCItem>* entry = contents.next; entry; entry = entry->next) {
        if (entry->data == item)
            entry->data = 0;
    }
}

int oCItemContainer::GetInvSplitScreen()
{
    static short splitScreen = -2;
    if (splitScreen == -2)
        splitScreen = (short)zoptions->ReadBool(zOPT_SEC_GAME, "invSplitScreen", 1);
    return splitScreen;
}

short oCItemContainer::GetInvMaxColumns()
{
    static short maxColumns;
    if (!maxColumns)
        maxColumns = (short)zoptions->ReadInt(zOPT_SEC_GAME, "invMaxColumns", 5);
    if (maxColumns <= 0) {
        maxColumns = -1;
        return 1024;
    }
    return maxColumns;
}

short oCItemContainer::GetInvMaxRows()
{
    static short maxRows;
    if (!maxRows)
        maxRows = (short)zoptions->ReadInt(zOPT_SEC_GAME, "invMaxRows", 0);
    if (maxRows <= 0) {
        maxRows = -1;
        return 1024;
    }
    return maxRows;
}

short oCItemContainer::TransferCountToAmount(short count)
{
    if (count >= 10000)
        return 10000;
    if (count >= 1000)
        return 1000;
    if (count >= 100)
        return 100;
    if (count >= 10)
        return 10;
    return 1;
}

float oCItemContainer::GetValueMultiplier()
{
    static float multiplier;
    if (multiplier == 0.0f) {
        zCPar_Symbol* symbol = parser.GetSymbol("TRADE_VALUE_MULTIPLIER");
        if (symbol)
            symbol->GetValue(multiplier, 0);
        if (multiplier <= 0.0f)
            multiplier = 0.300000012f;
    }
    return multiplier;
}

zSTRING oCItemContainer::GetCurrencyInstanceName()
{
    static zSTRING currencyName;
    if (currencyName.IsEmpty()) {
        zCPar_Symbol* symbol = parser.GetSymbol("TRADE_CURRENCY_INSTANCE");
        if (symbol)
            symbol->GetValue(currencyName, 0);
        if (currencyName.IsEmpty())
            currencyName = "ITMI_GOLD";
    }
    return currencyName;
}

int oCItemContainer::GetCurrencyInstance()
{
    static int currencyInstance = -1;
    if (currencyInstance == -1)
        currencyInstance = parser.GetIndex(GetCurrencyInstanceName());
    return currencyInstance;
}

oCItem* oCItemContainer::CreateCurrencyItem(short amount)
{
    if (!ogame || !ogame->GetWorld())
        return 0;
    oCItem* item = (oCItem*)ogame->GetGameWorld()->CreateVob((zTVobType)0x81,
        GetCurrencyInstanceName());
    if (item)
        item->amount = amount;
    return item;
}

void oCItemContainer::RemoveCurrencyItem(oCItem* item)
{
    if (ogame && ogame->GetWorld())
        ogame->GetGameWorld()->RemoveVob(item);
}

void oCItemContainer::Container_PrepareDraw()
{
}

void oCItemContainer::Container_Draw()
{
    for (zCList<oCItemContainer>* node = s_openContainers.next; node; node = node->next)
        node->data->Draw();
}

int oCItemContainer::IsPassive() const
{
    return passive;
}

void oCItemContainer::GetPosition(int& x, int& y) const
{
    x = posx;
    y = posy;
}

oCItemContainer* oCItemContainer::GetNextContainerLeft(oCItemContainer* container)
{
    if (!container)
        return 0;
    int currentX;
    int currentY;
    container->GetPosition(currentX, currentY);
    oCItemContainer* result = 0;
    int bestX = currentX;
    for (zCList<oCItemContainer>* node = s_openContainers.next; node; node = node->next) {
        oCItemContainer* candidate = node->data;
        if (candidate == container || candidate->IsPassive() || !candidate->IsOpen())
            continue;
        int x;
        int y;
        candidate->GetPosition(x, y);
        if (x < bestX) {
            result = candidate;
            bestX = x;
        }
    }
    return result;
}

oCItemContainer* oCItemContainer::GetNextContainerRight(oCItemContainer* container)
{
    if (!container)
        return 0;
    int currentX;
    int currentY;
    container->GetPosition(currentX, currentY);
    oCItemContainer* result = 0;
    int bestX = currentX;
    for (zCList<oCItemContainer>* node = s_openContainers.next; node; node = node->next) {
        oCItemContainer* candidate = node->data;
        if (candidate == container || candidate->IsPassive() || !candidate->IsOpen())
            continue;
        int x;
        int y;
        candidate->GetPosition(x, y);
        if (x > bestX) {
            result = candidate;
            bestX = x;
        }
    }
    return result;
}

oCItem* oCNpc::GetTradeItem()
{
    if (trader && trader->GetContents()->next)
        return trader->GetContents()->next->data;
    return 0;
}

void oCNpc::OpenTradeContainer(oCItem* item, oCNpc* other, int amount)
{
    if (!item)
        return;
    if (!trader)
        trader = new oCItemContainer;
    tradeNpc = other;
    oCItem* removed = RemoveFromInv(item->GetInstance(), amount);
    zCList<oCItem>* entry = trader->GetContents()->next;
    oCItem* offered = entry ? entry->data : 0;
    if (offered && offered->MultiSlot() && removed &&
        offered->GetInstance() == removed->GetInstance()) {
        ++offered->amount;
        if (--removed->amount > 0)
            trader->Insert(removed);
        else
            ogame->GetGameWorld()->RemoveVob(removed);
        return;
    }
    if (offered) {
        trader->Remove2(offered);
        PutInInv(offered);
    }
    if (removed)
        trader->Insert(removed);
}

void oCNpc::CloseTradeContainer()
{
    if (trader) {
        zCList<oCItem>* entry = trader->GetContents()->next;
        if (entry && entry->data) {
            oCItem* item = entry->data;
            trader->Remove2(item);
            PutInInv(item);
        }
        trader->Close();
    }
    if (tradeNpc) {
        oCNpc* other = tradeNpc;
        tradeNpc = 0;
        other->tradeNpc = 0;
        other->CloseTradeContainer();
    }
    tradeNpc = 0;
}

void oCNpc::OpenTradeOffer(oCNpc* other)
{
    oCItem* item = other->GetTradeItem();
    if (!item)
        return;
    if (trader) {
        zCList<oCItem>* entry = trader->GetContents()->next;
        if (entry && entry->data) {
            oCItem* previous = entry->data;
            trader->Remove2(previous);
            PutInInv(previous);
        }
    }
    oCItemReactModule* module = ogame->GetTradeManager()->GetModule(other, this, item);
    if (!module)
        return;
    int instance = 0;
    int amount = 0;
    module->GetOfferedInstance(instance, amount);
    oCItem* offer = IsInInv(instance, amount);
    if (!offer) {
        offer = (oCItem*)ogame->GetGameWorld()->CreateVob((zTVobType)0x81, instance);
        offer->amount = amount;
        PutInInv(offer);
    }
    OpenTradeContainer(offer, other, IsAPlayer() ? 1 : amount);
}

int oCNpc::CheckItemReactModule(oCNpc* other, oCItem* item)
{
    oCItemReactModule* module = ogame->GetTradeManager()->GetModule(this, other, item);
    if (module && module->StartReaction(item->amount))
        ExchangeTradeModules(other);
    other->CloseTradeContainer();
    CloseTradeContainer();
    return 0;
}

void oCNpc::ExchangeTradeModules(oCNpc* other)
{
    oCItemContainer* otherTrader = other->trader;
    other->trader = trader;
    trader = otherTrader;
    if (!GetBodyState())
        GetModel()->StartAni("T_TRADEITEM", 0);
    if (!other->GetBodyState())
        other->GetModel()->StartAni("T_TRADEITEM", 0);
    other->CloseTradeContainer();
    CloseTradeContainer();
}

zSTRING oCItemContainer::GetName()
{
    return titleText;
}

void oCItemContainer::SetName(zSTRING& name)
{
    if (name.Length())
        titleText = name;
}

int oCItemContainer::GetMode()
{
    return invMode;
}

void oCItemContainer::SetMode(int mode)
{
    if (mode >= 0 && mode < 6)
        invMode = mode;
}

int oCItemContainer::IsActive()
{
    return frame;
}

void oCItemContainer::DisableManipulateItems(int disabled)
{
    m_bManipulateItemsDisabled = disabled;
}

int oCItemContainer::CanManipulateItems()
{
    return !m_bManipulateItemsDisabled;
}

void oCItemContainer::DisableTransferMoreThanOneItem(int disabled)
{
    m_bCanTransferMoreThanOneItem = !disabled;
}

int oCItemContainer::CanTransferMoreThanOneItem()
{
    return m_bCanTransferMoreThanOneItem;
}

short oCItemContainer::GetTransferCount()
{
    return TransferCount;
}

void oCItemContainer::SetTransferCount(short count)
{
    TransferCount = count;
}

void oCItemContainer::IncTransferCount(short count)
{
    TransferCount += count;
}

oCItemContainer::~oCItemContainer()
{
    Close();
    if (ownList)
        contents.DeleteListDatas();
    zCList<oCItemContainer>* previous = &contList;
    while (previous->next) {
        zCList<oCItemContainer>* node = previous->next;
        if (node->data == this) {
            previous->next = node->next;
            delete node;
            break;
        }
        previous = node;
    }
    npc = 0;
}

void oCItemContainer::Init(int x, int y, int mode)
{
    invMode = mode > 0 && mode <= 5 ? mode : 0;
    static int showArrows;
    showArrows = zoptions->ReadBool(zOPT_SEC_GAME, "invShowArrows", 1);
    marginTop = 0;
    marginLeft = 0;
    maxSlotsCol = GetInvMaxColumns();
    maxSlotsColScr = 1;
    maxSlotsRow = GetInvMaxRows();
    maxSlotsRowScr = 1;
    maxSlots = maxSlotsCol * maxSlotsRow;
    if (zsound) {
        zCSoundFX* sound = zsound->LoadSoundFXScript("INV_OPEN");
        zsound->PlaySound(sound, 0);
        if (sound)
            sound->Release();
    }
}

void oCItemContainer::GetSize(int& width, int& height)
{
    width = maxSlotsCol * 70;
    height = maxSlotsRow * 70;
}

int oCItemContainer::gfx_loaded;
zCGfx* oCItemContainer::gfx_cat;
zCGfx* oCItemContainer::gfx_equip;
zCGfx* oCItemContainer::gfx_cursor;
zCGfx* oCItemContainer::gfx_cursor_equip;
zCGfx* oCItemContainer::gfx_arrow;
zCGfx* oCItemContainer::gfx_arrowRight;
zCGfx* oCItemContainer::gfx_arrowUp;
zCGfx* oCItemContainer::gfx_arrowDown;

void oCItemContainer::LoadGrafix()
{
    if (gfx_loaded)
        return;
    gfx_cat = new zCGfx("invchest.tga");
    gfx_equip = new zCGfx("equip.tga");
    gfx_cursor = new zCGfx("cursor.tga");
    gfx_cursor_equip = new zCGfx("cursor_equip.tga");
    gfx_arrow = new zCGfx("arroleft.tga");
    gfx_arrowRight = new zCGfx("arrorite.tga");
    gfx_arrowUp = new zCGfx("arroup.tga");
    gfx_arrowDown = new zCGfx("arrodown.tga");
    gfx_loaded = 1;
}

zCList<oCItem>* oCItemContainer::JumpOffset(int& firstPage, int& lastPage)
{
    firstPage = 1;
    lastPage = 0;
    if (selectedItem < 0)
        return contents.next;
    int count = 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next)
        ++count;
    if (selectedItem >= count)
        selectedItem = count - 1;
    if (selectedItem < offset) {
        offset -= maxSlotsRow;
        if (offset < 0)
            offset = 0;
    }
    zCList<oCItem>* node = contents.next;
    for (int i = 0; node && i < offset; ++i)
        node = node->next;
    firstPage = offset == 0;
    return node;
}

void oCItemContainer::DrawCategory()
{
    if (!IsPassive() && titleText.Length() && viewTitle)
        viewTitle->Print(posx + marginLeft, posy + marginTop, titleText + "\n");
}

void oCItemContainer::DrawItemInfo(oCItem* item, zCWorld* world)
{
    if (!GetEnableHandleEvent() || !viewItemInfo)
        return;
    screen->InsertItem(viewItemInfo, 0);
    if (item) {
        if (world) {
            viewItemInfo->InsertItem(viewItemInfoItem, 0);
            item->SetLightingSwell(0);
            item->RenderItem(world, viewItemInfoItem, 0.0f);
            item->SetLightingSwell(1);
        }
        zSTRING description = item->GetDescription();
        if (description.IsEmpty())
            description = item->GetName();
        viewItemInfo->PrintTimedCX(0, description, -2.0f);
        for (int i = 0; i < 6; ++i) {
            zSTRING text = item->GetText(i);
            int count = item->GetCount(i);
            viewItemInfo->PrintTimed(250, (i + 1) * 250, text, -2.0f);
            if (count) {
                zSTRING valueText = zSTRING((long)count);
                if (count >= 5) {
                    int value = item->GetValue();
                    if (invMode == 5) {
                        value = (int)(value * GetValueMultiplier());
                        if (item->GetValue() > 0 && value <= 0)
                            value = 1;
                    }
                    valueText = zSTRING((long)value);
                }
                viewItemInfo->PrintTimed(7940 - viewItemInfo->FontSize(valueText),
                    (i + 1) * 250, valueText, -2.0f);
            }
        }
    }
    viewItemInfo->Blit();
    if (world)
        viewItemInfo->RemoveItem(viewItemInfoItem);
    screen->RemoveItem(viewItemInfo);
}

void oCItemContainer::Draw()
{
    if (!IsOpen())
        return;
    CheckSelectedItem();
    maxSlotsColScr = 8192 / 70;
    if (IsSplitScreen())
        maxSlotsColScr /= 2;
    if (maxSlotsColScr < 2)
        maxSlotsColScr = 2;
    if (maxSlotsCol > maxSlotsColScr)
        maxSlotsCol = maxSlotsColScr;
    maxSlotsRowScr = 8192 / 70;
    if (maxSlotsRowScr < 2)
        maxSlotsRowScr = 2;
    if (maxSlotsRow > maxSlotsRowScr)
        maxSlotsRow = maxSlotsRowScr;
    maxSlots = maxSlotsCol * maxSlotsRow;
    DrawCategory();
    screen->InsertItem(viewBack, 0);
    int firstPage;
    int lastPage;
    zCList<oCItem>* node = JumpOffset(firstPage, lastPage);
    oCItem* selected = GetSelectedItem();
    int slotWidth = 8192 / maxSlotsCol;
    int slotHeight = 8192 / maxSlotsRow;
    for (int i = 0; i < maxSlots; ++i) {
        oCItem* item = node ? node->data : 0;
        if (item) {
            int selectedItem = item == selected;
            int equipped = item->HasFlag(0x40000000);
            zCView* slot = selectedItem
                ? (equipped ? viewItemActiveHighlighted : viewItemHightlighted)
                : (equipped ? viewItemActive : viewItem);
            viewBack->InsertItem(slot, 0);
            slot->SetTransparency(GetEnableHandleEvent() ? 255 : 64);
            slot->SetPos((i % maxSlotsCol) * slotWidth, (i / maxSlotsCol) * slotHeight);
            slot->SetSize(slotWidth, slotHeight);
            slot->Blit();
            item->SetLightingSwell(0);
            item->RenderItem(rndWorld, slot, selectedItem && IsActive() ? 1.0f : 0.0f);
            item->SetLightingSwell(1);
            viewBack->RemoveItem(slot);
            if (selectedItem && IsActive())
                DrawItemInfo(item, rndWorld);
            if (item->amount > 1 && textView) {
                viewBack->InsertItem(textView, 0);
                textView->Print(slotWidth - 1000 - textView->FontSize(zSTRING((long)item->amount)),
                    slotHeight - 800 - textView->FontY(), zSTRING((long)item->amount));
                viewBack->RemoveItem(textView);
            }
        }
        if (node)
            node = node->next;
    }
    screen->RemoveItem(viewBack);
}

int oCItemContainer::IsSplitScreen()
{
    int mode = GetMode();
    if (mode == 1 || (mode > 3 && mode <= 5))
        return GetInvSplitScreen();
    oCItemContainer* left = GetNextContainerLeft(this);
    if (left) {
        mode = left->GetMode();
        if (mode == 1 || (mode > 3 && mode <= 5))
            return GetInvSplitScreen();
    }
    return 0;
}

void oCItemContainer::SetContents(zCList<oCItem>* newContents)
{
    DeleteContents();
    ownList = 0;
    contents.data = newContents ? newContents->data : 0;
    contents.next = newContents ? newContents->next : 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next) {
        if (node->data)
            node->data->RotateForInventory(1);
    }
}

void oCItemContainer::Open(int x, int y, int mode)
{
    OpenPassive(x, y, mode);
    Activate();
    passive = 0;
    if (!IsOpen())
        s_openContainers.Insert(this);
}

void oCItemContainer::OpenPassive(int x, int y, int mode)
{
    if (rndWorld)
        rndWorld->Release();
    rndWorld = new zCWorld;
    zCClassDef::ObjectCreated(rndWorld, zCWorld::classDef);
    rndWorld->m_bIsInventoryWorld = 1;
    posx = x;
    posy = y;
    right = x > 4095;
    Init(x, y, mode);
    viewArrowAtTop = new zCView(0, 0, 70, 70, 2);
    screen->InsertItem(viewArrowAtTop);
    viewArrowAtTop->InsertBack(inventoryOTexture);
    viewArrowAtTop->SetAlphaBlendFunc(2);
    viewArrowAtTop->SetTransparency(96);
    screen->RemoveItem(viewArrowAtTop);
    viewArrowAtBottom = new zCView(0, 0, 70, 70, 2);
    screen->InsertItem(viewArrowAtBottom);
    viewArrowAtBottom->InsertBack(inventoryUTexture);
    viewArrowAtBottom->SetAlphaBlendFunc(2);
    viewArrowAtBottom->SetTransparency(96);
    screen->RemoveItem(viewArrowAtBottom);
    viewTitle = new zCView(0, 0, 100, 100, 2);
    screen->InsertItem(viewTitle);
    viewTitle->InsertBack(inventoryTitleTexture);
    viewTitle->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewTitle);
    viewBack = new zCView(0, 0, 100, 100, 2);
    screen->InsertItem(viewBack);
    viewBack->InsertBack(inventoryBackTexture);
    viewBack->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewBack);
    viewItem = new zCView(0, 0, 70, 70, 2);
    screen->InsertItem(viewItem);
    viewItem->InsertBack(inventorySlotTexture);
    viewItem->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewItem);
    viewItemActive = new zCView(0, 0, 70, 70, 2);
    screen->InsertItem(viewItemActive);
    viewItemActive->InsertBack(inventorySlotEquippedTexture);
    viewItemActive->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewItemActive);
    viewItemHightlighted = new zCView(0, 0, 70, 70, 2);
    screen->InsertItem(viewItemHightlighted);
    viewItemHightlighted->InsertBack(inventorySlotHighlightedTexture);
    viewItemHightlighted->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewItemHightlighted);
    viewItemActiveHighlighted = new zCView(0, 0, 70, 70, 2);
    screen->InsertItem(viewItemActiveHighlighted);
    viewItemActiveHighlighted->InsertBack(inventorySlotEquippedHighlightedTexture);
    viewItemActiveHighlighted->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewItemActiveHighlighted);
    textView = new zCView(0, 0, 70, 70, 2);
    viewItemInfo = new zCView(0, 0, 100, 100, 2);
    screen->InsertItem(viewItemInfo);
    viewItemInfo->InsertBack(inventoryDescTexture);
    viewItemInfo->SetAlphaBlendFunc(2);
    screen->RemoveItem(viewItemInfo);
    viewItemInfoItem = new zCView(0, 0, 100, 100, 2);
    viewItemInfo->InsertItem(viewItemInfoItem, 0);
    viewItemInfoItem->SetAlphaBlendFunc(2);
    viewItemInfo->RemoveItem(viewItemInfoItem);
    if (!IsOpen())
        s_openContainers.Insert(this);
    if (m_bManipulateItemsDisabled)
        CheckSelectedItem();
    passive = 1;
    open = 1;
}

void oCItemContainer::CheckSelectedItem()
{
    if (GetSelectedItem() && !m_bManipulateItemsDisabled)
        return;
    if (!contents.next) {
        selectedItem = -1;
        offset = 0;
    } else if (selectedItem < 0) {
        selectedItem = 0;
        offset = 0;
    }
    for (int i = 0; i < 10 && GetSelectedItem(); ++i) {
        if (!GetSelectedItem()->HasFlag(0x40000000))
            break;
        NextItem();
    }
    for (int i = 0; i < 10 && GetSelectedItem(); ++i) {
        if (!GetSelectedItem()->HasFlag(0x40000000))
            break;
        PrevItem();
    }
}

void oCItemContainer::NextItem()
{
    if (!contents.next)
        return;
    int oldSelected = selectedItem;
    int oldOffset = offset;
    if (GetSelectedItem())
        GetSelectedItem()->RotateForInventory(1);
    int count = 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next)
        ++count;
    if (selectedItem >= count - 1) {
        selectedItem = oldSelected;
        offset = oldOffset;
        if (IsSplitScreen())
            ActivateNextContainer(1);
        return;
    }
    ++selectedItem;
    if (selectedItem > maxSlots + offset - 1)
        offset += maxSlotsCol;
}

void oCItemContainer::NextItemLine()
{
    if (!contents.next)
        return;
    int oldSelected = selectedItem;
    int oldOffset = offset;
    if (GetSelectedItem())
        GetSelectedItem()->RotateForInventory(1);
    int count = 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next)
        ++count;
    if (selectedItem >= count - 1) {
        selectedItem = oldSelected;
        offset = oldOffset;
        return;
    }
    if (selectedItem >= count - maxSlotsCol)
        ++selectedItem;
    else
        selectedItem += maxSlotsCol;
    if (selectedItem >= count)
        selectedItem = count - 1;
    if (selectedItem > maxSlots + offset - 1)
        offset += maxSlotsCol;
}

void oCItemContainer::PrevItem()
{
    if (!contents.next)
        return;
    int oldSelected = selectedItem;
    int oldOffset = offset;
    if (GetSelectedItem())
        GetSelectedItem()->RotateForInventory(1);
    if (selectedItem <= 0) {
        selectedItem = oldSelected;
        offset = oldOffset;
        if (IsSplitScreen())
            ActivateNextContainer(-1);
        return;
    }
    --selectedItem;
    if (selectedItem < offset)
        offset -= maxSlotsCol;
    if (offset < 0)
        offset = 0;
}

void oCItemContainer::PrevItemLine()
{
    if (!contents.next || selectedItem < 0)
        return;
    int oldSelected = selectedItem;
    int oldOffset = offset;
    if (GetSelectedItem())
        GetSelectedItem()->RotateForInventory(1);
    if (selectedItem <= 0) {
        selectedItem = oldSelected;
        offset = oldOffset;
        return;
    }
    selectedItem = selectedItem < maxSlotsCol ? selectedItem - 1 : selectedItem - maxSlotsCol;
    if (selectedItem < 0)
        selectedItem = 0;
    if (selectedItem < offset)
        offset -= maxSlotsCol;
    if (offset < 0)
        offset = 0;
}

int oCItemContainer::IsEmpty() const
{
    return contents.next == 0;
}

int oCItemContainer::TransferItem(int direction, int amount)
{
    oCItemContainer* target = direction >= 0 ? GetNextContainerRight(this) : GetNextContainerLeft(this);
    oCItem* item = GetSelectedItem();
    if (!target || !item)
        return 0;
    int transferAmount = 1;
    if (CanTransferMoreThanOneItem()) {
        transferAmount = amount;
        if (item->amount < transferAmount)
            transferAmount = item->amount;
    }
    ++item->refCtr;
    for (int i = 0; i < transferAmount; ++i) {
        oCItem* transferred = Remove(item, 1);
        if (transferred)
            target->Insert(transferred);
    }
    item->Release();
    return 1;
}

int oCItemContainer::ActivateNextContainer(int direction)
{
    oCItemContainer* target = direction >= 0 ? GetNextContainerRight(this) : GetNextContainerLeft(this);
    if (target)
        target->Activate();
    return (int)target;
}

void oCItemContainer::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    int count = 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next)
        ++count;
    archiver.WriteGroupBegin("INVENTORY");
    archiver.WriteInt(count);
    int index = 0;
    for (zCList<oCItem>* node = contents.next; node; node = node->next, ++index)
        archiver.WriteObject(node->data);
    archiver.WriteGroupEnd();
}

void oCItemContainer::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
        Insert((oCItem*)archiver.ReadObject(0));
}

static int ContainerTakeItem(oCItemContainer* container)
{
    oCItem* item = container->GetSelectedItem();
    if (!item || !oCNpc::player->CanCarry(item))
        return 0;
    int amount = container->GetTransferCount();
    if (amount <= 0 || amount > item->amount)
        amount = item->amount;
    oCItem* taken = container->RemoveByPtr(item, amount);
    oCNpc::player->PutInInv(taken);
    return 1;
}

int oCItemContainer::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (zinput->IsBindedToggled(5, key) || (key == 0x20c && zinput->KeyToggled(key)))
        SetTransferCount(0);
    switch (key) {
    case 1:
    case 0x12:
        PrevItem();
        return 1;
    case 2:
    case 0x13:
        NextItem();
        return 1;
    case 3:
    case 0x20a:
        PrevItemLine();
        return 1;
    case 4:
    case 0x20b:
        NextItemLine();
        return 1;
    case 5:
    case 0xb:
    case 0x20c: {
        oCItem* item = GetSelectedItem();
        if (item && oCNpc::player->CanCarry(item)) {
            short amount = TransferCountToAmount(GetTransferCount());
            if (GetTransferCount() + amount < item->amount)
                IncTransferCount(amount);
            else
                SetTransferCount(item->amount);
            ContainerTakeItem(this);
        }
        return 1;
    }
    case 0xe:
    case 0xf:
    case 0x20d:
        Close();
        oCNpc::player->CloseInventory();
        return 1;
    default:
        return 0;
    }
}

oCStealContainer::oCStealContainer()
    : owner(0)
{
}

oCStealContainer::~oCStealContainer()
{
    ownList = 0;
    contents.data = 0;
    contents.next = 0;
    owner = 0;
}

void oCStealContainer::SetOwner(oCNpc* npc)
{
    owner = npc;
    CreateList();
}

oCNpc* oCStealContainer::GetOwner()
{
    return owner;
}

void oCStealContainer::CreateList()
{
    ownList = 0;
    contents.DeleteList();
    int count = owner->inventory2.GetNumItemsInCategory();
    for (int i = 0; i < count; ++i) {
        oCItem* item = owner->inventory2.GetItem(i);
        if (!item || item->HasFlag(0x10) || item->HasFlag(0x40000000))
            continue;
        if (owner->GetTradeNpc() && owner->GetTradeNpc()->IsAPlayer() &&
            item->GetInstanceName() == GetCurrencyInstanceName())
            continue;
        Insert(item);
    }
}

int oCStealContainer::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (key == 0xe || key == 0xf || key == 0x20d) {
        Close();
        owner->StopTheft(oCNpc::player);
        return 1;
    }
    if (key == 1 || key == 0x12) {
        PrevItem();
        return 1;
    }
    if (key == 2 || key == 0x13) {
        NextItem();
        return 1;
    }
    if (key == 3 || key == 0x20a) {
        PrevItemLine();
        return 1;
    }
    if (key == 4 || key == 0x20b) {
        NextItemLine();
        return 1;
    }
    if (key != 5 && key != 0xb && key != 0x20c)
        return 0;
    oCItem* item = GetSelectedItem();
    if (item && owner) {
        if (item->HasFlag(0x40000000) || std::rand() % 100 >= oCNpc::player->GetTalentValue(6)) {
            owner->StopTheft(oCNpc::player);
        } else {
            ContainerTakeItem(this);
            if (IsEmpty())
                owner->StopTheft(oCNpc::player);
        }
    }
    return 1;
}

oCNpcContainer::oCNpcContainer()
{
}

oCNpcContainer::~oCNpcContainer()
{
    ownList = 0;
    contents.data = 0;
    contents.next = 0;
    owner = 0;
}

void oCNpcContainer::CreateList()
{
    ownList = 0;
    int count = owner->inventory2.GetNumItemsInCategory();
    for (int i = 0; i < count; ++i) {
        oCItem* item = owner->inventory2.GetItem(i);
        if (item && !item->HasFlag(0x10) && !item->HasFlag(0x40000000))
            oCItemContainer::Insert(item);
    }
}

int oCNpcContainer::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (key == 0xe || key == 0xf || key == 0x20d) {
        Close();
        oCNpc::player->CloseDeadNpc(oCNpc::player);
        return 1;
    }
    if (key == 1 || key == 0x12) {
        PrevItem();
        return 1;
    }
    if (key == 2 || key == 0x13) {
        NextItem();
        return 1;
    }
    if (key == 3 || key == 0x20a) {
        PrevItemLine();
        return 1;
    }
    if (key == 4 || key == 0x20b) {
        NextItemLine();
        return 1;
    }
    if (key != 5 && key != 0xb && key != 0x20c)
        return 0;
    oCItem* item = GetSelectedItem();
    if (item) {
        if (item->HasFlag(0x40000000))
            owner->UnequipItem(item);
        ContainerTakeItem(this);
        if (IsEmpty())
            oCNpc::player->CloseDeadNpc(oCNpc::player);
    }
    return 1;
}

oCItem* oCNpcContainer::Insert(oCItem* item)
{
    if (!item)
        return 0;
    oCItem* result = oCItemContainer::Insert(item);
    if (owner && ownList)
        owner->PutInInv(result);
    return result;
}

void oCNpcContainer::Remove(oCItem* item)
{
    if (!item)
        return;
    oCItemContainer::Remove(item);
    if (owner && ownList)
        owner->RemoveFromInv(item, item->amount);
}
