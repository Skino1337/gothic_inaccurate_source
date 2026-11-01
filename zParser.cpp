#include "include/oCollisionObjectMiscChar.h"
#include "include/zParser.h"
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <io.h>

static unsigned char tree_version = 0;
static int parserState = -1;
static zSTRING parserPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING parserPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING parserPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING parserPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING parserPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING parserPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING parserPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING parserPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING parserPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING parserPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";

void zCParser::SetVersion(unsigned char version)
{
    tree_version = version;
}

unsigned char zCParser::GetVersion()
{
    return tree_version;
}

zCPar_File::zCPar_File()
    : fileid(0), begin_adr(NULL), size(0), labels(0), treeload(0), tree(NULL)
{
}

zCPar_File::~zCPar_File()
{
    delete[] begin_adr;
    delete tree;
}

static unsigned long long ParDateNumber(zDATE& date)
{
    unsigned long long result = (unsigned long long(date.year) << 8) + date.month;
    result = (result << 8) + date.day;
    result = (result << 8) + date.hour;
    result = (result << 8) + date.minute;
    return (result << 8) + date.second;
}

int zCPar_File::IsNewerDate(zDATE& first, zDATE& second)
{
    return ParDateNumber(first) > ParDateNumber(second);
}

int zCPar_File::IsExactDate(zDATE& first, zDATE& second)
{
    return ParDateNumber(first) == ParDateNumber(second);
}

void zCPar_File::SetFileNumber(int number)
{
    fileid = number;
}

int zCPar_File::GetFileNumber()
{
    return fileid;
}

zSTRING zCPar_File::GetName()
{
    return name;
}

void zCPar_File::EnableTreeLoad(int enabled)
{
    treeload = enabled;
}

int zCPar_File::Load(zSTRING& fileName, zCPar_SymbolTable& symbols, zDATE& date)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    if (!file->Exists())
    {
        delete file;
        return -1;
    }
    DeleteTree();
    name = fileName;
    if (treeload)
    {
        zFILE_STATS stats;
        file->GetStats(stats);
        if (LoadTree(stats, symbols))
        {
            delete file;
            return 1;
        }
    }
    file->Open(0);
    size = file->Size() + 1;
    begin_adr = new char[size];
    if (!begin_adr)
    {
        file->Close();
        delete file;
        return -2;
    }
    file->Reset();
    file->Read(begin_adr, size - 1);
    file->Close();
    delete file;
    begin_adr[size - 1] = 0;
    return 0;
}

char* zCPar_File::GetStartAddress()
{
    return begin_adr;
}

char* zCPar_File::GetEndAddress()
{
    return begin_adr + size - 1;
}

void zCPar_File::Dispose()
{
    delete[] begin_adr;
    begin_adr = NULL;
}

void zCPar_File::SetTree(zCPar_TreeNode* newTree)
{
    delete tree;
    tree = newTree;
}

zCPar_TreeNode* zCPar_File::GetTree()
{
    return tree;
}

void zCPar_File::SetLabelCount(int count)
{
    labels = count;
}

int zCPar_File::GetLabelCount()
{
    return labels;
}

void zCPar_File::SaveTree(int fileNumber, zCPar_Symbol* symbol, zDATE& date)
{
    zoptions->ChangeDir(7);
    zSTRING fileName = name.Copied(0, name.Search(".")) + ".tre";
    zFILE* file = zfactory->CreateZFile(fileName);
    file->SetDir("_work");
    file->Create();
    file->Write(&date, sizeof(date));
    file->Write(&tree_version, sizeof(tree_version));
    file->Write(&labels, sizeof(labels));
    file->Write(&fileNumber, sizeof(fileNumber));
    zCPar_Symbol* current = symbol;
    for (int i = 0; i < fileNumber && current; ++i)
    {
        current->Save(file);
        zCPar_Symbol* parent = current->GetParent();
        file->Write(&parent, sizeof(parent));
        if (parent)
            file->Write(parent->name + "\n");
        current = current->GetNext();
    }
    SaveTreeNode(file, tree);
    file->Close();
    delete file;
}

void zCPar_File::SaveTreeNode(zFILE* file, zCPar_TreeNode* node)
{
    if (!node)
        return;
    int hasName = !node->name.IsEmpty();
    file->Write(&hasName, sizeof(hasName));
    if (hasName)
        file->Write(node->name + "\n");
    file->Write(&node->token, sizeof(node->token));
    file->Write(&node->value, sizeof(node->value));
    file->Write(&node->info, sizeof(node->info));
    int hasLeft = node->left != NULL;
    int hasRight = node->right != NULL;
    int hasNext = node->next != NULL;
    file->Write(&hasLeft, sizeof(hasLeft));
    file->Write(&hasRight, sizeof(hasRight));
    file->Write(&hasNext, sizeof(hasNext));
    if (hasLeft)
        SaveTreeNode(file, node->left);
    if (hasRight)
        SaveTreeNode(file, node->right);
    if (hasNext)
        SaveTreeNode(file, node->next);
}

zCPar_TreeNode* zCPar_File::LoadTreeNode(zFILE* file)
{
    zCPar_TreeNode* node = new zCPar_TreeNode;
    int hasName = 0;
    file->Read(&hasName, sizeof(hasName));
    if (hasName)
        file->Read(node->name);
    file->Read(&node->token, sizeof(node->token));
    file->Read(&node->value, sizeof(node->value));
    file->Read(&node->info, sizeof(node->info));
    int hasLeft = 0;
    int hasRight = 0;
    int hasNext = 0;
    file->Read(&hasLeft, sizeof(hasLeft));
    file->Read(&hasRight, sizeof(hasRight));
    file->Read(&hasNext, sizeof(hasNext));
    if (hasLeft)
        node->left = LoadTreeNode(file);
    if (hasRight)
        node->right = LoadTreeNode(file);
    if (hasNext)
        node->next = LoadTreeNode(file);
    return node;
}

int zCPar_File::LoadTree(zFILE_STATS& stats, zCPar_SymbolTable& symbols)
{
    zSTRING fileName = name.Copied(0, name.Search(".")) + ".tre";
    zFILE* file = zfactory->CreateZFile(fileName);
    file->SetDir("_work");
    if (!file->Exists())
    {
        delete file;
        return 0;
    }
    file->Open(0);
    zDATE storedDate;
    file->Read(&storedDate, sizeof(storedDate));
    if (!IsExactDate(storedDate, stats.modifyDate))
    {
        file->Close();
        delete file;
        return 0;
    }
    DeleteTree();
    unsigned char version = 0;
    file->Read(&version, sizeof(version));
    if (version != tree_version)
    {
        file->Close();
        delete file;
        return 0;
    }
    file->Read(&labels, sizeof(labels));
    int count = 0;
    file->Read(&count, sizeof(count));
    for (int i = 0; i < count; ++i)
    {
        zCPar_Symbol* symbol = new zCPar_Symbol;
        symbol->Load(file);
        int hasParent = 0;
        file->Read(&hasParent, sizeof(hasParent));
        if (hasParent)
        {
            zSTRING parentName;
            file->Read(parentName);
            symbol->SetParent(symbols.GetSymbol(parentName));
        }
        symbols.Insert(symbol);
        symbol->SetFileNr(fileid);
    }
    tree = LoadTreeNode(file);
    file->Close();
    delete file;
    return 1;
}

void zCPar_File::DeleteTree()
{
    delete tree;
    tree = NULL;
}

static int parserState = -1;
zCParser parser(1000);
static int parserErrorState = -1;
zSTRING Semikolon(";");
zSTRING ParErr_SyntaxError("Syntax error : ");
zSTRING ParErr_Redefined("Redefined identifier : ");
zSTRING ParErr_UnknownVar("Unknown identifier : ");
zSTRING ParErr_OutOfArray("Array value out of range : ");
zSTRING ParErr_UndefinedFunction("Undefined function : ");
zSTRING ParErr_NoValidClass("Not a valid class or prototype name :");
zSTRING ParErr_Expected("Expected ");
zSTRING Operators("+-*/%|&<>=");
zCParser* zCParser::cur_parser = &parser;

zCParser* zCParser::GetParser()
{
    return cur_parser;
}

zCParser::zCParser(int symbolTableSize)
    : msgfunc(NULL), self(NULL), compiled(0), treesave(0), datsave(0), parse_changed(0),
      treeload(0), mergemode(0), linkingnr(0), linec(0), line_start(0), ext_parse(0),
      pc_start(NULL), pc(NULL), oldpc(NULL), pc_stop(NULL), oldpc_stop(NULL), params(0),
      in_funcnr(0), in_classnr(0), stringcount(0), in_func(NULL), in_class(NULL), error(0),
      stop_on_error(0), errorline(0), prevword_nr(0), instance(0), instance_help(0),
      progressBar(NULL), tree(NULL), treenode(NULL), win_code(NULL), debugmode(0), curfuncnr(0),
      labelcount(0), labelpos(NULL), add_funclist(NULL), add_created(0)
{
    for (int i = 0; i < 16; ++i)
    {
        prevword_index[i] = NULL;
        prevline_index[i] = 0;
    }
    symtab.SetSize(symbolTableSize);
    Reset();
}

zCParser::~zCParser()
{
    for (int i = 0; i < file.numInArray; ++i)
        delete file[i];
    symtab.Clear();
    stringtab.Clear();
    stack.Clear();
    datastack.Clear();
    add_funclist = NULL;
    self = NULL;
    pc_start = NULL;
    pc = NULL;
    oldpc = NULL;
    pc_stop = NULL;
    oldpc_stop = NULL;
    in_func = NULL;
    in_class = NULL;
    tree = NULL;
    treenode = NULL;
    win_code = NULL;
    labelpos = NULL;
    msgfunc = NULL;
}

void zCParser::SetMessageFunc(void(*messageFunc)(zSTRING))
{
    msgfunc = messageFunc;
}

void zCParser::SetStopOnError(int enabled)
{
    stop_on_error = enabled;
}

int zCParser::enableParsing = 1;

void sysExit();

void zCParser::Error(zSTRING& text, int line)
{
    error = 1;
    zSTRING message = "U:PAR: ";
    if (linkingnr >= 0)
    {
        message += GetFileName() + ":" + zSTRING(line);
        errorline = line;
    }
    else
    {
        message += " ( line " + zSTRING(linec) + " )";
        errorline = linec;
    }
    message += text;
    if (msgfunc)
        msgfunc(message);
    if (stop_on_error)
    {
        zerr.Message(message);
        sysExit();
    }
    else
        zerr.Message(message);
}

void zCParser::Message(zSTRING& text)
{
    if (msgfunc)
        msgfunc(text);
    zerr.Message("U:PAR: " + text);
}

int zCParser::Error()
{
    return error;
}

int zCParser::SaveDat(zSTRING& fileName)
{
    if (!datsave)
        return 0;
    zoptions->ChangeDir(17);
    zFILE* file = zfactory->CreateZFile(fileName);
    file->Create();
    file->Write(&tree_version, sizeof(tree_version));
    symtab.Save(file);
    stack.Save(file);
    file->Close();
    delete file;
    return 0;
}

int zCParser::LoadDat(zSTRING& fileName)
{
    zoptions->ChangeDir(17);
    zFILE* file = zfactory->CreateZFile(fileName);
    file->Open(0);
    unsigned char version = 0;
    file->Read(&version, sizeof(version));
    if (version != tree_version)
    {
        file->Close();
        delete file;
        zSTRING text = "Dat-File is not compatible. Please use the \"SRC\"-File.";
        Error(text, 0);
        return -1;
    }
    symtab.Load(file);
    stack.Load(file);
    file->Close();
    delete file;
    instance_help = symtab.GetIndex("INSTANCE_HELP");
    return 0;
}

int zCParser::Parse(const zSTRING& fileName)
{
    mainfile = fileName;
    mainfile.Upper();
    compiled = 0;
    if (!enableParsing)
    {
        if (mainfile.Search(".DAT") > 0)
            return LoadDat(mainfile);
        return -1;
    }
    if (mainfile.Search(".SRC") > 0)
        return ParseSource(mainfile);
    if (mainfile.Search(".DAT") > 0)
        return LoadDat(mainfile);
    return ParseFile(mainfile);
}

int zCParser::ParseSource(zSTRING& source)
{
    zoptions->ChangeDir(7);
    zFILE* file = zfactory->CreateZFile(source);
    if (!file->Exists())
    {
        delete file;
        zSTRING text = "Source-File " + source + " not found.";
        Error(text, 0);
        return -1;
    }
    file->Open(0);
    zSTRING line;
    while (file->Read(line))
    {
        line.Upper();
        if (line.Search("*") >= 0 || line.Search("?") >= 0)
        {
            zFILE::DirectFileAccess(true);
            _finddata_t data;
            long handle = _findfirst(line.ToChar(), &data);
            zFILE::DirectFileAccess(false);
            if (handle < 0)
            {
                Error("Found no matching File : " + line, 0);
                continue;
            }
            do
            {
                zSTRING match = data.name;
                match.Upper();
                if (match.Search(".SRC") >= 0)
                    ParseSource(match);
                else if (match.Search(".D") >= 0)
                    ParseFile(match);
            } while (_findnext(handle, &data) == 0);
            _findclose(handle);
        }
        else if (line.Search(".SRC") >= 0)
            ParseSource(line);
        else if (line.Search(".D") >= 0)
            ParseFile(line);
    }
    file->Close();
    delete file;
    return 0;
}

int zCParser::ParseFile(zSTRING& fileName)
{
    for (int i = 0; i < file.numInArray; ++i)
    {
        if (file[i]->GetName() == fileName)
            return 0;
    }
    fname = fileName;
    labelcount = 0;
    tree = new zCPar_TreeNode;
    treenode = tree;
    tree->token = 0x5A;
    zCPar_File* current = new zCPar_File;
    current->SetFileNumber(file.numInArray);
    current->EnableTreeLoad(treeload);
    file.InsertEnd(current);
    zDATE date = {};
    int result = current->Load(fileName, symtab, date);
    if (result == -1)
    {
        file.Remove(current);
        delete current;
        zSTRING text = "File " + fileName + " not found.";
        Error(text, 0);
        return -1;
    }
    if (!result)
    {
        parse_changed = 1;
        pc_start = current->GetStartAddress();
        pc = pc_start;
        pc_stop = current->GetEndAddress();
        while (pc < pc_stop)
        {
            ReadWord();
            if (aword == "CONST" || aword == "VAR")
                DeclareVar();
            else if (aword == "FUNC")
                DeclareFunc();
            else if (aword == "CLASS")
                DeclareClass();
            else if (aword == "INSTANCE")
                DeclareInstance();
            else if (aword == "PROTOTYPE")
                DeclarePrototype();
            else if (aword.IsEmpty())
                break;
            else
                Error(ParErr_SyntaxError + aword, 0);
            Match(";");
        }
        current->SetLabelCount(labelcount);
        current->SetTree(tree);
        tree = NULL;
        treenode = NULL;
        current->Dispose();
        if (treesave)
            current->SaveTree(symtab.GetNumInList(), symtab.GetFirstSymbol(), date);
    }
    return 0;
}

void zCParser::ParseBlock()
{
    Match("{");
    while (pc < pc_stop)
    {
        ReadWord();
        if (aword == "}")
            break;
        if (aword == "RETURN")
            DeclareReturn();
        else if (aword == "IF")
            DeclareIf();
        else if (aword == "CONST" || aword == "VAR")
            DeclareVar();
        else
            DeclareAssign(aword);
        Match(";");
    }
    Match("}");
}

void zCParser::CreatePCode()
{
    error = 0;
    if (!compiled)
    {
        zerr.Message("U:PAR: Linking ...");
        for (int i = 0; i < file.numInArray; ++i)
        {
            if (!file[i]->tree)
                continue;
            labelpos = new int[file[i]->labels];
            for (zCPar_TreeNode* node = file[i]->tree; node; node = PushOnStack(node))
                ;
            SolveLabels(file[i]->tree);
            delete file[i]->tree;
            file[i]->tree = NULL;
            delete[] labelpos;
            labelpos = NULL;
        }
        zerr.Message("U:PAR: Symbols   : " + zSTRING(symtab.GetNumInList()));
        zerr.Message("U:PAR: Code Size : " + zSTRING(stack.GetDynSize()) + " bytes.");
    }
    if (!compiled && datsave)
        SaveDat(mainfile.Copied(0, mainfile.Search(".")) + ".DAT");
    self = GetSymbol("SELF");
    compiled = 1;
}

void zCParser::ShowSymbols()
{
    symtab.Show();
}

void zCParser::PushTree(zCPar_TreeNode* node)
{
    for (zCPar_TreeNode* current = node; current; current = PushOnStack(current))
        ;
}

zCPar_TreeNode* zCParser::PushOnStack(zCPar_TreeNode* node)
{
    for (zCPar_TreeNode* right = node->right; right; right = PushOnStack(right))
        ;
    for (zCPar_TreeNode* left = node->left; left; left = PushOnStack(left))
        ;
    int token = node->token;
    if (token <= 0x21 || token == 0x46 || token == 0x47 || token == 0x49 ||
        token == 0x4A || token == 0x5D || token == 0x3C)
        stack.PushByte(token);
    else if (token == 0x33)
    {
        stack.PushByte(0x40);
        stack.PushInt(node->value);
    }
    else if (token == 0x34)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(FindIndex(node->name));
        int type = symbol ? (symbol->ele >> 12) & 0xF : 0;
        if ((node->info == 2 || node->info == 5) && (type == 6 || type == 7))
            stack.PushByte(0x40);
        else
            stack.PushByte(node->info ? 0x41 : 0xF5);
        stack.PushInt(node->value);
    }
    else if (token == 0x44 || token == 0x43)
    {
        stack.PushByte(token);
        if (token == 0x43 && node->name == "NULL")
            stack.PushInt(node->value);
        else
            stack.PushInt(symtab.GetIndex(node->name));
    }
    else if (token == 0x62)
    {
        stack.PushByte(0x41);
        stack.PushInt(symtab.GetIndex(node->name));
    }
    else if (token == 0x4B || token == 0x4C)
    {
        stack.PushByte(token);
        stack.PushInt(node->info);
    }
    else if (token == 0x5B)
        labelpos[node->value] = stack.GetDynSize();
    else if (token == 0x5C || token == 0x60)
    {
        in_funcnr = symtab.GetIndex(node->name);
        in_func = symtab.GetSymbol(in_funcnr);
        if (in_func)
            in_func->SetStackPos(stack.GetDynSize());
        if (token == 0x60)
            in_class = in_func ? in_func->GetParent() : NULL;
    }
    else if (token == 0x61)
    {
        in_funcnr = 0;
        in_func = NULL;
        in_classnr = 0;
        in_class = NULL;
    }
    else
        stack.PushByte(token);
    return node->next;
}

void zCParser::SolveLabels(zCPar_TreeNode* node)
{
    for (zCPar_TreeNode* current = node; current; current = current->next)
    {
        if (current->token == 0x4B || current->token == 0x4C)
            stack.Set(labelpos[current->value], current->info);
        if (current->right)
            SolveLabels(current->right);
        if (current->left)
            SolveLabels(current->left);
    }
}

int zCParser::MergeFile(zSTRING& fileName)
{
    mergemode = 1;
    error = 0;
    treesave = 0;
    Parse(fileName);
    if (!error)
    {
        CreatePCode();
        for (int i = 0; i < symtab.GetNumInList(); ++i)
        {
            zCPar_Symbol* symbol = symtab.GetSymbol(i);
            if (!symbol || (symbol->ele & 0xF000) != 0x7000 || !symbol->HasFlag(0x10)
                || !symbol->GetOffset())
                continue;
            int position;
            symbol->GetStackPos(position, 0);
            zCPar_Symbol::SetUseInstance(symbol);
            DoStack(position + 5);
            zerr.Message("U:PAR: Instance " + symbol->name + " merged.");
        }
    }
    treesave = 0;
    mergemode = 0;
    return error;
}

int zCParser::PopDataValue()
{
    int marker = datastack.Pop();
    if (marker == 0x40)
        return datastack.Pop();
    if (marker == 0x41)
        return *(int*)datastack.Pop();
    return 0;
}

float zCParser::PopFloatValue()
{
    int marker = datastack.Pop();
    if (marker == 0x40)
        return datastack.PopFloat();
    if (marker == 0x41)
        return *(float*)datastack.Pop();
    return 0.0f;
}

int* zCParser::PopVarAddress()
{
    if (datastack.Pop() == 0x41)
        return (int*)datastack.Pop();
    datastack.Pop();
    return NULL;
}

zSTRING* zCParser::PopString()
{
    datastack.Pop();
    return (zSTRING*)datastack.Pop();
}

void zCParser::DoStack(int position)
{
    stack.SetPopPos(position);
    while (true)
    {
        int command = stack.PopByte();
        if (command == 0x3C)
            return;
        if (command != 9 && command >= 0 && command <= 0x12)
        {
            int right = PopDataValue();
            int left = PopDataValue();
            int result = 0;
            switch (command)
            {
            case 0: result = left + right; break;
            case 1: result = left - right; break;
            case 2: result = left * right; break;
            case 3: result = left / right; break;
            case 4: result = left % right; break;
            case 5: result = left | right; break;
            case 6: result = left & right; break;
            case 7: result = left < right; break;
            case 8: result = left > right; break;
            case 0xB: result = left || right; break;
            case 0xC: result = left && right; break;
            case 0xD: result = left << right; break;
            case 0xE: result = left >> right; break;
            case 0xF: result = left <= right; break;
            case 0x10: result = left == right; break;
            case 0x11: result = left != right; break;
            case 0x12: result = left >= right; break;
            }
            datastack.Push(result);
            continue;
        }
        switch (command)
        {
        case 0x13:
            *PopVarAddress() += PopDataValue();
            break;
        case 0x14:
            *PopVarAddress() -= PopDataValue();
            break;
        case 0x15:
            *PopVarAddress() *= PopDataValue();
            break;
        case 0x16:
            *PopVarAddress() /= PopDataValue();
            break;
        case 0x1E:
            datastack.Push(PopDataValue());
            break;
        case 0x1F:
            datastack.Push(-PopDataValue());
            break;
        case 0x20:
            datastack.Push(!PopDataValue());
            break;
        case 0x21:
            datastack.Push(~PopDataValue());
            break;
        case 0x3D:
        {
            int popPos = stack.GetPopPos();
            DoStack(stack.PopInt());
            stack.SetPopPos(popPos);
            break;
        }
        case 0x40:
            datastack.Push(stack.PopInt());
            break;
        case 0x41:
            datastack.Push((int)symtab.GetSymbol(stack.PopInt()));
            break;
        case 0x43:
            datastack.Push(stack.PopInt());
            break;
        case 0x46:
            *PopVarAddress() = (int)PopString();
            break;
        case 0x47:
            *(float*)PopVarAddress() = PopFloatValue();
            break;
        case 0x48:
            *PopVarAddress() = PopDataValue();
            break;
        case 0x49:
            *(float*)PopVarAddress() = PopFloatValue();
            break;
        case 0x9:
            *PopVarAddress() = PopDataValue();
            break;
        default:
        {
            zSTRING text = "Error : unknown stack command.";
            Error(text, 0);
            return;
        }
        }
    }
}

void* zCParser::CallFunc(const zSTRING& name)
{
    return CallFunc(symtab.GetIndex(name));
}

void* zCParser::CallFunc(int function, ...)
{
    static int result = 0;
    result = 0;
    zCPar_Symbol* symbol = symtab.GetSymbol(function);
    if (!symbol || (symbol->ele & 0xF000) != 0x5000)
    {
        zerr.Message("Warning : Engine calls an undefined function. (index : " +
            zSTRING(function) + ")");
        return &result;
    }
    datastack.Clear();
    va_list args;
    va_start(args, function);
    int parameters = symbol->ele & 0xFFF;
    for (int i = 0; i < parameters; ++i)
    {
        zCPar_Symbol* parameter = symtab.GetSymbol(function + i + 1);
        int type = (parameter->ele >> 12) & 0xF;
        if (type == 1)
        {
            int value = va_arg(args, int);
            parameter->SetValue(value, 0);
            datastack.Push(value);
            datastack.Push(0x40);
        }
        else if (type == 2)
        {
            float value = float(va_arg(args, double));
            parameter->SetValue(value, 0);
            int bits;
            memcpy(&bits, &value, sizeof(bits));
            datastack.Push(bits);
            datastack.Push(0x40);
        }
        else if (type == 3)
        {
            zSTRING value = va_arg(args, zSTRING*);
            parameter->SetValue(value, 0);
            datastack.Push((int)parameter->GetDataAdr(0));
            datastack.Push(0x41);
        }
        else
        {
            zSTRING text = "Function type not supported.";
            Error(text, 0);
            break;
        }
    }
    va_end(args);
    curfuncnr = function;
    DoStack(symbol->GetStackPos());
    if (symbol->HasFlag(1))
        result = PopDataValue();
    curfuncnr = -1;
    return &result;
}

int zCParser::GetCurrentFunc()
{
    return curfuncnr;
}

int zCParser::CreateInstance(const zSTRING& name, void* data)
{
    zSTRING upperName = name.Upper();
    return CreateInstance(symtab.GetIndex(upperName), data);
}

int zCParser::CreateInstance(int instance, void* data)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (!symbol || (symbol->ele & 0xF000) != 0x7000)
        return 0;
    symbol->SetOffset((int)data);
    zCPar_Symbol::SetUseInstance(symbol);
    DoStack(symbol->GetStackPos());
    return 1;
}

int zCParser::CreatePrototype(const zSTRING& name, void* data)
{
    return CreatePrototype(symtab.GetIndex(name), data);
}

int zCParser::CreatePrototype(int instance, void* data)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (!symbol || (symbol->ele & 0xF000) != 0x6000)
        return 0;
    symbol->SetOffset((int)data);
    zCPar_Symbol::SetUseInstance(symbol);
    DoStack(symbol->GetStackPos());
    return 1;
}

void zCParser::Reset()
{
    datsave = 1;
    linkingnr = -1;
    in_funcnr = -1;
    in_classnr = -1;
    stop_on_error = 1;
    instance = -1;
    instance_help = -1;
    curfuncnr = -1;
    self = NULL;
    compiled = 0;
    parse_changed = 0;
    mergemode = 0;
    linec = 0;
    line_start = 0;
    ext_parse = 0;
    pc_start = NULL;
    pc = NULL;
    oldpc = NULL;
    pc_stop = NULL;
    oldpc_stop = NULL;
    params = 0;
    stringcount = 10000;
    in_func = NULL;
    in_class = NULL;
    error = 0;
    errorline = 0;
    prevword_nr = 0;
    tree = NULL;
    treenode = NULL;
    win_code = NULL;
    debugmode = 0;
    labelcount = 0;
    labelpos = NULL;
    add_created = 0;
    msgfunc = NULL;
    for (int i = 0; i < 16; ++i)
    {
        prevword_index[i] = NULL;
        prevline_index[i] = 0;
    }
    for (int i = 0; i < file.numInArray; ++i)
        delete file[i];
    file.numInArray = 0;
    symtab.Clear();
    stringtab.Clear();
    stack.Clear();
    datastack.Clear();
    progressBar = NULL;
    treeload = 0;
    treesave = 0;
    CreateHelpers();
}

void zCParser::SetPercentDone(int percent)
{
    if (progressBar)
        progressBar->SetPercent(percent, zSTRING());
}

void zCParser::CreateHelpers()
{
    zCPar_Symbol* symbol = new zCPar_Symbol;
    symbol->name = zSTRING((char)0xFF) + "INSTANCE_HELP";
    symbol->ele = (symbol->ele & 0xFFFF7001) | 0x7001;
    symtab.Insert(symbol);
    instance_help = symtab.GetIndex(symbol->name);
}

void zCParser::EnableTreeSave(int enabled)
{
    treesave = enabled;
}

void zCParser::EnableDatSave(int enabled)
{
    datsave = enabled;
}

void zCParser::EnableTreeLoad(int enabled)
{
    treeload = enabled;
}

int zCParser::GetIndex(const zSTRING& name)
{
    zSTRING upperName = name.Upper();
    return symtab.GetIndex(upperName);
}

int zCParser::MatchClass(int instance, const zSTRING& name)
{
    int classIndex = GetIndex(name);
    if (classIndex < 0)
        return 0;
    return GetBaseClass(instance) == classIndex;
}

int zCParser::GetBaseClass(zCPar_Symbol* symbol)
{
    if (!symbol)
        return -1;
    if ((symbol->ele & 0xF000) == 0x7000)
        symbol = symbol->GetParent();
    if (symbol && (symbol->ele & 0xF000) == 0x6000)
        symbol = symbol->GetParent();
    return symbol ? symtab.GetIndex(symbol) : -1;
}

int zCParser::GetBaseClass(int index)
{
    return GetBaseClass(symtab.GetSymbol(index));
}

int zCParser::GetBase(int index)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(index);
    if (!symbol)
        return -1;
    int type = (symbol->ele >> 12) & 0xF;
    if (type != 6 && type != 7)
        return -1;
    return symtab.GetIndex(symbol->GetParent());
}

int zCParser::GetPrototype(int startIndex, int parentIndex)
{
    zCPar_Symbol* parent = symtab.GetSymbol(parentIndex);
    if (!parent)
        return -1;
    for (int i = startIndex; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if ((symbol->ele & 0xF000) == 0x6000 && symbol->GetParent() == parent)
            return i;
    }
    return -1;
}

int zCParser::GetInstance(int startIndex, int parentIndex)
{
    zCPar_Symbol* parent = symtab.GetSymbol(parentIndex);
    if (!parent)
        return -1;
    for (int i = startIndex; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if ((symbol->ele & 0xF000) != 0x7000 || !symbol->HasFlag(1) ||
            symbol->name.Search(".") >= 0)
            continue;
        zCPar_Symbol* base = symbol->GetParent();
        if (base && (base->ele & 0xF000) == 0x6000)
            base = base->GetParent();
        if (base == parent)
            return i;
    }
    return -1;
}

zSTRING zCParser::GetSymbolInfo(int instance, int& first, int& second)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (!symbol)
    {
        first = 0;
        second = -1;
        return zSTRING();
    }
    first = (symbol->ele >> 12) & 0xF;
    second = symbol->ele & 0xFFF;
    return symbol->name;
}

zCPar_Symbol* zCParser::GetSymbolInfo(int index)
{
    return symtab.GetSymbol(index);
}

zCPar_Symbol* zCParser::GetSymbol(int index)
{
    return symtab.GetSymbol(index);
}

zCPar_Symbol* zCParser::GetSymbol(const zSTRING& name)
{
    zSTRING upperName = name.Upper();
    return symtab.GetSymbol(upperName);
}

zSTRING zCParser::GetClassVarInfo(int instance, int member, int& type, int& elements)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    type = -1;
    elements = -1;
    if (!symbol || (symbol->ele & 0xF000) != 0x4000)
        return zSTRING();
    while (member-- > 0 && symbol)
        symbol = symbol->GetNext();
    if (!symbol || !symbol->HasFlag(4))
        return zSTRING();
    type = (symbol->ele >> 12) & 0xF;
    elements = symbol->ele & 0xFFF;
    return symbol->name.Copied(symbol->name.Search(".") + 1, symbol->name.Length());
}

void zCParser::GetClassVarInfo(int classIndex, zSTRING& memberName, int& type, int& elements)
{
    zSTRING upperName = memberName.Upper();
    zCPar_Symbol* classSymbol = symtab.GetSymbol(classIndex);
    type = -1;
    elements = -1;
    if (!classSymbol || (classSymbol->ele & 0xF000) != 0x4000)
        return;
    zSTRING fullName = classSymbol->name + "." + upperName;
    zCPar_Symbol* symbol = symtab.GetSymbol(fullName);
    if (symbol && symbol->HasFlag(4))
    {
        type = (symbol->ele >> 12) & 0xF;
        elements = symbol->ele & 0xFFF;
    }
}

zSTRING zCParser::GetText(zSTRING& name, int index)
{
    return GetText(symtab.GetIndex(name), index);
}

zSTRING zCParser::GetText(int instance, int index)
{
    zSTRING value;
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (symbol && (symbol->ele & 0xF000) == 0x3000)
        symbol->GetValue(value, index);
    return value;
}

int zCParser::GetInt(int instance, int index)
{
    int value = 0;
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (symbol && (symbol->ele & 0xF000) == 0x2000)
        symbol->GetValue(value, index);
    return value;
}

zSTRING zCParser::GetInstanceValue(zSTRING& className, int member, void* data, int maxLength)
{
    zSTRING upperName = className.Upper();
    return GetInstanceValue(symtab.GetIndex(upperName), member, data, maxLength);
}

zSTRING zCParser::GetInstanceValue(int classIndex, int member, void* data, int maxLength)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(classIndex);
    if (!symbol || (symbol->ele & 0xF000) != 0x4000 || member >= (symbol->ele & 0xFFF))
        return zSTRING();
    for (int i = 0; i <= member; ++i)
        symbol = symbol->GetNext();
    if (!symbol)
        return zSTRING();
    char* address = (char*)data + symbol->GetOffset();
    switch ((symbol->ele >> 12) & 0xF)
    {
    case 1:
    {
        zSTRING value;
        value.Sprintf("%.9g", *(float*)address);
        return value;
    }
    case 2:
        return zSTRING((long)*(int*)address);
    case 3:
        return zSTRING("\"") + *(zSTRING*)address + zSTRING("\"");
    case 5:
    {
        zCPar_Symbol* function = symtab.GetSymbol(*(int*)address);
        return function ? function->name : zSTRING();
    }
    default:
        return zSTRING();
    }
}

int zCParser::IsValid(zSTRING& className, void* data, zSTRING& expression)
{
    zSTRING source = expression + ";";
    zSTRING upperName = className.Upper();
    zCPar_Symbol* classSymbol = symtab.GetSymbol(upperName);
    if (!classSymbol || (classSymbol->ele & 0xF000) != 0x4000)
        return 0;
    error = 0;
    zCPar_Symbol::SetUseInstanceAdr(data);
    in_class = classSymbol;
    pc = source.ToChar();
    pc_stop = pc + source.Length() + 1;
    zCPar_TreeNode* node = ParseExpression();
    int value = 0;
    if (node && !error)
        value = EvalLeaf(node, 0);
    return value > 0;
}

int zCParser::CheckClassSize(zSTRING& className, int size)
{
    return CheckClassSize(GetIndex(className), size);
}

int zCParser::CheckClassSize(int classIndex, int size)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(classIndex);
    if (!symbol || (symbol->ele & 0xF000) != 0x4000)
        return 0;
    int scriptSize = symbol->GetOffset();
    if (scriptSize != size)
    {
        zSTRING message = "Class - size not compatible : " + symbol->name + "\n" +
            "CPP - size : " + zSTRING((long)size) + "\n" +
            "Script - size : " + zSTRING((long)scriptSize);
        Error(message, -1);
    }
    return scriptSize == size;
}

void zCParser::AddClassOffset(zSTRING& className, int offset)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(className);
    if (!symbol || (symbol->ele & 0xF000) != 0x4000)
        return;
    if (!compiled)
    {
        symbol->SetClassOffset(offset);
        int count = symbol->ele & 0xFFF;
        for (int i = 0; i < count; ++i)
        {
            symbol = symbol->GetNext();
            if (symbol->HasFlag(4))
                symbol->SetOffset(symbol->GetOffset() + offset);
        }
    }
    else if (symbol->GetClassOffset() != offset)
    {
        zSTRING message = "Dat-File is not compatible (Classoffset changed) ! Please reparse !";
        Error(message, -1);
    }
}

static zCPar_Symbol* heroInstance = NULL;

int zCParser::SetInstance(const zSTRING& name, void* data)
{
    zCPar_Symbol* symbol = GetSymbol(name);
    if (name == "HERO" && !heroInstance)
        heroInstance = symbol;
    if (!symbol || (symbol->ele & 0xF000) != 0x7000)
        return 0;
    symbol->SetOffset((int)data);
    return 1;
}

int zCParser::SetInstance(int instance, void* data)
{
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (!symbol || (symbol->ele & 0xF000) != 0x7000)
        return 0;
    symbol->SetOffset((int)data);
    return 1;
}

int zCParser::AutoCompletion(zSTRING& input)
{
    int matches = 0;
    zSTRING completion;
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (!symbol || (symbol->ele & 0xF000) != 0x7000)
            continue;
        if (symbol->name.Search(input) == 0)
        {
            ++matches;
            completion = symbol->name.Copied(input.Length(), symbol->name.Length());
            if (matches > 1)
                return 0;
        }
    }
    if (matches == 1)
        input = completion;
    return matches == 1;
}

int zCParser::GetNumFiles()
{
    return file.numInArray;
}

zSTRING zCParser::GetFileName(int index)
{
    if (index < file.numInArray)
        return file[index]->GetName();
    return zSTRING();
}

int zCParser::IsStackFull()
{
    return stack.ReachedEnd();
}

int zCParser::GetLastErrorLine()
{
    return errorline;
}

void zCParser::Reparse(zSTRING& fileName)
{
    for (int i = 0; i < file.numInArray; ++i)
    {
        if (GetFileName(i) == fileName)
        {
            file[i]->treeload = 0;
            ParseFile(fileName);
            file[i]->treeload = 0;
            break;
        }
    }
}

int zCParser::SaveInstance(zSTRING& name, void* data)
{
    zSTRING upperName = name.Upper();
    return SaveInstance(symtab.GetIndex(upperName), data);
}

int zCParser::SaveInstance(int instance, void* data)
{
    if (file.numInArray <= 0)
        return 0;
    zCPar_Symbol* instanceSymbol = symtab.GetSymbol(instance);
    if (!instanceSymbol || (instanceSymbol->ele & 0xF000) != 0x7000)
        return 0;
    int baseIndex = GetBase(instance);
    zCPar_Symbol* parent = symtab.GetSymbol(baseIndex);
    if (parent && (parent->ele & 0xF000) == 0x6000)
        baseIndex = GetBase(baseIndex);
    zCPar_Symbol* classSymbol = symtab.GetSymbol(baseIndex);
    if (!classSymbol)
        return 0;
    zoptions->ChangeDir(8);
    int fileNumber = instanceSymbol->filenr & 0x7FFFF;
    int line;
    int lineCount;
    int position;
    int positionLength;
    instanceSymbol->GetLineData(line, lineCount, position, positionLength);
    if (fileNumber < 0 || positionLength <= 0)
        return 0;
    zSTRING newLine = "\r\n";
    zSTRING source = "INSTANCE " + instanceSymbol->name + " (" + parent->name + ")" +
        newLine + "{" + newLine;
    zCPar_Symbol* member = classSymbol;
    int memberCount = classSymbol->ele & 0xFFF;
    for (int i = 0; i < memberCount; ++i)
    {
        member = member->GetNext();
        char* address = (char*)data + member->GetOffset();
        zSTRING memberName = member->name;
        int separator = memberName.Search(".");
        if (separator > 0)
            memberName = memberName.Copied(separator + 1, memberName.Length());
        memberName.Lower();
        int elements = member->ele & 0xFFF;
        for (int element = 0; element < elements; ++element)
        {
            zSTRING text = "     " + memberName;
            if (elements > 1)
                text = text + "[" + zSTRING((long)element) + "]";
            text = text + " = ";
            int keep = 0;
            switch ((member->ele >> 12) & 0xF)
            {
            case 1:
            {
                float value = *(float*)address;
                zSTRING number;
                number.Sprintf("%.9g", value);
                text = text + number;
                keep = value != 0.0f;
                address += 4;
                break;
            }
            case 2:
            {
                int value = *(int*)address;
                text = text + zSTRING((long)value);
                keep = value != 0;
                address += 4;
                break;
            }
            case 3:
            {
                zSTRING& value = *(zSTRING*)address;
                text = text + "\"" + value + "\"";
                keep = !value.IsEmpty();
                address += sizeof(zSTRING);
                break;
            }
            case 5:
            {
                int value = *(int*)address;
                zCPar_Symbol* function = symtab.GetSymbol(value);
                if (value > 0 && function)
                {
                    text = text + function->name;
                    keep = 1;
                }
                address += 4;
                break;
            }
            }
            text = text + ";";
            if (keep)
                source = source + text + newLine;
        }
    }
    source = source + "};";
    if (fileNumber >= file.numInArray)
        return 0;
    zoptions->ChangeDir(8);
    zSTRING fileName = file[fileNumber]->GetName();
    zFILE_FILE* sourceFile = (zFILE_FILE*)zfactory->CreateZFile(fileName);
    sourceFile->UpdateBlock(source, position, positionLength);
    sourceFile->Close();
    delete sourceFile;
    int newLineCount = 1;
    for (int i = 0; i < source.Length(); ++i)
    {
        if (source[i] == '\n')
            ++newLineCount;
    }
    int lineDelta = newLineCount - lineCount;
    instanceSymbol->SetLineData(line, newLineCount, position, source.Length());
    for (int i = instance + 1; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (!symbol)
            break;
        int nextLine;
        int nextLineCount;
        int nextPosition;
        int nextPositionLength;
        symbol->GetLineData(nextLine, nextLineCount, nextPosition, nextPositionLength);
        if (nextLine <= line)
            break;
        symbol->SetLineData(nextLine + lineDelta, nextLineCount, nextPosition, nextPositionLength);
    }
    return 1;
}

void zCParser::CloseViews()
{
    if (win_code)
        screen->RemoveItem(win_code);
    debugmode = 0;
}

void zCParser::ShowCode(int instance)
{
    int stackPosition = 0;
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    if (symbol)
    {
        int type = (symbol->ele >> 12) & 0xF;
        if (type == 5 || type == 7)
            symbol->GetStackPos(stackPosition, 0);
    }
    int fontY = screen->FontY();
    if (!win_code)
    {
        int top = fontY * 6;
        win_code = new zCView(0, top, 0x1FFF, top + fontY * 12, 2);
        screen->InsertItem(win_code, 0);
        win_code->ClrFlags(0x200);
    }
    if (!win_code->ondesk)
        screen->InsertItem(win_code, 0);
    debugmode = 1;
    ShowPCode(stackPosition, win_code, 0);
}

void zCParser::SaveInfoFile(zSTRING fileName)
{
    zFILE* output = zfactory->CreateZFile(fileName);
    output->Create();
    output->Write(zSTRING("// ***  Externals  ***\n\n"));
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (!symbol->HasFlag(8))
            continue;
        zSTRING text = "FUNC ";
        if (!symbol->HasFlag(2))
            text = text + "VOID";
        else
        {
            switch (symbol->GetOffset())
            {
            case 1:
                text = text + "FLOAT";
                break;
            case 2:
                text = text + "INT";
                break;
            case 3:
                text = text + "STRING";
                break;
            case 7:
                text = text + "INSTANCE";
                break;
            }
        }
        text = text + " " + symbol->name + " ( ";
        int parameters = symbol->ele & 0xFFF;
        for (int parameter = 0; parameter < parameters; ++parameter)
        {
            zCPar_Symbol* parameterSymbol = symtab.GetSymbol(i + parameter + 1);
            switch ((parameterSymbol->ele >> 12) & 0xF)
            {
            case 1:
                text = text + "VAR FLOAT r";
                break;
            case 2:
                text = text + "VAR INT i";
                break;
            case 3:
                text = text + "VAR STRING s";
                break;
            case 5:
                text = text + "VAR FUNC f";
                break;
            case 7:
                text = text + "VAR INSTANCE n";
                break;
            }
            text = text + zSTRING((long)parameter);
            if (parameter + 1 < parameters)
                text = text + ", ";
        }
        text = text + " ) { ";
        if (symbol->HasFlag(2))
            text = text + "return 0; ";
        text = text + "};";
        output->Write(text + "\n");
    }
    output->Close();
    delete output;
}

void zCParser::SetInfoFile(zCList<zSTRING>* list, const zSTRING& fileName)
{
    add_funclist = list;
    add_filename = fileName;
    add_created = 0;
}

int zCParser::IsInAdditionalInfo(const zSTRING& name)
{
    if (add_funclist)
    {
        for (zCList<zSTRING>* entry = add_funclist->next; entry; entry = entry->next)
        {
            zSTRING symbolName = *entry->data;
            symbolName.Upper();
            if (symbolName == name)
                return 1;
        }
    }
    return 0;
}

int zCParser::WriteAdditionalInfo(zSTRING& name, int line, int position)
{
    zFILE_FILE currentDirectory;
    currentDirectory.SetCurrentDir();
    zoptions->ChangeDir(17);
    zFILE* output = zfactory->CreateZFile(add_filename);
    if (add_created)
    {
        output->Open(1);
        output->Append();
    }
    else
    {
        output->Create();
        add_created = 1;
    }
    output->Write("FILE   { " + add_filename + " }\n");
    output->Write("SYMBOL { " + name + " }\n");
    output->Write("LINE   { " + zSTRING((long)line) + " }\n");
    output->Write("FILEPOS{ " + zSTRING((long)position) + " }\n\n");
    output->Close();
    delete output;
    currentDirectory.ChangeDir(0);
    return 1;
}

int zCParser::ClearAllInstanceRefs()
{
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (symbol && (symbol->ele & 0xF000) == 0x7000)
            symbol->SetOffset(0);
    }
    return 1;
}

int zCParser::ClearInstanceRefs(void* data)
{
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (symbol && (symbol->ele & 0xF000) == 0x7000 && symbol->GetInstanceAdr() == data)
            symbol->SetOffset(0);
    }
    return 1;
}

int zCParser::ResetGlobalVars()
{
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (!symbol || (symbol->ele & 0xF000) != 0x2000 || ((symbol->ele >> 16) & 0x3F))
            continue;
        int elements = symbol->ele & 0xFFF;
        for (int element = 0; element < elements; ++element)
            symbol->SetValue(0, element);
    }
    return 1;
}

void zCParser::CreateVarReferenceList(const zSTRING& className, zCArray<int>& references)
{
    int classIndex = parser.GetIndex(className);
    if (classIndex < 0)
        return;
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (!symbol || (symbol->ele & 0xF000) != 0x7000)
            continue;
        zCPar_Symbol* parent = symbol->GetParent();
        if (parent && (parent->ele & 0xF000) == 0x6000)
            parent = parent->GetParent();
        if (parent && symtab.GetIndex(parent) == classIndex)
            references.InsertEnd(i);
    }
}

void zCParser::ResetWithVarReferenceList(zCArray<int>& references, void* data)
{
    for (int i = 0; i < references.numInArray; ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(references[i]);
        if (symbol && (symbol->ele & 0xF000) == 0x7000 && symbol->GetInstanceAdr() == data)
            symbol->SetOffset(0);
    }
}

int zCParser::SaveGlobalVars(zCArchiver& archiver)
{
    int count = 0;
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (symbol && (symbol->ele & 0xF000) == 0x2000 && !((symbol->ele >> 16) & 0x3F) &&
            (symbol->ele & 0xFFF) > 0)
            ++count;
    }
    archiver.WriteInt(count);
    int saved = 0;
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        if (!symbol || (symbol->ele & 0xF000) != 0x2000 || ((symbol->ele >> 16) & 0x3F) ||
            (symbol->ele & 0xFFF) <= 0)
            continue;
        zSTRING entry = zSTRING((long)saved);
        archiver.WriteString("symName" + entry, symbol->name);
        int elements = symbol->ele & 0xFFF;
        archiver.WriteInt("symName" + entry + "cnt", elements);
        for (int element = 0; element < elements; ++element)
        {
            int value;
            symbol->GetValue(value, element);
            archiver.WriteInt("symValue" + entry + "_" + zSTRING((long)element), value);
        }
        ++saved;
    }
    return 1;
}

int zCParser::LoadGlobalVars(zCArchiver& archiver)
{
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
    {
        zSTRING entry = zSTRING((long)i);
        zSTRING name = archiver.ReadString("symName" + entry);
        name.Upper();
        zCPar_Symbol* symbol = symtab.GetSymbol(name);
        if (!symbol || (symbol->ele & 0xF000) != 0x2000 || ((symbol->ele >> 16) & 0x3F) ||
            (symbol->ele & 0xFFF) <= 0)
        {
            archiver.ReadInt("symValue" + entry);
            continue;
        }
        int elements = archiver.ReadInt("symName" + entry + "cnt");
        for (int element = 0; element < elements; ++element)
        {
            int value = archiver.ReadInt("symValue" + entry + "_" + zSTRING((long)element));
            symbol->SetValue(value, element);
        }
    }
    return 1;
}

static int treeNodeCount = 0;

zCPar_TreeNode::zCPar_TreeNode()
    : token(0), value(0), info(0), left(NULL), right(NULL), next(NULL), cNr(treeNodeCount++)
{
}

zCPar_TreeNode::zCPar_TreeNode(int token, int value)
    : token((unsigned char)token), value(value), info(0), left(NULL), right(NULL), next(NULL),
      cNr(treeNodeCount++)
{
}

zCPar_TreeNode::zCPar_TreeNode(int token, zSTRING& name, int value)
    : name(name), token((unsigned char)token), value(value), info(0), left(NULL), right(NULL),
      next(NULL), cNr(treeNodeCount++)
{
}

zCPar_TreeNode::~zCPar_TreeNode()
{
    delete right;
    right = NULL;
    delete left;
    left = NULL;
    delete next;
    next = NULL;
}

void zCPar_TreeNode::SetLeft(zCPar_TreeNode* node)
{
    left = node;
}

void zCPar_TreeNode::SetRight(zCPar_TreeNode* node)
{
    right = node;
}

void zCPar_TreeNode::SetNext(zCPar_TreeNode* node)
{
    next = node;
}

zCPar_TreeNode* zCPar_TreeNode::SeekEndTree(zCPar_TreeNode* node)
{
    zCPar_TreeNode* result = node;
    while (result->next)
        result = result->next;
    return result;
}

zCPar_TreeNode* zCPar_TreeNode::MakeDyadicNode(zCPar_TreeNode* left, int token,
    zCPar_TreeNode* right)
{
    zCPar_TreeNode* node = new zCPar_TreeNode(token, 0);
    node->left = left;
    node->right = right;
    return node;
}

zCPar_TreeNode* zCPar_TreeNode::MakeMonadicNode(int token, zCPar_TreeNode* left)
{
    zCPar_TreeNode* node = new zCPar_TreeNode(token, 0);
    node->right = left;
    return node;
}

void zCParser::GetOperatorString(int token, zSTRING& text)
{
    static const char* operators[] = {
        " +", " -", " *", " /", " %", " |", " &", " <", " >", " =", "??", "||", "&&",
        "<<", ">>", "<=", "==", "!=", ">=", "+=", "-=", "*=", "/="
    };
    if (token >= 0 && token <= 22)
        text = operators[token];
    else if (token == 30 || token == 31)
        text = operators[token - 30];
    else if (token == 32)
        text = " !";
    else if (token == 33)
        text = " ~";
    else
        text = "??";
}

void zCParser::FindNext(char* text)
{
    prevword_nr = (prevword_nr + 1) & 0xF;
    prevword_index[prevword_nr] = pc;
    prevline_index[prevword_nr] = linec;
    char* start = pc;
    pc = std::strstr(pc, text);
    if (!pc)
    {
        zSTRING message = "Unexpected End of File.";
        Error(message, 0);
        return;
    }
    while (start)
    {
        char* newLine = std::strchr(start, '\n');
        if (!newLine)
            break;
        if (newLine <= pc)
        {
            start = newLine + 1;
            ++linec;
            line_start = start - pc_start;
        }
        if (!start || start > pc)
            break;
    }
}

void zCParser::PrevWord()
{
    pc = prevword_index[prevword_nr];
    linec = prevline_index[prevword_nr];
    prevword_nr = (prevword_nr - 1) & 0xF;
}

void zCParser::ReadWord(zSTRING& word)
{
    static const char separators[] = ";,:=><*+-/?'%()/&!|{}[]\"~";
    for (;;)
    {
        prevword_nr = (prevword_nr + 1) & 0xF;
        prevword_index[prevword_nr] = pc;
        prevline_index[prevword_nr] = linec;
        char text[1024];
        int length = 0;
        while (pc < pc_stop)
        {
            char c = *pc++;
            if (c > ' ')
            {
                if (std::strchr(separators, c))
                {
                    if (length <= 0)
                        text[length++] = c;
                    else
                        --pc;
                    break;
                }
                text[length++] = c;
            }
            else
            {
                if (c == '\n')
                {
                    ++linec;
                    line_start = pc - pc_start;
                    if (!length)
                    {
                        prevword_index[prevword_nr] = pc;
                        prevline_index[prevword_nr] = linec;
                    }
                }
                if (length > 0)
                    break;
            }
        }
        if (!length && pc >= pc_stop)
        {
            if (ext_parse)
            {
                ext_parse = 0;
                pc = oldpc;
                pc_stop = oldpc_stop;
                continue;
            }
        }
        text[length] = 0;
        word = text;
        word.Upper();
        if (word != "/")
            break;
        if (*pc == '/')
        {
            ++pc;
            FindNext("\n");
            ++pc;
        }
        else if (*pc == '*')
        {
            ++pc;
            FindNext("*/");
            pc += 2;
        }
        else
            break;
    }
}

void zCParser::Match(zSTRING& expected)
{
    ReadWord(aword);
    if (!(aword == expected))
    {
        zSTRING message = "Expected '" + expected + "'";
        Error(message, 0);
    }
}

int zCParser::ReadVarType()
{
    ReadWord(aword);
    if (aword == "INT")
        return 2;
    if (aword == "FLOAT")
        return 1;
    if (aword == "STRING")
        return 3;
    if (aword == "FUNC")
        return 5;
    int classIndex = symtab.GetIndex(aword);
    zCPar_Symbol* symbol = symtab.GetSymbol(classIndex);
    if (symbol && (symbol->ele & 0xF000) == 0x4000)
        return classIndex + 7;
    zSTRING message = "Syntax error: " + aword;
    Error(message, 0);
    return -1;
}

int zCParser::ReadFuncType()
{
    ReadWord(aword);
    if (aword == "VOID")
        return 0;
    if (aword == "INT")
        return 2;
    if (aword == "FLOAT")
        return 1;
    if (aword == "STRING")
        return 3;
    zCPar_Symbol* symbol = symtab.GetSymbol(aword);
    if (symbol && (symbol->ele & 0xF000) == 0x4000)
        return 7;
    zSTRING message = "Syntax error: " + aword;
    Error(message, 0);
    return -1;
}

int zCParser::ReadInt()
{
    int negative = 0;
    ReadWord(aword);
    if (aword == "-" || aword == "+")
    {
        negative = aword == "-";
        ReadWord(aword);
    }
    if (aword.Length() > 0 && aword[0] >= '0' && aword[0] <= '9')
    {
        for (int i = 1; i < aword.Length(); ++i)
        {
            if (aword[i] < '0' || aword[i] > '9')
            {
                zSTRING message = "Expected an Integer-Value :" + aword;
                Error(message, 0);
                return -1;
            }
        }
        int value = aword.ToInt();
        return negative ? -value : value;
    }
    zCPar_Symbol* symbol = symtab.GetSymbol(FindIndex(aword));
    if (!symbol)
    {
        zSTRING message = "Unknown variable: " + aword;
        Error(message, 0);
        return -1;
    }
    if ((symbol->ele & 0xF000) != 0x2000 || !symbol->HasFlag(1))
    {
        zSTRING message = "Expected integer constant : " + aword;
        Error(message, 0);
        return -1;
    }
    int value;
    symbol->GetValue(value, 0);
    return negative ? -value : value;
}

float zCParser::ReadFloat()
{
    int negative = 0;
    ReadWord(aword);
    if (aword == "-" || aword == "+")
    {
        negative = aword == "-";
        ReadWord(aword);
    }
    zSTRING text = aword;
    int point = 0;
    int exponent = 0;
    for (int i = 0; i < aword.Length(); ++i)
    {
        char c = aword[i];
        if (c >= '0' && c <= '9')
            continue;
        if (c == '.' && !point)
        {
            point = 1;
            continue;
        }
        if (c == 'E')
        {
            if (i != aword.Length() - 1)
            {
                zSTRING message = "Syntax error: " + aword;
                Error(message, 0);
            }
            exponent = 1;
            break;
        }
        zSTRING message = "Syntax error: " + aword;
        Error(message, 0);
        return -1.0f;
    }
    if (exponent)
        text = text + zSTRING((long)ReadInt());
    float value = text.ToFloat();
    return negative ? -value : value;
}

void zCParser::ReadString(zSTRING& value)
{
    value = "";
    ReadWord(aword);
    if (aword != "\"")
    {
        zSTRING message = "Expected '\"'";
        Error(message, 0);
    }
    while (*pc != '"')
    {
        char character[2] = { *pc, 0 };
        value += character;
        if (pc >= pc_stop)
        {
            zSTRING message = "Unexpected End of File.";
            Error(message, 0);
        }
        else
            ++pc;
    }
    ++pc;
}

int zCParser::ReadArray()
{
    zSTRING token;
    ReadWord(token);
    if (token != "[")
    {
        PrevWord();
        return 0;
    }
    int size = ReadInt();
    zSTRING close = "]";
    Match(close);
    return size;
}

zCPar_TreeNode* zCParser::CreateLeaf(int token, zCPar_TreeNode* previous)
{
    zCPar_TreeNode* node = new zCPar_TreeNode(token, 0);
    if (previous)
        previous->next = node;
    return node;
}

zCPar_TreeNode* zCParser::CreateStringLeaf()
{
    ReadWord(aword);
    if (aword == "\"")
    {
        PrevWord();
        zSTRING value;
        ReadString(value);
        zCPar_TreeNode* node = CreateLeaf(0x62, NULL);
        node->name = value;
        node->value = 0;
        node->info = 3;
        return node;
    }
    zSTRING next;
    ReadWord(next);
    PrevWord();
    if (next != "(")
    {
        zCPar_TreeNode* node = CreateLeaf(0x34, NULL);
        node->name = aword;
        node->value = 0;
        node->info = 3;
        return node;
    }
    zCPar_TreeNode* previous = treenode;
    zCPar_TreeNode* node = CreateLeaf(0x5A, NULL);
    treenode = node;
    DeclareFuncCall(aword, 3);
    treenode = previous;
    return node;
}

zCPar_TreeNode* zCParser::CreateFloatLeaf()
{
    int negative = 0;
    ReadWord(aword);
    if (aword.Length() > 0 && aword[0] == '-')
        negative = 1;
    if (negative || (aword.Length() > 0 && aword[0] == '+'))
        ReadWord(aword);
    if (aword.Length() > 0 && aword[0] >= '0' && aword[0] <= '9')
    {
        if (negative)
            PrevWord();
        PrevWord();
        float value = ReadFloat();
        zCPar_TreeNode* node = CreateLeaf(0x33, NULL);
        node->fvalue = value;
        node->info = 1;
        return node;
    }
    if (negative)
        PrevWord();
    zCPar_TreeNode* node = CreateLeaf(0x34, NULL);
    node->name = aword;
    node->value = 0;
    node->info = 1;
    return node;
}

int zCParser::GetNextToken()
{
    ReadWord(aword);
    char first = aword.Length() > 0 ? aword[0] : ' ';
    if ((first >= 'A' && first <= 'Z') || first == '_' || first == '#')
    {
        zSTRING next;
        ReadWord(next);
        PrevWord();
        return next == "(" ? 0x5C : 0x34;
    }
    if (first >= '0' && first <= '9')
    {
        for (int i = 1; i < aword.Length(); ++i)
        {
            if (aword[i] < '0' || aword[i] > '9')
            {
                zSTRING message = "Syntax error: " + aword;
                Error(message, 0);
            }
        }
        return 0x33;
    }
    zSTRING operators = "+-*/%|&<>=";
    int token = operators.Search(aword);
    if (token < 0)
    {
        switch (first)
        {
        case '!':
            if (*pc == '=')
            {
                ++pc;
                return 0x11;
            }
            return 0x20;
        case '(':
            return 0x28;
        case ')':
            return 0x29;
        case ',':
            return 0x2B;
        case ';':
            return 0x2A;
        case '{':
        case '}':
            return 0x2C;
        case '~':
            return 0x21;
        default:
        {
            zSTRING message = "Syntax Error.";
            Error(message, 0);
            return 0x2D;
        }
        }
    }
    if (first == '|' && *pc == '|')
    {
        ++pc;
        return 0x0B;
    }
    if (first == '&' && *pc == '&')
    {
        ++pc;
        return 0x0C;
    }
    if (first == '<')
    {
        if (*pc == '<')
        {
            ++pc;
            return 0x0D;
        }
        if (*pc == '=')
        {
            ++pc;
            return 0x0F;
        }
    }
    else if (first == '>')
    {
        if (*pc == '>')
        {
            ++pc;
            return 0x0E;
        }
        if (*pc == '=')
        {
            ++pc;
            return 0x12;
        }
    }
    if (*pc == '=')
    {
        if (first == '=')
        {
            ++pc;
            return 0x10;
        }
        if (first == '+')
        {
            ++pc;
            return 0x13;
        }
        if (first == '-')
        {
            ++pc;
            return 0x14;
        }
        if (first == '*')
        {
            ++pc;
            return 0x15;
        }
        if (first == '/')
        {
            ++pc;
            return 0x16;
        }
    }
    return token;
}

zCPar_TreeNode* zCParser::Parse_Expression_Primary(int& token)
{
    zCPar_TreeNode* result = NULL;
    if (token == 0x33)
    {
        result = new zCPar_TreeNode(token, aword.ToInt());
        token = GetNextToken();
    }
    else if (token == 0x34)
    {
        zSTRING name = aword;
        int index = 0;
        ReadWord(aword);
        if (aword == "(")
            ReadWord(aword);
        else if (aword == "[")
        {
            index = ReadInt();
            zSTRING close = "]";
            Match(close);
        }
        else
            PrevWord();
        result = new zCPar_TreeNode(0x34, name, index);
        result->info = 2;
        token = GetNextToken();
    }
    else if (token == 0x5C)
    {
        zCPar_TreeNode* oldTree = tree;
        zCPar_TreeNode* oldNode = treenode;
        result = new zCPar_TreeNode(0x5A, 0);
        treenode = result;
        tree = result;
        DeclareFuncCall(aword, 2);
        tree = result;
        treenode = oldNode;
        token = GetNextToken();
    }
    else if (token == 0x28)
    {
        token = GetNextToken();
        result = Parse_Expression(token, -1);
        if (token == 0x29)
            token = GetNextToken();
        else
        {
            zSTRING message = "Missing ')'";
            Error(message, 0);
        }
    }
    else if (token == 0 || token == 1 || (token >= 0x1E && token <= 0x21))
    {
        int unaryToken = token <= 1 ? token + 0x1E : token;
        token = GetNextToken();
        result = zCPar_TreeNode::MakeMonadicNode(unaryToken, Parse_Expression_Primary(token));
    }
    else
    {
        zSTRING message = "Syntax error: " + aword;
        Error(message, 0);
    }
    return result;
}

zCPar_TreeNode* zCParser::Parse_Expression(int& token, int minPriority)
{
    static const int priorities[] = {
        10, 10, 11, 11, 11, 4, 6, 8, 8, 0, -1, 2, 3, 9, 9, 8, 7, 7, 8, 0, 0, 0, 0
    };
    zCPar_TreeNode* result = Parse_Expression_Primary(token);
    if (!result)
        return result;
    while (token <= 0x21)
    {
        int priority = token < 23 ? priorities[token] : 0;
        if (priority <= minPriority)
            break;
        int operation = token;
        token = GetNextToken();
        zCPar_TreeNode* right = Parse_Expression(token, priority);
        if ((operation == 9 || (operation >= 0x13 && operation <= 0x16)) &&
            result->token != 0x34)
        {
            zSTRING message = "Assignment : Left operand is not a lvalue";
            Error(message, 0);
        }
        result = zCPar_TreeNode::MakeDyadicNode(result, operation, right);
    }
    return result;
}

zCPar_TreeNode* zCParser::ParseExpressionEx(zSTRING& expression)
{
    oldpc = pc;
    oldpc_stop = pc_stop;
    pc = expression.ToChar();
    pc_stop = pc + expression.Length() + 1;
    ext_parse = 1;
    int token = GetNextToken();
    return Parse_Expression(token, -1);
}

zCPar_TreeNode* zCParser::ParseExpression()
{
    int token = GetNextToken();
    return Parse_Expression(token, -1);
}

int zCParser::EvalLeaf(zCPar_TreeNode* node, int target)
{
    int token = node->token;
    if (token == 0x33)
        return node->value;
    if (token == 0x34)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(FindIndex(node->name));
        if (!symbol)
        {
            zSTRING message = "Unknown Var '" + node->name + "'.";
            Error(message, 0);
        }
        if (target == 9 || (target >= 0x13 && target <= 0x16))
            return (int)symbol->GetDataAdr(node->value);
        if (!symbol->HasFlag(4))
        {
            int value;
            symbol->GetValue(value, node->value);
            return value;
        }
        return *(int*)symbol->GetDataAdr(node->value);
    }
    if (token > 0x21)
    {
        zSTRING message = "Error in EvalLeaf.";
        Error(message, 0);
        return 0;
    }
    int leftValue = 0;
    int rightValue;
    if (token >= 0x1E)
        rightValue = EvalLeaf(node->right, 0);
    else
    {
        leftValue = EvalLeaf(node->left, token);
        rightValue = EvalLeaf(node->right, 0);
    }
    switch (token)
    {
    case 0:
        return leftValue + rightValue;
    case 1:
        return leftValue - rightValue;
    case 2:
        return leftValue * rightValue;
    case 3:
        return leftValue / rightValue;
    case 4:
        return leftValue % rightValue;
    case 5:
        return leftValue | rightValue;
    case 6:
        return leftValue & rightValue;
    case 7:
        return leftValue < rightValue;
    case 8:
        return leftValue > rightValue;
    case 9:
        *(int*)leftValue = rightValue;
        return rightValue;
    case 0x0B:
        return leftValue || rightValue;
    case 0x0C:
        return leftValue && rightValue;
    case 0x0D:
        return leftValue << (rightValue & 0x1F);
    case 0x0E:
        return leftValue >> (rightValue & 0x1F);
    case 0x0F:
        return leftValue <= rightValue;
    case 0x10:
        return leftValue == rightValue;
    case 0x11:
        return leftValue != rightValue;
    case 0x12:
        return leftValue >= rightValue;
    case 0x13:
        *(int*)leftValue += rightValue;
        return *(int*)leftValue;
    case 0x14:
        *(int*)leftValue -= rightValue;
        return *(int*)leftValue;
    case 0x15:
        *(int*)leftValue *= rightValue;
        return *(int*)leftValue;
    case 0x16:
        *(int*)leftValue /= rightValue;
        return *(int*)leftValue;
    case 0x1E:
        return rightValue;
    case 0x1F:
        return -rightValue;
    case 0x20:
        return !rightValue;
    case 0x21:
        return ~rightValue;
    default:
    {
        zSTRING message = "Code - Error in Tree-Expression !";
        Error(message, 0);
        return leftValue;
    }
    }
}

void zCParser::DeclareVar(int flags)
{
    for (;;)
    {
        int type = ReadVarType();
        zCPar_Symbol* classSymbol = NULL;
        if (type >= 7)
        {
            classSymbol = symtab.GetSymbol(type - 7);
            type = 7;
        }
        for (;;)
        {
            int startLine = linec;
            int startPosition = line_start;
            zSTRING name;
            ReadWord(name);
            int elements = 1;
            ReadWord(aword);
            if (aword == "[")
            {
                elements = ReadInt();
                zSTRING close = "]";
                Match(close);
            }
            else
                PrevWord();
            zCPar_Symbol* symbol = new zCPar_Symbol;
            symbol->name = name;
            symbol->ele = (type << 12) | elements;
            if (classSymbol)
                symbol->SetParent(classSymbol);
            else if (in_class)
                symbol->SetParent(in_class);
            else if (in_func)
                symbol->SetParent(in_func);
            if (flags)
                symbol->SetFlag(1);
            if (in_func)
            {
                symbol->name = in_func->name + "." + symbol->name;
                if (params)
                {
                    in_func->ele = (in_func->ele & ~0xFFF) | ((in_func->ele + 1) & 0xFFF);
                    int parameterType = type;
                    int assignmentToken = 9;
                    int variableToken = 0x34;
                    if (parameterType == 3)
                        assignmentToken = 0x46;
                    else if (parameterType == 1)
                        assignmentToken = 0x49;
                    else if (parameterType == 5)
                        assignmentToken = 0x48;
                    else if (parameterType == 7)
                    {
                        variableToken = 0x43;
                        assignmentToken = 0x4A;
                    }
                    else if (parameterType != 2)
                    {
                        zSTRING message = "Parameters of this type are not supported.";
                        Error(message, 0);
                    }
                    zCPar_TreeNode* variable = new zCPar_TreeNode(variableToken, symbol->name, 0);
                    variable->info = parameterType;
                    zCPar_TreeNode* assignment = new zCPar_TreeNode(assignmentToken, 0);
                    variable->next = assignment;
                    assignment->next = treenode->next;
                    treenode->next = variable;
                    if ((in_func->ele & 0xFFF) == 1)
                        treenode = assignment;
                }
            }
            else if (in_class)
            {
                symbol->name = in_class->name + "." + symbol->name;
                if (params)
                {
                    in_class->ele = (in_class->ele & ~0xFFF) | ((in_class->ele + 1) & 0xFFF);
                    symbol->SetParent(in_class);
                    symbol->SetFlag(4);
                }
            }
            if (!symtab.Insert(symbol))
            {
                zSTRING message = "Redefined: " + symbol->name;
                Error(message, 0);
            }
            ReadWord(aword);
            if (aword == "=")
            {
                if (elements > 1)
                {
                    zSTRING open = "{";
                    Match(open);
                }
                for (int element = 0; element < elements; ++element)
                {
                    switch (type)
                    {
                    case 1:
                        symbol->SetValue(ReadFloat(), element);
                        break;
                    case 2:
                    {
                        int token = GetNextToken();
                        int value = EvalLeaf(Parse_Expression(token, -1), 0);
                        symbol->SetValue(value, element);
                        PrevWord();
                        break;
                    }
                    case 3:
                    {
                        zSTRING value;
                        ReadString(value);
                        symbol->SetValue(value, element);
                        break;
                    }
                    case 5:
                    {
                        ReadWord(aword);
                        int function = GetIndex(aword);
                        if (function < 0)
                        {
                            zSTRING message = "Unknown variable: " + aword;
                            Error(message, 0);
                        }
                        symbol->ele = (symbol->ele & ~0xF000) | 0x2000;
                        symbol->SetValue(function, element);
                        symbol->ele = (symbol->ele & ~0xF000) | 0x5000;
                        break;
                    }
                    }
                    if (element + 1 < elements)
                    {
                        zSTRING comma = ",";
                        Match(comma);
                    }
                }
                if (elements > 1)
                {
                    zSTRING close = "}";
                    Match(close);
                }
                if (type == 5)
                    symbol->ele = (symbol->ele & ~0xF000) | 0x2000;
            }
            symbol->SetLineData(startLine, linec - startLine + 1, startPosition,
                pc - pc_start - startPosition + 3);
            symbol->SetFileNr(file.numInArray - 1);
            ReadWord(aword);
            if (aword != ",")
            {
                PrevWord();
                return;
            }
            if (in_func)
                break;
        }
        zSTRING semicolon = ";";
        Match(semicolon);
    }
}

void zCParser::DeclareClass()
{
    int startLine = linec;
    int startPosition = line_start;
    zSTRING name;
    ReadWord(name);
    params = 1;
    in_class = new zCPar_Symbol;
    in_class->name = name;
    in_class->ele = 0x4000;
    if (!symtab.Insert(in_class))
    {
        zSTRING message = "Redefined: " + in_class->name;
        Error(message, 0);
    }
    ParseBlock();
    in_class->SetLineData(startLine, linec - startLine + 1, startPosition,
        pc - pc_start - startPosition + 3);
    in_class->SetFileNr(file.numInArray - 1);
    in_class = NULL;
    in_classnr = 0;
    params = 0;
}

void zCParser::DeclareInstance()
{
    int startLine = linec;
    int startPosition = line_start;
    zCArray<zSTRING> names;
    for (;;)
    {
        zSTRING name;
        ReadWord(name);
        if (name.Length() > 0 && name[0] >= '0' && name[0] <= '9')
        {
            zSTRING message = "Syntax Error. First letter is a digit. Tnstancename : " + name;
            Error(message, 0);
        }
        names.InsertEnd(name);
        ReadWord(aword);
        if (aword != ",")
            break;
    }
    PrevWord();
    zSTRING open = "(";
    Match(open);
    ReadWord(aword);
    zCPar_Symbol* parent = symtab.GetSymbol(aword);
    int parentType = parent ? (parent->ele >> 12) & 0xF : 0;
    if (!parent || (parentType != 4 && parentType != 6))
    {
        zSTRING message = "No valid class: " + aword;
        Error(message, 0);
        return;
    }
    zSTRING close = ")";
    Match(close);
    in_class = parentType == 6 ? parent->GetParent() : parent;
    zCArray<zCPar_Symbol*> instances;
    for (int i = 0; i < names.numInArray; ++i)
    {
        in_func = new zCPar_Symbol;
        in_func->name = names[i];
        in_func->ele = 0x7000;
        in_func->SetParent(parent);
        if (!symtab.Insert(in_func))
        {
            if (!mergemode)
            {
                zSTRING message = "Redefined: " + in_func->name;
                Error(message, 0);
            }
            else
            {
                delete in_func;
                in_func = symtab.GetSymbol(names[i]);
                if (!in_func || (in_func->ele & 0xF000) != 0x7000)
                {
                    zSTRING message = "Redefined: " + names[i];
                    Error(message, 0);
                    return;
                }
                in_func->SetFlag(0x10);
            }
        }
        instances.InsertEnd(in_func);
        zCPar_TreeNode* node = CreateLeaf(0x60, treenode);
        treenode = node;
        node->name = names[i];
    }
    ReadWord(aword);
    PrevWord();
    if (aword != ";")
    {
        ParseBlock();
        in_func->SetFlag(1);
    }
    for (int i = 0; i < instances.numInArray; ++i)
    {
        instances[i]->SetLineData(startLine, linec - startLine + 1, startPosition,
            pc - pc_start - startPosition + 3);
        instances[i]->SetFileNr(file.numInArray - 1);
    }
    in_func = NULL;
    in_class = NULL;
    in_classnr = 0;
    treenode = CreateLeaf(0x3C, treenode);
    treenode = CreateLeaf(0x61, treenode);
}

void zCParser::DeclarePrototype()
{
    int startLine = linec;
    int startPosition = line_start;
    zSTRING name;
    ReadWord(name);
    if (name.Length() > 0 && name[0] >= '0' && name[0] <= '9')
    {
        zSTRING message = "Syntax Error. First letter is a digit in prototypename.";
        Error(message, 0);
    }
    zSTRING open = "(";
    Match(open);
    ReadWord(aword);
    zCPar_Symbol* classSymbol = symtab.GetSymbol(aword);
    if (!classSymbol || (classSymbol->ele & 0xF000) != 0x4000)
    {
        zSTRING message = "No valid class: " + aword;
        Error(message, 0);
        return;
    }
    zSTRING close = ")";
    Match(close);
    in_func = new zCPar_Symbol;
    in_func->name = name;
    in_func->ele = 0x6000;
    in_func->SetParent(classSymbol);
    in_class = classSymbol;
    if (!symtab.Insert(in_func))
    {
        zSTRING message = "Redefined: " + in_func->name;
        Error(message, 0);
    }
    zCPar_TreeNode* node = CreateLeaf(0x60, treenode);
    treenode = node;
    node->name = name;
    ParseBlock();
    in_func->SetLineData(startLine, linec - startLine + 1, startPosition,
        pc - pc_start - startPosition + 3);
    in_func->SetFileNr(file.numInArray - 1);
    in_func = NULL;
    in_class = NULL;
    in_classnr = 0;
    treenode = CreateLeaf(0x3C, treenode);
    treenode = CreateLeaf(0x61, treenode);
}

void zCParser::DeclareIf()
{
    int endLabel = -1;
    for (;;)
    {
        int token = GetNextToken();
        treenode->next = Parse_Expression(token, -1);
        while (treenode->next)
            treenode = treenode->next;
        int falseLabel = labelcount++;
        treenode = CreateLeaf(0x4C, treenode);
        treenode->value = falseLabel;
        PrevWord();
        ParseBlock();
        ReadWord(aword);
        if (aword == "ELSE")
        {
            if (endLabel < 0)
                endLabel = labelcount++;
            treenode = CreateLeaf(0x4B, treenode);
            treenode->value = endLabel;
            ReadWord(aword);
            if (aword == "{")
            {
                treenode = CreateLeaf(0x5B, treenode);
                treenode->value = falseLabel;
                PrevWord();
                ParseBlock();
                ReadWord(aword);
                if (aword != ";")
                {
                    zSTRING message = "Expected ';'";
                    Error(message, 0);
                }
                break;
            }
            PrevWord();
            zSTRING ifWord = "IF";
            Match(ifWord);
        }
        treenode = CreateLeaf(0x5B, treenode);
        treenode->value = falseLabel;
        if (aword != "IF")
            break;
    }
    if (endLabel >= 0)
    {
        treenode = CreateLeaf(0x5B, treenode);
        treenode->value = endLabel;
    }
    PrevWord();
}

void zCParser::DeclareAssign(zSTRING& name)
{
    zSTRING targetName = name;
    int symbolIndex = FindIndex(name);
    if (symbolIndex < 0)
    {
        zSTRING message = "Unknown variable: " + targetName;
        Error(message, 0);
        return;
    }
    zCPar_Symbol* symbol = symtab.GetSymbol(symbolIndex);
    int type = (symbol->ele >> 12) & 0xF;
    if (type == 2)
    {
        PrevWord();
        int token = GetNextToken();
        treenode->next = Parse_Expression(token, -1);
        treenode = treenode->next;
        PrevWord();
        return;
    }
    if (type == 5)
    {
        if (!symbol->HasFlag(1))
            DeclareAssignFunc(targetName);
        else
            DeclareFuncCall(targetName, 0);
        return;
    }
    int index = 0;
    if (type == 1 || type == 3)
    {
        ReadWord(aword);
        if (aword == "[")
        {
            index = ReadInt();
            zSTRING close = "]";
            Match(close);
        }
        else
            PrevWord();
    }
    zSTRING assign = "=";
    Match(assign);
    if (type == 1)
    {
        ReadWord(aword);
        zCPar_Symbol* source = GetSymbol(aword);
        if (!source || (source->ele & 0xF000) != 0x5000)
        {
            PrevWord();
            treenode = CreateLeaf(0x33, treenode);
            treenode->fvalue = ReadFloat();
        }
        else
            DeclareFuncCall(aword, 1);
        treenode = CreateLeaf(0x34, treenode);
        treenode->name = targetName;
        treenode->value = index;
        treenode->info = 1;
        treenode = CreateLeaf(0x49, treenode);
    }
    else if (type == 3)
    {
        treenode->next = CreateStringLeaf();
        while (treenode->next)
            treenode = treenode->next;
        treenode = CreateLeaf(0x34, treenode);
        treenode->name = targetName;
        treenode->value = index;
        treenode->info = 3;
        treenode = CreateLeaf(0x46, treenode);
    }
    else if (type == 7)
    {
        ReadWord(aword);
        DeclareFuncCall(aword, 7);
        treenode = CreateLeaf(0x43, treenode);
        treenode->name = targetName;
        treenode->info = 7;
        treenode = CreateLeaf(0x4A, treenode);
    }
}

void zCParser::DeclareAssignFunc(zSTRING& name)
{
    zSTRING targetName = name;
    int index = 0;
    ReadWord(aword);
    if (aword == "[")
    {
        index = ReadInt();
        zSTRING close = "]";
        Match(close);
    }
    else
        PrevWord();
    zSTRING assign = "=";
    Match(assign);
    ReadWord(aword);
    treenode = CreateLeaf(0x44, treenode);
    treenode->name = aword;
    treenode = CreateLeaf(0x34, treenode);
    treenode->name = targetName;
    treenode->value = index;
    treenode->info = 5;
    treenode = CreateLeaf(0x48, treenode);
}

void zCParser::DeclareFunc()
{
    int startLine = linec;
    int startPosition = line_start;
    int returnType = ReadFuncType();
    zSTRING name;
    ReadWord(name);
    zCPar_Symbol* function = new zCPar_Symbol;
    function->name = name;
    function->ele = 0x5000;
    function->SetFlag(1);
    if (!symtab.Insert(function))
    {
        zSTRING message = "Redefined: " + function->name;
        Error(message, 0);
    }
    in_func = function;
    treenode = CreateLeaf(0x5C, treenode);
    treenode->name = name;
    if (returnType)
        function->SetFlag(2);
    function->SetOffset(returnType);
    zSTRING open = "(";
    Match(open);
    ReadWord(aword);
    if (aword == "VAR")
    {
        params = 1;
        DeclareVar(0);
        params = 0;
        zSTRING close = ")";
        Match(close);
        ParseBlock();
    }
    else if (aword == ")")
        ParseBlock();
    else
    {
        zSTRING message = "Expected ')'";
        Error(message, 0);
        return;
    }
    function->SetLineData(startLine, linec - startLine + 1, startPosition,
        pc - pc_start - startPosition + 3);
    function->SetFileNr(file.numInArray - 1);
    in_func = NULL;
    in_funcnr = 0;
    treenode = CreateLeaf(0x5D, treenode);
}

void zCParser::DeclareReturn()
{
    if (!in_func)
    {
        zSTRING message = "Unexpected 'return'";
        Error(message, 0);
        return;
    }
    if (in_func->HasFlag(2))
    {
        if (in_func->GetOffset() == 2)
        {
            int token = GetNextToken();
            treenode->next = Parse_Expression(token, -1);
            if (treenode->next)
                treenode = zCPar_TreeNode::SeekEndTree(treenode->next);
            PrevWord();
        }
        else if (in_func->GetOffset() == 3)
        {
            treenode->next = CreateStringLeaf();
            while (treenode->next)
                treenode = treenode->next;
        }
    }
    treenode = CreateLeaf(0x3C, treenode);
}

void zCParser::DeclareFuncCall(zSTRING& name, int expectedReturn)
{
    zCPar_Symbol* function = symtab.GetSymbol(name);
    if (!function)
    {
        zSTRING message = "Undefined function: " + name;
        Error(message, 0);
        return;
    }
    if (expectedReturn > 0 && function->GetOffset() != expectedReturn)
    {
        zSTRING message = "Func does not return a";
        if (expectedReturn == 1)
            message = message + " float";
        else if (expectedReturn == 2)
            message = message + "n int";
        else if (expectedReturn == 3)
            message = message + " string";
        else if (expectedReturn == 7)
            message = message + "n instance";
        Error(message, 0);
        return;
    }
    int additional = IsInAdditionalInfo(name);
    int infoLine = linec;
    int infoPosition = line_start;
    zSTRING open = "(";
    Match(open);
    zCPar_Symbol* parameter = function;
    int parameters = function->ele & 0xFFF;
    for (int i = 0; i < parameters; ++i)
    {
        parameter = parameter->GetNext();
        switch ((parameter->ele >> 12) & 0xF)
        {
        case 1:
            treenode->next = CreateFloatLeaf();
            treenode = treenode->next;
            break;
        case 2:
        {
            int token = GetNextToken();
            treenode->next = Parse_Expression(token, -1);
            while (treenode->next)
                treenode = treenode->next;
            PrevWord();
            break;
        }
        case 3:
            treenode->next = CreateStringLeaf();
            while (treenode->next)
                treenode = treenode->next;
            break;
        case 5:
            ReadWord(aword);
            if (aword == "NOFUNC")
            {
                treenode = CreateLeaf(0x33, treenode);
                treenode->value = -1;
            }
            else
            {
                treenode = CreateLeaf(0x44, treenode);
                treenode->name = aword;
            }
            break;
        case 7:
        {
            ReadWord(aword);
            treenode = CreateLeaf(0x43, treenode);
            if (aword == "SELF" && in_func && (in_func->ele & 0xF000) == 0x7000)
            {
                treenode->name = in_func->name;
                treenode->value = GetBaseClass(in_func);
            }
            else
            {
                treenode->name = aword;
                treenode->value = GetBaseClass(parameter);
            }
            treenode->info = 7;
            break;
        }
        default:
        {
            zSTRING message = "Parameters of this type not currently supported.";
            Error(message, 0);
            return;
        }
        }
        if (i + 1 < parameters)
        {
            zSTRING comma = ",";
            Match(comma);
        }
    }
    zSTRING close = ")";
    Match(close);
    treenode = CreateLeaf(0x3D, treenode);
    treenode->name = function->name;
    if (additional)
        WriteAdditionalInfo(function->name, infoLine, infoPosition);
}

zCPar_Symbol* zCParser::SearchFuncWithStartAddress(int address)
{
    for (int i = 0; i < symtab.GetNumInList(); ++i)
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(i);
        int type = (symbol->ele >> 12) & 0xF;
        if (type != 5 && type != 6)
            continue;
        int position = 0;
        symbol->GetStackPos(position, 0);
        if (position == address)
            return symbol;
    }
    return NULL;
}

void zCParser::GetNextCommand(zSTRING& command)
{
    command = zSTRING((long)stack.GetPopPos()) + ".";
    int operation = stack.PopByte();
    if (operation <= 0x21)
    {
        zSTRING text;
        GetOperatorString(operation, text);
        command = command + "Op " + text;
        return;
    }
    switch (operation)
    {
    case 0x40:
        command = command + "Push " + zSTRING((long)stack.PopInt());
        break;
    case 0xF5:
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(stack.PopInt());
        int index = stack.PopByte();
        command = command + "Push " + symbol->name + "[" + zSTRING((long)index) + "]";
        break;
    }
    case 0x41:
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(stack.PopInt());
        command = command + "Push " + symbol->name + "                 ";
        int type = (symbol->ele >> 12) & 0xF;
        if (type == 2)
        {
            int value;
            symbol->GetValue(value, 0);
            command = command + "(" + zSTRING((long)value) + ")";
        }
        else if (type == 1)
        {
            float value;
            symbol->GetValue(value, 0);
            zSTRING text;
            text.Sprintf("%.9g", value);
            command = command + "(" + text + ")";
        }
        else if (type == 3)
        {
            zSTRING value;
            if (!symbol->HasFlag(4))
                symbol->GetValue(value, 0);
            else
                value = symbol->name;
            command = command + "(" + value + ")";
        }
        break;
    }
    case 0x3C:
        command = command + "Ret";
        break;
    case 0x3F:
        command = command + "Pop";
        break;
    case 0x3E:
    {
        command = command + "Call External ";
        zCPar_Symbol* symbol = GetSymbol(stack.PopInt());
        if (symbol)
            command = command + symbol->name;
        break;
    }
    case 0x3D:
    {
        command = command + "Call ";
        zCPar_Symbol* symbol = SearchFuncWithStartAddress(stack.PopInt());
        if (symbol)
            command = command + symbol->name;
        break;
    }
    case 0x46:
    case 0x47:
        command = command + "String =";
        break;
    case 0x48:
        command = command + "Func =";
        break;
    case 0x4B:
        command = command + "Jump " + zSTRING((long)stack.PopInt());
        break;
    case 0x4C:
        command = command + "Jumpf " + zSTRING((long)stack.PopInt());
        break;
    case 0x50:
    {
        zCPar_Symbol* symbol = symtab.GetSymbol(stack.PopInt());
        command = command + "Set In : " + symbol->name;
        break;
    }
    case 0x43:
    {
        command = command + "Push Inst: ";
        zCPar_Symbol* symbol = GetSymbol(stack.PopInt());
        if (symbol)
            command = command + symbol->name;
        break;
    }
    case 0x4A:
        command = command + "Instance =";
        break;
    case 0x49:
        command = command + "Float =";
        break;
    default:
        command = command + "??";
        break;
    }
}

void zCParser::ShowPCodeSpy(zSTRING& name)
{
    zCPar_Symbol* symbol = GetSymbol(name);
    if (!symbol)
        return;
    int position = 0;
    symbol->GetStackPos(position, 0);
    stack.SetPopPos(position);
    std::printf("U:PAR: * PSEUDO - CODE *");
    for (int i = 0; i < 40 && stack.GetPopPos() < stack.GetDynSize(); ++i)
    {
        zSTRING command;
        GetNextCommand(command);
        zerr.Message("U:PAR: " + command);
    }
}

void zCParser::ShowPCode(int position, zCView* view, int flags)
{
    int maxLines = 0x1FFF / view->FontY();
    view->ClrPrintwin();
    stack.SetPopPos(position);
    view->ClrPrintwin();
    zSTRING title = " ::\n";
    if (in_func)
        title = in_func->name + title;
    view->Printwin(title);
    for (int i = 0; i < maxLines && stack.GetPopPos() < stack.GetDynSize(); ++i)
    {
        zSTRING command;
        GetNextCommand(command);
        view->Printwin(command + "\n");
    }
    stack.SetPopPos(position);
}

void zCParser::DefineExternal(const zSTRING& name, int(*function)(), int returnType, ...)
{
    zCPar_Symbol* symbol = new zCPar_Symbol;
    symbol->name = name;
    symbol->name.Upper();
    symbol->ele = 0x5000;
    symbol->SetFlag(9);
    if (returnType)
        symbol->SetFlag(2);
    symbol->SetOffset(returnType);
    if (!symtab.Insert(symbol))
    {
        if (compiled)
        {
            zSTRING symbolName = symbol->name;
            delete symbol;
            symbol = GetSymbol(symbolName);
        }
        else
        {
            zSTRING message = "Redefined External . " + symbol->name;
            Error(message, 0);
            return;
        }
    }
    symbol->SetStackPos((int)function);
    if (compiled)
        return;
    va_list arguments;
    va_start(arguments, returnType);
    int type = va_arg(arguments, int);
    while (type)
    {
        zCPar_Symbol* parameter = new zCPar_Symbol;
        parameter->name = symbol->name + ".PAR" + zSTRING((long)(symbol->ele & 0xFFF));
        parameter->ele = type << 12;
        parameter->SetParent(symbol);
        symtab.Insert(parameter);
        symbol->ele = (symbol->ele & ~0xFFF) | ((symbol->ele + 1) & 0xFFF);
        type = va_arg(arguments, int);
    }
    va_end(arguments);
}

void zCParser::DefineExternalVar(const zSTRING& name, void* address, int type, int elements)
{
    zCPar_Symbol* symbol = new zCPar_Symbol;
    symbol->name = name;
    symbol->name.Upper();
    symbol->ele = (type << 12) | elements;
    symbol->SetFlag(9);
    symbol->ele |= 0x400000;
    if (!symtab.Insert(symbol))
    {
        if (compiled)
        {
            zSTRING symbolName = symbol->name;
            delete symbol;
            symbol = GetSymbol(symbolName);
        }
        else
        {
            zSTRING message = "Redefined External Var. " + symbol->name;
            Error(message, 0);
            return;
        }
    }
    symbol->ele &= ~0x400000;
    symbol->SetDataAdr(address);
}

void zCParser::GetParameter(int& value)
{
    value = PopDataValue();
}

void zCParser::GetParameter(float& value)
{
    if (datastack.Pop() != 0x40)
        value = *(float*)datastack.Pop();
    else
        value = (float)datastack.Pop();
}

void zCParser::GetParameter(zSTRING& value)
{
    value = *PopString();
}

void* zCParser::GetInstance()
{
    int index = datastack.Pop();
    if (index <= 0)
        return NULL;
    return (void*)symtab.GetSymbol(index)->GetOffset();
}

void* zCParser::GetInstanceAndIndex(int& index)
{
    index = datastack.Pop();
    if (index <= 0)
        return NULL;
    return (void*)symtab.GetSymbol(index)->GetOffset();
}

void zCParser::SetReturn(int value)
{
    datastack.Push(value);
    datastack.Push(0x40);
}

void zCParser::SetReturn(float value)
{
    datastack.Push(*(int*)&value);
    datastack.Push(0x40);
}

void zCParser::SetReturn(void* value)
{
    datastack.Push(instance_help);
    zCPar_Symbol* symbol = symtab.GetSymbol(instance_help);
    if (symbol)
        symbol->SetOffset((int)value);
}

void zCParser::SetReturn(zSTRING& value)
{
    datastack.Push((int)&value);
    datastack.Push(0x42);
}

int zCParser::FindInstanceVar(zSTRING& name)
{
    int separator = name.Search(".");
    if (separator <= 0)
        return -1;
    zSTRING instanceName = name.Copied(0, separator);
    name.Delete(0, separator + 1);
    instance = FindIndex(instanceName);
    zCPar_Symbol* symbol = symtab.GetSymbol(instance);
    zCPar_Symbol* parent = NULL;
    if (symbol && (symbol->ele & 0xF000) == 0x7000)
    {
        parent = symbol->GetParent();
        if (parent && (parent->ele & 0xF000) == 0x6000)
            parent = parent->GetParent();
    }
    if (!symbol || (symbol->ele & 0xF000) != 0x7000 || !parent ||
        (parent->ele & 0xF000) != 0x4000)
        return -1;
    name = parent->name + "." + name;
    return symtab.GetIndex(name);
}

int zCParser::GetLastInstance()
{
    return instance;
}

int zCParser::FindIndex(zSTRING& name)
{
    int index = FindInstanceVar(name);
    if (index >= 0)
        return index;
    if (in_func)
    {
        index = symtab.GetIndex(in_func->name + "." + name);
        if (index >= 0)
            return index;
    }
    if (in_class)
    {
        index = symtab.GetIndex(in_class->name + "." + name);
        if (index >= 0)
            return index;
    }
    return symtab.GetIndex(name);
}

zCPar_StringTable::zCPar_StringTable(int size)
    : array(size)
{
}

zCPar_StringTable::~zCPar_StringTable()
{
    Clear();
}

void zCPar_StringTable::Clear()
{
    for (int i = 0; i < array.numInArray; ++i)
        delete array[i];
    array.numInArray = 0;
}

zSTRING* zCPar_StringTable::Insert(zSTRING& value)
{
    zSTRING* string = new zSTRING(value);
    array.InsertEnd(string);
    return string;
}

int zCPar_StringTable::GetLastEntry()
{
    return array.numInArray - 1;
}

zSTRING* zCPar_StringTable::GetString(int index)
{
    if (index > array.numInArray)
        return NULL;
    return array[index];
}

void zCPar_StringTable::Save(zSTRING& fileName)
{
    zFILE* output = zfactory->CreateZFile(fileName);
    output->Create();
    for (int i = 0; i < array.numInArray; ++i)
    {
        output->Write(*array[i]);
        output->Write(zSTRING("\n"));
    }
    output->Close();
    delete output;
}

void zCPar_StringTable::Load(zSTRING& fileName)
{
    zFILE* input = zfactory->CreateZFile(fileName);
    input->Open(0);
    if (input->Exists())
    {
        while (!input->Eof())
        {
            zSTRING value;
            input->Read(value);
            if (!input->Eof())
                Insert(value);
        }
        input->Close();
    }
    delete input;
}

void zCPar_StringTable::ShrinkToFit()
{
    int count = array.numInArray;
    if (count <= 0)
    {
        delete[] array.parray;
        array.parray = NULL;
        array.numAlloc = 0;
        array.numInArray = 0;
        return;
    }
    if (array.numAlloc <= count)
        return;
    zSTRING** strings = new zSTRING*[count];
    for (int i = 0; i < count; ++i)
        strings[i] = array[i];
    delete[] array.parray;
    array.parray = strings;
    array.numAlloc = count;
}

void zCPar_StringTable::Show()
{
    for (int i = 0; i < array.numInArray; ++i)
        std::printf("%s\n", array[i]->ToChar());
}

zCPar_Symbol* zCPar_Symbol::instance_sym = NULL;
void* zCPar_Symbol::instance_adr = NULL;

void zCPar_Symbol::SetUseInstance(zCPar_Symbol* symbol)
{
    instance_sym = NULL;
    instance_adr = NULL;
    if (!symbol)
        return;
    int type = (symbol->ele >> 12) & 0xF;
    if ((type == 6 || type == 7) && symbol->offset)
    {
        instance_adr = (void*)symbol->offset;
        instance_sym = symbol;
    }
    else if (type != 6 && type != 7)
        zerr.Warning("U:PAR:Object is not an instance :" + symbol->name);
}

void zCPar_Symbol::SetUseInstanceAdr(void* address)
{
    instance_adr = address;
}

void* zCPar_Symbol::GetUseInstance()
{
    return instance_sym;
}

zCPar_Symbol::zCPar_Symbol()
    : next(NULL), adr(NULL), offset(0), ele(0), filenr(0), line(0), line_anz(0), pos_beg(0),
      pos_anz(0), parent(NULL)
{
}

zCPar_Symbol::~zCPar_Symbol()
{
    if (adr && !(ele & 0x40000) && (ele & 0x400000))
    {
        int type = (ele >> 12) & 0xF;
        int count = ele & 0xFFF;
        if ((type == 1 || type == 2) && count > 1)
            delete[] adr;
        else if (type == 3)
        {
            if (count <= 1)
                delete stringdata;
            else
                delete[] stringdata;
        }
    }
    adr = NULL;
    parent = NULL;
    next = NULL;
}

void zCPar_Symbol::AllocSpace()
{
    if (ele & 0x400000)
        return;
    int type = (ele >> 12) & 0xF;
    int count = ele & 0xFFF;
    if (HasFlag(4))
    {
        if (!count)
            zerr.Message("U: SYM: Allocspace with no elements !");
        int size = 0;
        if (type == 1 || type == 2 || type == 5)
            size = count * 4;
        else if (type == 3)
            size = count * 20;
        if (parent)
        {
            offset = parent->offset;
            parent->offset += size;
        }
    }
    else
    {
        switch (type)
        {
        case 1:
            if (count > 1)
                floatdata = new float[count]();
            else
                adr = NULL;
            break;
        case 2:
            if (count > 1)
                intdata = new int[count]();
            else
                adr = NULL;
            break;
        case 3:
            if (count > 1)
                stringdata = new zSTRING[count];
            else if (count == 1)
                stringdata = new zSTRING;
            else
                adr = NULL;
            break;
        case 4:
            offset = 0;
            break;
        case 5:
        case 6:
        case 7:
            adr = NULL;
            break;
        }
    }
    ele |= 0x400000;
}

void* zCPar_Symbol::GetDataAdr(int index)
{
    int type = (ele >> 12) & 0xF;
    int count = ele & 0xFFF;
    if (!HasFlag(4))
    {
        switch (type)
        {
        case 1:
            return count <= 1 ? (void*)&single_floatdata : (void*)(floatdata + index);
        case 2:
            return count <= 1 ? (void*)&single_intdata : (void*)(intdata + index);
        case 3:
            return stringdata + index;
        case 4:
            return adr;
        case 5:
        case 6:
        case 7:
            return &single_intdata;
        default:
            return adr;
        }
    }
    if (!instance_adr)
    {
        static float emptyFloat = 0.0f;
        static int emptyInt = 0;
        static zSTRING emptyString;
        if (zerr.filter_level >= 2)
            zerr.Warning("U:PAR: Adressing an empty Instance : " + name);
        if (type == 1)
            return &emptyFloat;
        if (type == 3)
            return &emptyString;
        return &emptyInt;
    }
    return (char*)instance_adr + offset + index * (type == 3 ? 20 : 4);
}

void zCPar_Symbol::SetDataAdr(void* address)
{
    adr = address;
}

void zCPar_Symbol::SetFlag(int flag)
{
    ele |= (flag & 0x3F) << 16;
}

int zCPar_Symbol::HasFlag(int flag)
{
    return ((ele >> 16) & flag & 0x3F) != 0;
}

void zCPar_Symbol::SetParent(zCPar_Symbol* parent)
{
    this->parent = parent;
}

zCPar_Symbol* zCPar_Symbol::GetParent()
{
    return parent;
}

void* zCPar_Symbol::GetInstanceAdr()
{
    return (void*)offset;
}

zCPar_Symbol* zCPar_Symbol::GetNext()
{
    return next;
}

int zCPar_Symbol::ArrayCheck(int index)
{
    return index >= 0 && index < (ele & 0xFFF);
}

void zCPar_Symbol::GetStackPos(int& position, int offset)
{
    position = single_intdata;
}

void zCPar_Symbol::SetStackPos(int position, int offset)
{
    single_intdata = position;
}

void zCPar_Symbol::SetValue(int value, int index)
{
    if ((ele & 0xFFF) <= 1)
        single_intdata = value;
    else
        intdata[index] = value;
}

void zCPar_Symbol::SetValue(float value, int index)
{
    if ((ele & 0xFFF) <= 1)
        single_floatdata = value;
    else
        floatdata[index] = value;
}

void zCPar_Symbol::SetValue(zSTRING& value, int index)
{
    stringdata[index] = value;
}

void zCPar_Symbol::GetValue(int& value, int index)
{
    if ((ele & 0xFFF) <= 1)
        value = single_intdata;
    else
        value = intdata[index];
}

void zCPar_Symbol::GetValue(float& value, int index)
{
    if ((ele & 0xFFF) <= 1)
        value = single_floatdata;
    else
        value = floatdata[index];
}

void zCPar_Symbol::GetValue(zSTRING& value, int index)
{
    value = stringdata[index];
}

void zCPar_Symbol::SetLineData(int line, int lineCount, int position, int positionLength)
{
    this->line = line & 0x7FFFF;
    line_anz = lineCount & 0x7FFFF;
    pos_beg = position & 0xFFFFFF;
    pos_anz = positionLength & 0xFFFFFF;
}

void zCPar_Symbol::GetLineData(int& line, int& lineCount, int& position, int& positionLength)
{
    line = this->line & 0x7FFFF;
    lineCount = line_anz & 0x7FFFF;
    position = pos_beg & 0xFFFFFF;
    positionLength = pos_anz & 0xFFFFFF;
}

void zCPar_Symbol::SetFileNr(int fileNumber)
{
    filenr = fileNumber & 0x7FFFF;
}

static void ParSaveSymbol(zCPar_Symbol* symbol, zFILE* file, int full)
{
    int hasName = symbol->name.Length() > 0;
    file->Write(&hasName, sizeof(hasName));
    if (hasName)
        file->Write(symbol->name + "\n");
    file->Write(&symbol->offset, 0x1C);
    if (full ? symbol->HasFlag(4) : !symbol->HasFlag(1))
        return;
    int type = (symbol->ele >> 12) & 0xF;
    int count = symbol->ele & 0xFFF;
    if (type == 1 || type == 2)
    {
        if (count > 1)
            file->Write(symbol->adr, count * 4);
        else if (count == 1)
            file->Write(&symbol->single_intdata, 4);
    }
    else if (type == 3)
    {
        for (int i = 0; i < count; ++i)
            file->Write(symbol->stringdata[i] + "\n");
    }
    else if (type == 4)
        file->Write(&symbol->adr, 4);
    else if (full && type >= 5 && type <= 7)
        file->Write(&symbol->single_intdata, 4);
}

void zCPar_Symbol::Save(zFILE* file)
{
    ParSaveSymbol(this, file, 0);
}

void zCPar_Symbol::SaveFull(zFILE* file)
{
    ParSaveSymbol(this, file, 1);
}

static void ParLoadSymbol(zCPar_Symbol* symbol, zFILE* file, int full)
{
    int hasName = 0;
    file->Read(&hasName, sizeof(hasName));
    if (hasName)
        file->Read(symbol->name);
    file->Read(&symbol->offset, 0x1C);
    symbol->next = NULL;
    symbol->ele &= ~0x400000;
    if ((symbol->ele & 0xF000) != 0x4000)
        symbol->AllocSpace();
    if (full ? symbol->HasFlag(4) : !symbol->HasFlag(1))
        return;
    int type = (symbol->ele >> 12) & 0xF;
    int count = symbol->ele & 0xFFF;
    if (type == 1 || type == 2)
    {
        if (count > 1)
            file->Read(symbol->adr, count * 4);
        else if (count == 1)
            file->Read(&symbol->single_intdata, 4);
    }
    else if (type == 3)
    {
        for (int i = 0; i < count; ++i)
            file->Read(symbol->stringdata[i]);
    }
    else if (type == 4)
        file->Read(&symbol->adr, 4);
    else if (full && type >= 5 && type <= 7)
        file->Read(&symbol->single_intdata, 4);
}

void zCPar_Symbol::LoadFull(zFILE* file)
{
    ParLoadSymbol(this, file, 1);
}

void zCPar_Symbol::Load(zFILE* file)
{
    ParLoadSymbol(this, file, 0);
}

void zCPar_Symbol::SetOffset(int offset)
{
    this->offset = offset;
}

int zCPar_Symbol::GetOffset()
{
    return offset;
}

void zCPar_Symbol::SetClassOffset(int offset)
{
    single_intdata = offset;
}

int zCPar_Symbol::GetClassOffset()
{
    return single_intdata;
}

zCPar_SymbolTable* zCPar_SymbolTable::cur_table = NULL;

zCPar_SymbolTable::zCPar_SymbolTable(int size)
    : preAllocatedSymbols(NULL), nextPreAllocated(-1), table(size), tablesort(size), lastsym(NULL),
      firstsym(NULL)
{
    tablesort.Compare = Compare;
}

zCPar_SymbolTable::~zCPar_SymbolTable()
{
    Clear();
}

void zCPar_SymbolTable::Clear()
{
    if (preAllocatedSymbols)
    {
        delete[] preAllocatedSymbols;
        preAllocatedSymbols = NULL;
        nextPreAllocated = -1;
    }
    else
    {
        for (int i = 0; i < table.numInArray; ++i)
            delete table[i];
    }
    table.numInArray = 0;
    tablesort.numInArray = 0;
    firstsym = NULL;
    lastsym = NULL;
}

void zCPar_SymbolTable::PreAlloc(int count)
{
    preAllocatedSymbols = new zCPar_Symbol[count];
    nextPreAllocated = count - 1;
}

int zCPar_SymbolTable::Compare(const void* first, const void* second)
{
    zCPar_Symbol* firstSymbol = cur_table->table[*(const int*)first];
    zCPar_Symbol* secondSymbol = cur_table->table[*(const int*)second];
    if (firstSymbol->name == secondSymbol->name)
        return 0;
    return firstSymbol->name < secondSymbol->name ? -1 : 1;
}

void zCPar_SymbolTable::Save(zFILE* file)
{
    int count = table.numInArray;
    file->Write(&count, sizeof(count));
    file->Write(tablesort.array, count * sizeof(int));
    for (int i = 0; i < count; ++i)
    {
        zCPar_Symbol* symbol = table[i];
        symbol->SaveFull(file);
        int parentIndex = symbol->parent ? GetIndex(symbol->parent) : -1;
        file->Write(&parentIndex, sizeof(parentIndex));
    }
}

struct zCParserExternals {
    zSTRING name;
    void* address;
};

template <class T>
struct zList {
    T* data;
    zList* next;

    int IsIn(T* value)
    {
        for (zList* entry = next; entry; entry = entry->next)
        {
            if (entry->data == value)
                return 1;
        }
        return 0;
    }
};

void zCPar_SymbolTable::Load(zFILE* file)
{
    zList<zCParserExternals>* externals = NULL;
    for (int i = 0; i < table.numInArray; ++i)
    {
        zCPar_Symbol* symbol = table[i];
        if (symbol->HasFlag(8))
        {
            zCParserExternals* data = new zCParserExternals;
            data->name = symbol->name;
            data->address = (symbol->ele & 0xF000) == 0x5000 ?
                (void*)symbol->single_intdata : symbol->GetDataAdr(0);
            zList<zCParserExternals>* external = new zList<zCParserExternals>;
            external->data = data;
            external->next = externals;
            externals = external;
        }
        if (!preAllocatedSymbols)
            delete symbol;
    }
    if (preAllocatedSymbols)
    {
        delete[] preAllocatedSymbols;
        preAllocatedSymbols = NULL;
        nextPreAllocated = -1;
    }
    table.numInArray = 0;
    tablesort.numInArray = 0;
    int count = 0;
    file->Read(&count, sizeof(count));
    if (tablesort.numAlloc < count)
        tablesort.AllocDelta(count - tablesort.numAlloc);
    table.AllocAbs(count);
    tablesort.numInArray = count;
    file->Read(tablesort.array, count * sizeof(int));
    PreAlloc(count);
    for (int i = 0; i < count; ++i)
    {
        zCPar_Symbol* symbol = preAllocatedSymbols + nextPreAllocated--;
        symbol->LoadFull(file);
        int parentIndex = -1;
        file->Read(&parentIndex, sizeof(parentIndex));
        if (parentIndex >= 0)
            symbol->parent = table[parentIndex];
        table.InsertEnd(symbol);
    }
    while (externals)
    {
        zList<zCParserExternals>* external = externals;
        externals = external->next;
        zCPar_Symbol* symbol = GetSymbol(external->data->name);
        if (!symbol || !symbol->HasFlag(8))
            zerr.Warning("U: PAR: External " + external->data->name + " not found.");
        else
            symbol->single_intdata = (int)external->data->address;
        delete external->data;
        delete external;
    }
}

int zCPar_SymbolTable::Search(const zSTRING& name, int first, int last)
{
    cur_table = this;
    while (first <= last)
    {
        int middle = (first + last) / 2;
        int index = tablesort.array[middle];
        zCPar_Symbol* symbol = table[index];
        if (symbol->name == name)
            return index;
        if (symbol->name < name)
            first = middle + 1;
        else
            last = middle - 1;
    }
    return -1;
}

int zCPar_SymbolTable::IsIn(zCPar_Symbol* symbol)
{
    return GetIndex(symbol) >= 0;
}

int zCPar_SymbolTable::IsIn(const zSTRING& name)
{
    return GetIndex(name) >= 0;
}

int zCPar_SymbolTable::GetIndex(zCPar_Symbol* symbol)
{
    return Search(symbol->name, 0, table.numInArray - 1);
}

int zCPar_SymbolTable::GetIndex(const zSTRING& name)
{
    return Search(name, 0, table.numInArray - 1);
}

int zCPar_SymbolTable::GetIndex(const zSTRING& name, int start)
{
    int last = start + 1 + (table[start]->ele & 0xFFF);
    return Search(name, start + 1, last);
}

zCPar_Symbol* zCPar_SymbolTable::GetSymbol(const zSTRING& name)
{
    int index = GetIndex(name);
    return GetSymbol(index);
}

zCPar_Symbol* zCPar_SymbolTable::GetSymbol(int index)
{
    if (index >= 0 && index < table.numInArray)
        return table[index];
    return NULL;
}

int zCPar_SymbolTable::Insert(zCPar_Symbol* symbol)
{
    if (GetIndex(symbol->name) >= 0)
        return 0;
    InsertEnd(symbol);
    return 1;
}

void zCPar_SymbolTable::InsertEnd(zCPar_Symbol* symbol)
{
    table.InsertEnd(symbol);
    int index = table.numInArray - 1;
    cur_table = this;
    tablesort.InsertSort(index);
    symbol->AllocSpace();
    if (!firstsym)
        firstsym = symbol;
    if (lastsym)
        lastsym->next = symbol;
    lastsym = symbol;
}

void zCPar_SymbolTable::ShrinkToFit()
{
    int count = table.numInArray;
    if (count <= 0)
    {
        delete[] table.parray;
        table.parray = NULL;
        table.numAlloc = 0;
        table.numInArray = 0;
    }
    else if (table.numAlloc > count)
    {
        zCPar_Symbol** symbols = new zCPar_Symbol*[count];
        for (int i = 0; i < count; ++i)
            symbols[i] = table[i];
        delete[] table.parray;
        table.parray = symbols;
        table.numAlloc = count;
    }
    count = tablesort.numInArray;
    if (count <= 0)
    {
        delete[] tablesort.array;
        tablesort.array = NULL;
        tablesort.numAlloc = 0;
        tablesort.numInArray = 0;
    }
    else if (tablesort.numAlloc > count)
    {
        int* indices = new int[count];
        for (int i = 0; i < count; ++i)
            indices[i] = tablesort.array[i];
        delete[] tablesort.array;
        tablesort.array = indices;
        tablesort.numAlloc = count;
    }
}

void zCPar_SymbolTable::SetSize(int size)
{
    if (size <= 0)
        return;
    table.AllocDelta(size);
    tablesort.AllocDelta(size);
}

int zCPar_SymbolTable::GetNumInList()
{
    return table.numInArray;
}

zCPar_Symbol* zCPar_SymbolTable::GetLastSymbol()
{
    return lastsym;
}

zCPar_Symbol* zCPar_SymbolTable::GetFirstSymbol()
{
    return firstsym;
}

void zCPar_SymbolTable::Show()
{
    for (int i = 0; i < table.numInArray; ++i)
    {
        zCPar_Symbol* symbol = table[i];
        zSTRING text = symbol->name;
        int count = symbol->ele & 0xFFF;
        if (count > 1)
            text = text + "[" + zSTRING((long)count) + "]";
        if (symbol->HasFlag(1))
        {
            text = text + (count <= 1 ? " = " : " = { ");
            for (int element = 0; element < count; ++element)
            {
                int type = (symbol->ele >> 12) & 0xF;
                if (type == 1)
                {
                    float value;
                    symbol->GetValue(value, element);
                    zSTRING number;
                    number.Sprintf("%.9g", value);
                    text = text + number;
                }
                else if (type == 2)
                {
                    int value;
                    symbol->GetValue(value, element);
                    text = text + zSTRING((long)value);
                }
                else if (type == 3)
                {
                    zSTRING value;
                    symbol->GetValue(value, element);
                    text = text + "\"" + value + "\"";
                }
                if (element + 1 < count)
                    text = text + ",";
            }
            if (count > 1)
                text = text + " }";
        }
        text = text + ";";
        std::printf("%d. %s\n", i, text.ToChar());
    }
}

zCPar_Stack::zCPar_Stack(int size)
    : stack(new unsigned char[size]), stackptr(stack), stacklast(stack), stacksize(size)
{
}

zCPar_Stack::~zCPar_Stack()
{
    delete[] stack;
    stack = NULL;
    stackptr = NULL;
    stacklast = NULL;
}

int zCPar_Stack::GetPopPos()
{
    return stackptr - stack;
}

void zCPar_Stack::SetPopPos(int position)
{
    stackptr = stack + position;
}

int zCPar_Stack::GetDynSize()
{
    return stacklast - stack;
}

int zCPar_Stack::GetStatSize()
{
    return stacksize;
}

void zCPar_Stack::PushByte(unsigned char value)
{
    CheckOverflow(1);
    *stacklast++ = value;
}

void zCPar_Stack::PushWord(unsigned short value)
{
    CheckOverflow(2);
    *(unsigned short*)stacklast = value;
    stacklast += 2;
}

void zCPar_Stack::PushInt(int value)
{
    CheckOverflow(4);
    *(int*)stacklast = value;
    stacklast += 4;
}

void zCPar_Stack::PushString(zSTRING& value)
{
    CheckOverflow(value.Length() + 2);
    *stacklast++ = (unsigned char)(value.Length() + 1);
    std::memcpy(stacklast, value.ToChar(), value.Length() + 1);
    stacklast += value.Length() + 1;
}

unsigned char zCPar_Stack::PopByte()
{
    return *stackptr++;
}

unsigned short zCPar_Stack::PopWord()
{
    unsigned short value = *(unsigned short*)stackptr;
    stackptr += 2;
    return value;
}

int zCPar_Stack::PopInt()
{
    int value = *(int*)stackptr;
    stackptr += 4;
    return value;
}

void zCPar_Stack::PopString(zSTRING& value)
{
    int length = *stackptr++;
    value = (char*)stackptr;
    stackptr += length;
}

void zCPar_Stack::CheckOverflow(int size)
{
    if (stacklast - stack + size <= stacksize)
        return;
    int newSize = stacksize * 2;
    unsigned char* newStack = new unsigned char[newSize];
    std::memcpy(newStack, stack, stacksize);
    stackptr = newStack + (stackptr - stack);
    stacklast = newStack + (stacklast - stack);
    delete[] stack;
    stack = newStack;
    stacksize = newSize;
}

void zCPar_Stack::Set(int position, int value)
{
    stack[position] = (unsigned char)value;
}

int zCPar_Stack::ReachedEnd()
{
    return stackptr == stacklast;
}

void zCPar_Stack::Clear()
{
    stacklast = stack;
}

void zCPar_Stack::Save(zFILE* file)
{
    int size = GetDynSize();
    file->Write(&size, sizeof(size));
    file->Write(stack, size);
}

void zCPar_Stack::Load(zFILE* file)
{
    int size = 0;
    file->Read(&size, sizeof(size));
    delete[] stack;
    stack = new unsigned char[size];
    file->Read(stack, size);
    stackptr = stack;
    stacklast = stack + size;
    stacksize = size;
}

zCPar_DataStack::zCPar_DataStack()
    : sptr(0)
{
    std::memset(stack, 0, sizeof(stack));
}

zCPar_DataStack::~zCPar_DataStack()
{
}

void zCPar_DataStack::Push(int value)
{
    if (sptr >= 2048)
        zerr.Message("C: zCPar_DataStack :: Push(): stack overflow!!!");
    stack[sptr++] = value;
}

int zCPar_DataStack::Pop()
{
    if (sptr <= 0)
        return 0;
    return stack[--sptr];
}

float zCPar_DataStack::PopFloat()
{
    if (sptr <= 0)
    {
        zerr.Warning("C: zCPar_DataStack :: PopFloat(): stack underflow - return 0.0f");
        return 0.0f;
    }
    int value = stack[--sptr];
    return *(float*)&value;
}

int zCPar_DataStack::IsEmpty()
{
    return sptr == 0;
}

void zCPar_DataStack::Clear()
{
    sptr = 0;
}
