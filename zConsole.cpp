#include "include/zConsole.h"
#include "include/zMoving.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zFile.h"

static int consoleState = -1;
static int consoleRuntimeState = -1;
zCList<zCConsole> zCConsole::active_consoles = {};
zCConsole* zCConsole::cur_console = NULL;
zCConsole zcon;

zCConDat::zCConDat(const zSTRING& newHint, const zSTRING& newName, int newType, void* address,
    int elements)
    : hint(newHint), name(newName), type(newType), adr(address), ele(elements), var(1), next(NULL)
{
    name.Upper();
}

zCConDat::zCConDat(const zSTRING& newHint, const zSTRING& newName, int newType)
    : hint(newHint), name(newName), type(newType), adr(NULL), ele(0), var(0), next(NULL)
{
    name.Upper();
}

int zCConsole::HandleEvent(int key)
{
    if (!cur_console)
        return 0;
    if (key == 1 || key == 0x2B || key == 0x3C || key == 0xC9 || key == 0xD1 ||
        key == 0xD3 || key == 0xD2)
    {
        cur_console->AddChar(key, 0);
        return 1;
    }
    int shift = zinput->KeyPressed(0x36) || zinput->KeyPressed(0x2A) ? 0x2A : 0;
    int caps = zinput->KeyPressed(0xB8) ? 0xB8 : 0;
    int character = GetCharFromDIK(key, shift, caps);
    if (character == 8 || character == 13)
    {
        cur_console->AddChar(character, 0);
        return 1;
    }
    if (character < 0x20 || character > 0xFF)
        return 0;
    cur_console->AddChar(character, 1);
    return 1;
}

zCConsole::zCConsole()
    : lastCommandPos(0), px(0), py(0), lx(0), ly(0), showmax(0), skip(0), dynsize(0), var(0),
      autocomplete(1), conview(NULL), evalcount(0), changedfunc(NULL), world(NULL), cparser(NULL),
      edit_index(0), edit_adr(NULL)
{
    for (int i = 0; i < zCON_MAX_EVAL; ++i)
        evalfunc[i] = NULL;
    active_consoles.Insert(this);
    list.InsertLast(new zCConDat("save infofile", "save infofile with current console-commands", 0));
    Register("help", "list commands on spy");
}

zCConsole::zCConsole(int x, int y, const zSTRING& name)
    : id(name), lastCommandPos(0), px(x), py(y), lx(4000), ly(0), showmax(0), skip(0), dynsize(1),
      var(0), autocomplete(1), conview(NULL), evalcount(0), changedfunc(NULL), world(NULL),
      cparser(NULL), edit_index(0), edit_adr(NULL)
{
    for (int i = 0; i < zCON_MAX_EVAL; ++i)
        evalfunc[i] = NULL;
    active_consoles.Insert(this);
}

zCConsole::~zCConsole()
{
    if (conview && conview->ondesk)
    {
        cur_console = NULL;
        screen->RemoveItem(conview);
        RemoveFocus();
    }
    delete conview;
    conview = NULL;
    zCConDat* command = list.root;
    while (command)
    {
        zCConDat* next = command->next;
        delete command;
        command = next;
    }
    list.root = NULL;
    list.last = NULL;
    list.count = 0;
    active_consoles.Remove(this);
}

void zCConsole::SetAutoCompletion(int enabled)
{
    autocomplete = enabled;
}

int zCConsole::HasAutoCompletion()
{
    return autocomplete;
}

void zCConsole::SetWorld(zCWorld* newWorld)
{
    world = newWorld;
}

zCWorld* zCConsole::GetWorld()
{
    return world;
}

void zCConsole::SetParser(zCParser* parser)
{
    cparser = parser;
}

zCParser* zCConsole::GetParser()
{
    return cparser;
}

void zCConsole::AddChar(int character, int printable)
{
    if (printable)
    {
        if (character != 0x20 || instr.Length())
        {
            instr.Append(zSTRING((char)character));
            if (autocomplete)
                AutoCompletion(instr);
            lastCommandPos = lastcommand.numInArray;
        }
    }
    else if (character == 1)
    {
        Hide();
        changedfunc = NULL;
    }
    else if (character == 8)
    {
        if (instr.Length() > 0)
            instr.Delete(instr.Length() - 1, 1);
        lastCommandPos = lastcommand.numInArray;
    }
    else if (character == 13)
    {
        instr.Upper();
        if (Evaluate(instr))
        {
            if (var)
                Update();
            if (instr.Length())
                lastcommand.InsertEnd(instr);
            instr.Clear();
        }
        lastCommandPos = lastcommand.numInArray;
    }
    else if (character == 0xC8)
    {
        AddSkip(-1);
    }
    else if (character == 0xD0)
    {
        AddSkip(1);
    }
    else if (character == 0xD1)
    {
        if (lastcommand.numInArray > 0 && lastCommandPos > 0)
            instr = lastcommand[--lastCommandPos].Lower();
    }
    else if (character == 0xD2)
    {
        instr.Clear();
        lastCommandPos = lastcommand.numInArray;
    }
    ShowInput();
}

void zCConsole::InsertVar(const zSTRING& name, const zSTRING& hint, int type, void* address,
    int elements)
{
    var = 1;
    list.InsertLast(new zCConDat(name, hint, type, address, elements));
}

void zCConsole::Register(const zSTRING& command, const zSTRING& hint, int type)
{
    list.InsertLast(new zCConDat(command, hint, type));
}

void zCConsole::Register(const zSTRING& command, const zSTRING& hint)
{
    list.InsertLast(new zCConDat(command, hint, 0));
}

int zCConsole::AutoCompletion(zSTRING& input)
{
    zSTRING prefix = input.PickWord(1, " ", " ").Upper();
    if (prefix.IsEmpty())
        return 0;
    if (input.PickWord(2, " ", " ").Length() > 0)
    {
        for (zCConDat* command = list.root; command; command = command->next)
        {
            if (command->name == prefix && cparser)
                return cparser->AutoCompletion(input);
        }
        return 0;
    }
    zCConDat* match = NULL;
    zSTRING common;
    int matches = 0;
    for (zCConDat* command = list.root; command; command = command->next)
    {
        if (command->name.Length() < prefix.Length() || command->name.Search(prefix) != 0)
            continue;
        match = command;
        ++matches;
        if (common.IsEmpty())
            common = command->name;
        else
        {
            while (!common.IsEmpty() && command->name.Search(common) != 0)
                common = common.Left(common.Length() - 1);
        }
    }
    if (matches == 1)
    {
        input = match->name.Lower() + " ";
        return 1;
    }
    if (matches > 1 && common.Length() > prefix.Length())
    {
        input = common.Lower();
        return 1;
    }
    if (!matches && cparser)
        return cparser->AutoCompletion(prefix);
    return 0;
}

void zCConsole::SetFocus()
{
    if (cur_console)
        cur_console->SetEnableHandleEvent(0);
    cur_console = this;
    SetEnableHandleEvent(1);
    SetHandleEventTop();
}

void zCConsole::RemoveFocus()
{
    if (cur_console)
        cur_console->SetEnableHandleEvent(0);
    cur_console = NULL;
    for (zCList<zCConsole>* node = active_consoles.next; node; node = node->next)
    {
        zCConsole* console = node->data;
        if (console != this && console->conview && console->conview->ondesk)
        {
            cur_console = console;
            break;
        }
    }
    if (!cur_console && zcon.conview && zcon.conview->ondesk)
        cur_console = &zcon;
    if (cur_console)
    {
        cur_console->SetEnableHandleEvent(1);
        cur_console->SetHandleEventTop();
        cur_console->Update();
    }
}

void zCConsole::Show()
{
    skip = 0;
    showmax = list.count > 24 ? 24 : list.count;
    if (!conview)
    {
        int maxLines = 8192 / screen->FontY();
        if (dynsize)
        {
            if (showmax + 3 > maxLines)
                showmax = maxLines - 3;
            ly = screen->FontY() * (showmax + 3);
        }
        else
        {
            if (lx < 0)
                lx = 4000;
            if (ly < 0)
                ly = 2000;
        }
        conview = new zCView(px, py, px + lx, py + ly, 2);
        screen->InsertItem(conview);
        conview->InsertBack("CONSOLE.TGA");
        conview->ClrFlags(0x200);
        conview->SetTransparency(150);
        conview->SetAlphaBlendFunc(2);
    }
    else if (!conview->ondesk)
        screen->InsertItem(conview);
    SetFocus();
    conview->DefPrintwin(0, 0, 0x1FFF, 0x1FFF - conview->FontY());
    Update();
}

void zCConsole::Hide()
{
    if (conview && conview->ondesk)
    {
        screen->RemoveItem(conview);
        RemoveFocus();
    }
}

void zCConsole::Toggle()
{
    if (IsVisible())
        Hide();
    else
        Show();
}

int zCConsole::IsVisible()
{
    return conview && conview->ondesk;
}

void zCConsole::AddSkip(int amount)
{
    skip += amount;
    if (skip < 0)
        skip = 0;
    if (skip + showmax > list.count)
        skip = list.count - showmax;
}

void zCConsole::Update()
{
    if (list.count <= 0 || !conview)
        return;
    int y = conview->FontY();
    if (!id.IsEmpty())
    {
        conview->Print(0, 0, id);
        y += conview->FontY();
    }
    zCConDat* command = list.root;
    for (int i = 0; command && i < skip; ++i)
        command = command->next;
    int shown = 0;
    for (; command && shown < showmax; command = command->next)
    {
        if (!command->adr)
            continue;
        zSTRING value;
        switch (command->type)
        {
        case zCON_TYPE_INT:
            value = zSTRING(*(int*)command->adr);
            break;
        case zCON_TYPE_FLOAT:
            value.Sprintf("%.9f", *(float*)command->adr);
            break;
        case zCON_TYPE_STRING:
            value = *(zSTRING*)command->adr;
            break;
        case zCON_TYPE_STRINGP:
            if (*(zSTRING**)command->adr)
                value = **(zSTRING**)command->adr;
            break;
        }
        conview->Print(0, y, command->name);
        conview->Print(4000, y, value);
        zSTRING hint = command->hint;
        if (command->ele > 1)
            hint.Append(":  (" + zSTRING(command->ele) + ")");
        conview->Print(6000, y, hint);
        y += conview->FontY();
        ++shown;
    }
    if (dynsize)
        conview->Print(0, 8191 - conview->FontY() * 2, savemsg);
    ShowInput();
}

void zCConsole::ShowInput()
{
    if (!conview)
        return;
    zSTRING input = ": " + instr;
    if (cur_console == this)
        input += "_";
    conview->Print(0, 8191 - conview->FontY(), input);
}

zCConDat* zCConsole::Get(const zSTRING& name)
{
    zSTRING commandName = name.PickWord(1, " ", " ").Upper();
    for (zCConDat* command = list.root; command; command = command->next)
    {
        if (command->name == commandName)
            return command;
    }
    return NULL;
}

zCConDat* zCConsole::GetBestMatch(const zSTRING& name)
{
    zSTRING upperName = name.Upper();
    for (zCConDat* command = list.root; command; command = command->next)
    {
        if (command->name.PickWord(1, " ", " ").Search(upperName) >= 0)
            return command;
    }
    return NULL;
}

void zCConsole::Eval(const zSTRING& input)
{
    zCConDat* command = Get(input);
    if (!command || !command->adr)
        return;
    zSTRING operation = input.PickWord(2, " ", " ");
    if (operation != "=")
        return;
    zSTRING value = input.PickWord(3, " ", " ");
    if (command->type == zCON_TYPE_STRING || command->type == zCON_TYPE_STRINGP)
    {
        const char* remainder = input.PickWordPos(3, "=", " ");
        if (remainder)
        {
            while (*remainder == ' ')
                ++remainder;
            value = remainder;
        }
    }
    switch (command->type)
    {
    case zCON_TYPE_INT:
        *(int*)command->adr = value.ToInt();
        break;
    case zCON_TYPE_FLOAT:
        *(float*)command->adr = value.ToFloat();
        break;
    case zCON_TYPE_STRING:
        *(zSTRING*)command->adr = value;
        break;
    case zCON_TYPE_STRINGP:
        **(zSTRING**)command->adr = value;
        break;
    }
    if (changedfunc)
        changedfunc(command->name);
}

int zCConsole::Evaluate(const zSTRING& input)
{
    savemsg.Clear();
    if (input.IsEmpty())
        return 0;
    zCConDat* command = Get(input);
    if (var && command && command->ele > 1 && input.Search("=") < 0)
    {
        int type = command->type;
        int elements = command->ele;
        void* address = command->adr;
        zSTRING hint = command->hint;
        zCConDat* entry = list.root;
        while (entry)
        {
            zCConDat* next = entry->next;
            delete entry;
            entry = next;
        }
        list.root = NULL;
        list.last = NULL;
        list.count = 0;
        dynsize = 1;
        lx = 4000;
        ly = 0;
        int elementSize = type == zCON_TYPE_STRING ? 20 : 4;
        for (int i = 0; i < elements; ++i)
        {
            void* elementAddress = (char*)address + i * elementSize;
            list.InsertLast(new zCConDat(zSTRING(i), hint, type, elementAddress, 1));
        }
        var = 1;
        if (IsVisible())
            Hide();
        Show();
        return 1;
    }
    if (var && command && input.Search("=") > 0)
    {
        Eval(input);
        return 1;
    }
    if (input.Search("SAVE INFOFILE") >= 0)
    {
        SaveInfoFile("Readme_Console.txt");
        savemsg = "File saved (DIR_CONFIG : Readme_Console.txt)";
    }
    else if (input == "HELP" || input.Search("HELP ") == 0)
        ShowHelp();
    else
    {
        for (int i = 0; i < evalcount && evalfunc[i]; ++i)
        {
            if (evalfunc[i](input, savemsg))
                break;
        }
    }
    if (!savemsg.IsEmpty() && !dynsize && conview)
        conview->Print(0, 0, savemsg + "\n");
    return 1;
}

void zCConsole::SetPos(int x, int y)
{
    px = x;
    py = y;
}

void zCConsole::SetDim(int width, int height)
{
    lx = width;
    ly = height;
    dynsize = 0;
}

void zCConsole::SetFlags(int flags)
{
    conview->Setflags(flags);
}

void zCConsole::AddEvalFunc(int(*evalFunc)(const zSTRING&, zSTRING&))
{
    if (evalcount + 1 < zCON_MAX_EVAL)
        evalfunc[evalcount++] = evalFunc;
}

void zCConsole::SetChangedFunc(void(*changedFunc)(const zSTRING&))
{
    changedfunc = changedFunc;
}

void zCConsole::ShowHelp()
{
    zerr.Message("U:CON: console commands:");
    for (zCConDat* command = list.root; command; command = command->next)
        zerr.Message("U:CON: " + command->name + "   : " + command->hint);
}

void zCConsole::SaveInfoFile(const zSTRING& fileName)
{
    zoptions->ChangeDir(0);
    zFILE* file = zfactory->CreateZFile(fileName);
    file->Create();
    for (zCConDat* command = list.root; command; command = command->next)
    {
        zSTRING line = command->name;
        int tabs = 3 - command->name.Length() / 8;
        for (int i = 0; i < tabs; ++i)
            line.Append("\t");
        line.Append(command->name + ": " + command->hint + "\n");
        file->Write(line);
    }
    file->Close();
    delete file;
}

int zCConsole::EditFunc(const zSTRING& input, zSTRING& message)
{
    if (!cur_console)
        return 0;
    zSTRING command = input.PickWord(1, " ", " ").Upper();
    if (command == "SAVE")
    {
        if (cur_console->cparser->SaveInstance(cur_console->edit_index, cur_console->edit_adr))
            message = "Saved,";
        else
            message = "Save failed.";
    }
    return 1;
}

int zCConsole::EditInstance(const zSTRING& name, void* instance)
{
    if (!cparser)
        return 0;
    return EditInstance(cparser->GetIndex(name), instance);
}

int zCConsole::EditInstance(int index, void* instance)
{
    if (!instance || !cparser)
        return 0;
    zCPar_Symbol* symbol = cparser->GetSymbol(index);
    if (!symbol || (symbol->ele & 0xF000) != 0x7000)
        return 0;
    zCPar_Symbol* baseClass = cparser->GetSymbol(cparser->GetBaseClass(index));
    dynsize = 1;
    lx = 4000;
    ly = 0;
    id = symbol->name + " (" + baseClass->name + ")";
    edit_index = index;
    edit_adr = instance;
    int members = symbol->ele & 0xFFF;
    for (int i = 0; i < members; ++i)
    {
        zCPar_Symbol* member = cparser->GetSymbol(index + i + 1);
        int parserType = (member->ele >> 12) & 0xF;
        if (parserType < 1 || parserType > 3)
            continue;
        zSTRING name = member->name;
        int separator = name.Search(".");
        if (separator > 0)
            name = name.Copied(separator + 1, name.Length() - separator - 1);
        int elements = member->ele & 0xFFF;
        void* address = (char*)instance + member->GetOffset();
        var = 1;
        list.InsertLast(new zCConDat(name, name, parserType - 1, address, elements));
    }
    AddEvalFunc(EditFunc);
    Show();
    return 1;
}

zCConDat::~zCConDat()
{
}
