#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"

oCNpcInventory::oCNpcInventory()
    : packAbility(1)
{
    inventory.data = 0;
    inventory.next = 0;
    owner = 0;
    ownList = 0;
}

oCNpcInventory::~oCNpcInventory()
{
    ClearInventory();
    owner = 0;
}

void oCNpcInventory::ClearInventory()
{
    zCList<oCItem>* node = inventory.next;
    while (node) {
        zCList<oCItem>* next = node->next;
        if (node->data)
            node->data->Release();
        delete node;
        node = next;
    }
    inventory.next = 0;
    inventory.data = 0;
    packString.Clear();
}

void oCNpcInventory::Open(int x, int y, int mode)
{
    UnpackItemsInCategory(0);
    oCItem* rightItem = (oCItem*)owner->GetRightHand();
    if (rightItem && rightItem->HasFlag(0x20000000)) {
        if (rightItem->GetInstanceName() == "ITLSTORCHBURNING") {
            owner->RemoveFromSlot(&NPC_NODE_RIGHTHAND, 0, 1);
            owner->PutInInv(rightItem);
        } else {
            owner->SetTorchAni(0, 1);
            oCItem* torch = (oCItem*)ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM, "ITLSTORCH");
            owner->RemoveFromSlot(&NPC_NODE_RIGHTHAND, 0, 1);
            owner->PutInInv(torch);
            rightItem->Release();
        }
    }
    oCItem* leftItem = (oCItem*)owner->GetLeftHand();
    if (leftItem && leftItem->HasFlag(0x20000000) &&
        leftItem->GetInstanceName() == "ITLSTORCHBURNING") {
        owner->RemoveFromSlot(&NPC_NODE_LEFTHAND, 0, 1);
        owner->PutInInv(leftItem);
    }
    OpenPassive(x, y, mode);
    Activate();
    passive = 0;
    if (!IsOpen())
        s_openContainers.Insert(this);
    if (owner)
        owner->SetBodyState(13);
    zinput->ClearKeyBuffer();
    zinput->ResetRepeatKey(1);
}

void oCNpcInventory::Close()
{
    oCItemContainer::Close();
}

void oCNpcInventory::Draw()
{
    oCItemContainer::Draw();
}

void oCNpcInventory::SetOwner(oCNpc* npc)
{
    owner = npc;
}

oCNpc* oCNpcInventory::GetOwner()
{
    return owner;
}

int oCNpcInventory::GetNumItemsInPackString() const
{
    if (!packAbility || packString.IsEmpty())
        return 0;
    int count = 0;
    for (int i = 0; i < packString.Length(); ++i) {
        if (packString[i] == ':')
            ++count;
    }
    return count;
}

int oCNpcInventory::GetNumItemsInCategory()
{
    int count = GetNumItemsInPackString();
    for (zCList<oCItem>* node = inventory.next; node; node = node->next)
        ++count;
    return count;
}

int oCNpcInventory::GetNumItemsInCategory(int category)
{
    return GetNumItemsInCategory();
}

oCItem* oCNpcInventory::GetItem(int index)
{
    if (index < 0 || index >= GetNumItemsInCategory())
        return 0;
    int current = 0;
    for (zCList<oCItem>* node = inventory.next; node; node = node->next, ++current) {
        if (current == index) {
            if (node->data)
                return node->data;
            break;
        }
    }
    if (packAbility) {
        zSTRING entry;
        int listCount = current;
        if (GetPackedItemBySlot(index - listCount, entry))
            return CreateFromPackString(entry);
    }
    return 0;
}

oCItem* oCNpcInventory::GetItem(int index, int category)
{
    return GetItem(index);
}

int oCNpcInventory::GetCategory(oCItem* item)
{
    if (!item)
        return 0;
    switch (item->mainflag) {
    case 2:
    case 4:
    case 8:
        return 1;
    case 0x10:
        return 2;
    case 0x20:
        return 5;
    case 0x40:
        return 7;
    case 0x80:
        return 6;
    case 0x200:
        return 3;
    case 0x80000000:
        return 4;
    default:
        return 8;
    }
}

oCItem* oCNpcInventory::Insert(oCItem* item)
{
    if (!item)
        return 0;
    if (item->GetInstance() < 0 || item->GetName().IsEmpty()) {
        zerr->Message("M: Invalid item (no name, no instance) put into inventory. Deleting it.");
        item->Release();
        return 0;
    }
    if (item->MultiSlot()) {
        for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
            oCItem* current = node->data;
            if (current->MultiSlot() && current->GetInstance() == item->GetInstance()) {
                current->amount += item->amount;
                ogame->GetGameWorld()->RemoveVob(item);
                return current;
            }
        }
    }
    inventory.Insert(item);
    CheckSelectedItem();
    ++item->refCtr;
    ogame->GetGameWorld()->RemoveVob(item);
    return item;
}

int oCNpcInventory::GetAmount(int instance)
{
    int amount = 0;
    for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
        if (node->data->GetInstance() == instance)
            amount += node->data->amount;
    }
    return amount;
}

oCItem* oCNpcInventory::IsIn(oCItem* item, int amount)
{
    if (item) {
        for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
            if (node->data == item && node->data->amount >= amount)
                return node->data;
        }
    }
    return 0;
}

oCItem* oCNpcInventory::IsIn(const zSTRING& name, int amount)
{
    return IsIn(parser.GetIndex(name), amount);
}

oCItem* oCNpcInventory::IsIn(int instance, int amount)
{
    for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
        if (node->data->GetInstance() == instance && node->data->amount >= amount)
            return node->data;
    }
    return 0;
}

void oCNpcInventory::Remove(oCItem* item)
{
    if (!item)
        return;
    zCList<oCItem>* previous = &inventory;
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
    item->Release();
}

oCItem* oCNpcInventory::RemoveByPtr(oCItem* item, int amount)
{
    return Remove(item, amount);
}

oCItem* oCNpcInventory::Remove(oCItem* item, int amount)
{
    if (!item)
        return 0;
    for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
        oCItem* current = node->data;
        if (current != item || current->HasFlag(0x40000000))
            continue;
        if (!current->MultiSlot() || current->amount <= amount) {
            zCList<oCItem>* previous = &inventory;
            while (previous->next) {
                zCList<oCItem>* entry = previous->next;
                if (entry->data == current) {
                    previous->next = entry->next;
                    delete entry;
                    break;
                }
                previous = entry;
            }
            CheckSelectedItem();
            ogame->GetGameWorld()->AddVob(current);
            current->Release();
            return current;
        }
        current->amount -= amount;
        oCItem* result = (oCItem*)ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM,
            current->GetInstance());
        result->amount = amount;
        ogame->GetGameWorld()->AddVob(result);
        result->Release();
        return result;
    }
    return 0;
}

oCItem* oCNpcInventory::Remove(int instance, int amount)
{
    for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
        if (node->data->GetInstance() == instance)
            return Remove(node->data, amount);
    }
    return 0;
}

oCItem* oCNpcInventory::Remove(const zSTRING& name, int amount)
{
    return Remove(parser.GetIndex(name), amount);
}

int oCNpcInventory::IsEmpty(int ignoreMissionItems, int ignoreEquipped)
{
    if (packAbility)
        UnpackItemsInCategory(0);
    for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
        oCItem* item = node->data;
        if (item && (!ignoreMissionItems || item->mainflag != 0x10) &&
            (!ignoreEquipped || !item->HasFlag(0x40000000)))
            return 0;
    }
    return 1;
}

void oCNpcInventory::Archive(zCArchiver& archiver)
{
    PackItemsInCategory(0);
    archiver.WriteString(packString);
    int count = 0;
    for (zCList<oCItem>* node = inventory.next; node; node = node->next)
        ++count;
    archiver.WriteGroupBegin("INVENTORY");
    archiver.WriteInt(count);
    int index = 0;
    for (zCList<oCItem>* node = inventory.next; node; node = node->next, ++index) {
        archiver.WriteObject(node->data);
        if (node->data->mainflag == 0x200) {
            int shortKey = 0;
            if (owner && owner->GetSpellBook())
                shortKey = owner->GetSpellBook()->GetKeyByItem(node->data);
            archiver.WriteInt(shortKey);
        }
    }
    archiver.WriteGroupEnd();
}

void oCNpcInventory::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    packString = archiver.ReadString();
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i) {
        oCItem* item = (oCItem*)archiver.ReadObject(0);
        if (!item) {
            zerr->Message("C: could not read inventory item");
            break;
        }
        if (item->mainflag == 0x200) {
            int shortKey = archiver.ReadInt();
            if (owner && owner->GetSpellBook())
                owner->GetSpellBook()->NextRegisterAt(shortKey);
        }
        if (item->HasFlag(0x40000000)) {
            item->ClearFlag(0x40000000);
            owner->Equip(item);
        } else {
            Insert(item);
        }
        item->Release();
    }
}

int Inventory_GetCategory(oCItem* item)
{
    if (!item)
        return 0;
    switch (item->mainflag) {
    case 2:
    case 4:
    case 8:
        return 1;
    case 0x10:
        return 2;
    case 0x20:
        return 5;
    case 0x40:
        return 7;
    case 0x80:
        return 6;
    case 0x200:
        return 3;
    case 0x80000000:
        return 4;
    default:
        return 8;
    }
}

zSTRING _GetCategorySelfPlayerPrefix()
{
    static zSTRING prefix;
    if (prefix.IsEmpty()) {
        zCPar_Symbol* symbol = parser.GetSymbol("NAME_Currency");
        if (symbol)
            symbol->GetValue(prefix, 0);
        if (prefix.IsEmpty())
            prefix = "Gold: ";
    }
    return prefix;
}

void oCNpcInventory::DrawCategory()
{
    if (owner && owner->IsAPlayer() && viewTitle) {
        zSTRING text = _GetCategorySelfPlayerPrefix() +
            zSTRING((long)GetAmount(oCItemContainer::GetCurrencyInstance()));
        viewTitle->Print(posx, posy, text);
    }
    oCItemContainer::DrawCategory();
}

int oCNpcInventory::HandleTrade(int key)
{
    zerr->Message("oCNpcInventory::HandleTrade() has been called although it is obsolete!");
    oCNpc* focus = owner->GetFocusNpc();
    if (!focus) {
        owner->CloseTradeContainer();
        return 1;
    }
    if (key == 2 || key == 0x13) {
        focus->CloseTradeContainer();
        owner->CloseTradeContainer();
    } else if (key == 1 || key == 0x12) {
        oCItem* selected = GetSelectedItem();
        int amount = owner->IsAPlayer() ? 1 : (selected ? selected->amount : 1);
        owner->OpenTradeContainer(selected, focus, amount);
        if (focus->GetPerceptionFunc(20) > 0)
            focus->OpenTradeOffer(owner);
    } else if (key == 5) {
        if (focus->GetPerceptionFunc(20) <= 0)
            owner->CloseTradeContainer();
        else {
            oCItem* tradeItem = owner->GetTradeItem();
            focus->AssessGivenItem(tradeItem, owner);
        }
    }
    return 1;
}

int oCNpcInventory::HandleEvent(int key)
{
    if (!IsActive() || !owner->GetEM(0)->IsEmpty(1) || owner->interactItemCurrentState != -1)
        return 0;
    return oCItemContainer::HandleEvent(key);
}

void oCNpcInventory::CheckForEquippedItems(int ignoreMissionItems)
{
    for (zCList<oCItem>* node = inventory.next; node; node = node->next) {
        oCItem* item = node->data;
        if (!item || !item->HasFlag(0x40000000))
            continue;
        int category = GetCategory(item);
        if (category == 2 && ignoreMissionItems)
            continue;
        item->ClearFlag(0x40000000);
        if (!item->HasFlag(2)) {
            if (item->HasFlag(4))
                owner->EquipFarWeapon(item);
            else if (category == 2)
                owner->EquipArmor(item);
            else if (category == 3)
                owner->Equip(item);
        } else {
            owner->EquipWeapon(item);
        }
    }
}

int oCNpcInventory::CanCarry(oCItem* item)
{
    return GetNumItemsInCategory() < 1024;
}

void oCNpcInventory::SetPackAbility(int enabled)
{
    if (!enabled)
        UnpackItemsInCategory(0);
    packAbility = enabled;
}

void oCNpcInventory::UnpackCategory()
{
    if (packAbility)
        UnpackItemsInCategory(0);
}

int oCNpcInventory::GetPackedItemBySlot(int slot, zSTRING& entry)
{
    int start = 0;
    for (int i = 0; i < slot; ++i) {
        int comma = packString.Search(",", start);
        if (comma < 0) {
            entry = "";
            return 0;
        }
        start = comma + 1;
    }
    int end = packString.Search(",", start);
    if (end < 0)
        end = packString.Length();
    entry = packString.Copied(start, end - start);
    return entry.Length();
}

int oCNpcInventory::GetPackedItemInfo(const zSTRING& entry, int remove, int& amount, int& equipped)
{
    amount = 0;
    equipped = 0;
    if (!packAbility || entry.IsEmpty())
        return 0;
    int position = packString.Search(entry + ":");
    if (position < 0 || (position > 0 && packString[position - 1] != ','))
        return 0;
    int valueStart = position + entry.Length() + 1;
    int end = packString.Search(",", valueStart);
    if (end < 0)
        end = packString.Length();
    zSTRING value = packString.Copied(valueStart, end - valueStart);
    amount = value.ToInt();
    if (amount <= 0)
        amount = 1;
    if (value.Length() && value[value.Length() - 1] == 'E')
        equipped = 1;
    if (remove) {
        if (end < packString.Length())
            packString.Delete(position, end - position + 1);
        else if (position > 0)
            packString.Delete(position - 1, end - position + 1);
        else
            packString.Delete(position, end - position);
    }
    return 1;
}

oCItem* oCNpcInventory::CreateFromPackString(const zSTRING& entry)
{
    if (!packAbility)
        return 0;
    int amount;
    int equipped;
    if (!GetPackedItemInfo(entry, 1, amount, equipped))
        return 0;
    oCItem* item = (oCItem*)ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM,
        parser.GetIndex(entry.Copied(0, entry.Search(":"))));
    if (item) {
        item->amount = amount;
        oCItem* result = Insert(item);
        item->Release();
        return result;
    }
    return 0;
}

int oCNpcInventory::PackSingleItem(oCItem* item)
{
    if (!packAbility || !item)
        return 0;
    zSTRING instanceName = item->GetSchemeName();
    if (item->MultiSlot()) {
        int oldAmount;
        int oldEquipped;
        if (GetPackedItemInfo(instanceName, 1, oldAmount, oldEquipped))
            item->amount += oldAmount;
    }
    oCItem* removed = RemoveByPtr(item, item->amount);
    if (!packString.IsEmpty())
        packString.Append(",");
    packString.Append(instanceName + ":" + zSTRING((long)removed->amount));
    if (removed->HasFlag(0x40000000))
        packString.Append("E");
    removed->Release();
    return 1;
}

void oCNpcInventory::PackAllItems(int category)
{
    PackItemsInCategory(category);
}

void oCNpcInventory::UnpackAllItems()
{
    UnpackItemsInCategory();
}

void oCNpcInventory::PackItemsInCategory(int category)
{
    if (!packAbility)
        return;
    zCList<oCItem>* node = inventory.next;
    while (node) {
        zCList<oCItem>* next = node->next;
        if (node->data && (!category || GetCategory(node->data) == category))
            PackSingleItem(node->data);
        node = next;
    }
}

void oCNpcInventory::UnpackItemsInCategory()
{
    if (!packAbility)
        return;
    while (!packString.IsEmpty()) {
        zSTRING entry;
        if (!GetPackedItemBySlot(0, entry))
            break;
        int equipped = entry.Search("E") >= 0;
        oCItem* item = CreateFromPackString(entry);
        if (item && equipped && owner)
            owner->Equip(item);
    }
    packString.Clear();
}

void oCNpcInventory::UnpackItemsInCategory(int category)
{
    UnpackItemsInCategory();
}
