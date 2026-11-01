#include "include/zObject.h"
#include "include/zString.h"
#include "include/zThread.h"
#include "include/zFile.h"
#include <windows.h>

void default_exit();
void ErrorBox(zSTRING text);

static unsigned int logMessageId;
static unsigned int logCommandId;
static int errorSystemState = -1;
static int errorQueueState = -1;
static zQUEUE<zSTRING> errorQueue;
static zCArray<zSTRING*> errorBlockStack;
static int errorObjectState = -1;
static int errorGlobalState = -1;
static zERROR zerrObject;
zERROR* zerr = &zerrObject;

zERROR::zERROR()
    : on_exit(default_exit), filter_flag(0), filter_level(-1), target(1), ack_type(3),
      log_file(NULL), indent_depth(0), spyHandle(NULL), spyMutex(NULL)
{
    filter_authors = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    spyMutex = new zCMutex;
    spyHandle = FindWindowA(NULL, "[zSpy]");
    if (spyHandle)
        SetForegroundWindow((HWND)spyHandle);
    logMessageId = RegisterWindowMessageA("WM_LOGMSG");
    logCommandId = RegisterWindowMessageA("WM_LOGCOMMAND");
}

zERROR::~zERROR()
{
    Separator("");
    if (log_file) {
        log_file->Close();
        delete log_file;
        log_file = NULL;
    }
    delete spyMutex;
    spyMutex = NULL;
}

void zERROR::Init(zSTRING parameters)
{
    parameters.Upper();
    const int logPosition = parameters.Search("-ZLOG:");
    if (logPosition >= 0) {
        const zSTRING options = parameters.Copied(logPosition + 6,
            parameters.Length() - logPosition - 6);
        const zSTRING targetOptions = options.PickWord(1, " ", " ");
        const zSTRING levelOptions = options.PickWord(2, " ", " ");
        const zSTRING authorOptions = options.PickWord(3, " ", " ");
        int newTarget = 0;
        if (targetOptions.Search("S") >= 0)
            newTarget |= 1;
        if (targetOptions.Search("F") >= 0)
            newTarget |= 2;
        SetTarget(newTarget);
        filter_level = levelOptions.IsEmpty() ? 0xFF : levelOptions.ToInt();
        if (authorOptions.IsEmpty())
            SetFilterAuthors("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        else
            filter_authors = authorOptions;
    }
    SearchForSpy();
    const ATOM startAtom = GlobalAddAtomA("START");
    SendMessageA((HWND)0xFFFF, logCommandId, 0, startAtom);
    while (GlobalDeleteAtom(startAtom))
        ;
    ack_type = 4;
    if (target > 0)
        ShowSpy(1);
    ack_type = 3;
}

void zERROR::SendToSpy(zSTRING& message)
{
    if (!spyHandle || !spyMutex)
        return;
    spyMutex->Lock(0xffffffff);
    SendMessageA((HWND)spyHandle, 0x4a, 0, (LPARAM)message.ToChar());
    spyMutex->Unlock();
}

int zERROR::Report(zERROR_LEVEL type, int id, const zSTRING& message, signed char level,
    unsigned int target, int line, char* file, char* author)
{
    return Report((zERROR_TYPE)type, id, message, level, target, line, file, author);
}

int zERROR::Report(zERROR_TYPE type, int id, const zSTRING& message, signed char level,
    unsigned int target, int line, char* file, char* author)
{
    if (filter_level < 0 && type != zERR_TYPE_FATAL)
        return id;
    if (type == zERR_TYPE_INFO && message.Length() >= 2 && message[1] == ':'
        && level <= filter_level && author && filter_authors.Search(author, 0) < 0)
        return id;
    const char* prefix = "Info: ";
    if (type == zERR_TYPE_WARN)
        prefix = "Warn: ";
    else if (type == zERR_TYPE_FAULT)
        prefix = "Fault:";
    else if (type == zERR_TYPE_FATAL)
        prefix = "Fatal:";
    zSTRING text = zSTRING(prefix) + message;
    if (id)
        text += zSTRING(id).Align(zSTR_RIGHT, 2, spaceChar);
    if (file) {
        zSTRING fileName = file;
        const int separator = fileName.SearchRev("\\", 1);
        if (separator >= 0)
            fileName.Delete(0, separator + 1);
        text += " .... " + fileName + ":" + zSTRING(line);
        if (author)
            text += " (" + zSTRING(author) + ")";
    }
    if (target & 1)
        SendToSpy(text);
    if ((target & 2) && log_file)
        log_file->Write(text + "\n");
    if (type >= ack_type) {
        if (zrenderer)
            zrenderer->Vid_SetScreenMode(2);
        ErrorBox(text);
    }
    if (type == zERR_TYPE_FATAL && on_exit)
        on_exit();
    if (target & 1)
        BlockBegin(file, line);
    if (target & 2)
        BlockEnd(file, line);
    return id;
}

void zERROR::BlockBegin(char* file, int line)
{
    if (indent_depth == 0xFF) {
        zerr->Warning("B: ERR: indent-depth out of bound (>255).");
        return;
    }
    errorBlockStack.InsertEnd(new zSTRING(file ? file : ""));
    ++indent_depth;
}

void zERROR::BlockEnd(char* file, int line)
{
    if (indent_depth <= 0 || errorBlockStack.numInArray <= 0) {
        zSTRING text;
        text.Sprintf("B: ERR: BLOCK-END unexpected: %s line %d", file, line);
        zerr->Warning(text);
        return;
    }
    const int index = errorBlockStack.numInArray - 1;
    zSTRING* context = errorBlockStack.parray[index];
    if (*context == (file ? file : "")) {
        errorBlockStack.RemoveOrderIndex(index);
        delete context;
        --indent_depth;
        return;
    }
    zSTRING text = "B: ERR: BLOCK-END in " + *context + " missed: at BLOCK-END in "
        + (file ? file : "") + " line " + zSTRING(line);
    zerr->Warning(text);
    errorBlockStack.RemoveOrderIndex(index);
    delete context;
}

void zERROR::Separator(const zSTRING& text)
{
    if (filter_level <= 0)
        return;
    zSTRING separator = "---------------";
    SendToSpy(separator);
    if (text.Length())
        Report(zERR_TYPE_INFO, 0, zSTRING("          ") + text, 1, 0, 0, NULL, NULL);
}

void zERROR::ShowSpy(int show)
{
    if (filter_level <= 0)
        return;
    const char* command = show ? "SHOW" : "HIDE";
    const ATOM atom = GlobalAddAtomA(command);
    SendMessageA((HWND)0xffff, logCommandId, 0, atom);
    while (GlobalDeleteAtom(atom))
        ;
}

int zERROR::Message(const zSTRING& message)
{
    if (filter_level < 5)
        return 0;
    return Report(zERR_TYPE_INFO, 0, message, 5, 0, 0x1d1, NULL, NULL);
}

int zERROR::Warning(const zSTRING& message)
{
    return Report(zERR_TYPE_WARN, 0, message, 0, 0, 0x1da, NULL, NULL);
}

int zERROR::Fatal(const zSTRING& message)
{
    return Report(zERR_TYPE_FATAL, 0, message, -1, 0, 0x1e3, NULL, NULL);
}

int zERROR::Fault(const zSTRING& message)
{
    return Report(zERR_TYPE_FAULT, 0, message, 0, 0, 0x1e8, NULL, NULL);
}

void zSTRING::operator delete(void* memory)
{
    s_MemPool.Free(memory);
}

void zERROR::SetFilterFlags(unsigned long flags)
{
    filter_flag = flags;
    if (filter_level >= 0)
        Report(zERR_TYPE_INFO, 0, zSTRING("B: ERR: SetFilterFlags   ") + GetFilterFlagDescription(),
            0, 0, 0, NULL, NULL);
}

void zERROR::SetFilterAuthors(zSTRING authors)
{
    filter_authors = authors;
    if (filter_level >= 0)
        Report(zERR_TYPE_INFO, 0, zSTRING("B: ERR: SetFilterAuthors ") + filter_authors,
            0, 0, 0, NULL, NULL);
}

void zERROR::SetFilterLevel(int level)
{
    filter_level = (signed char)level;
    if (filter_level >= 10)
        Report(zERR_TYPE_INFO, 0, zSTRING("B: ERR: SetFilterLevel   ") + zSTRING(level),
            10, 0, 0, NULL, NULL);
}

void zERROR::SetTarget(int value)
{
    target = value;
    if ((target & 2) && !log_file) {
        char userName[196];
        DWORD size = sizeof(userName);
        GetUserNameA(userName, &size);
        zSTRING name = userName;
        name.Lower();
        log_file = new zFILE_FILE(zSTRING("c:\\errorlog_by_") + name + ".txt");
        log_file->Create();
    }
    if (filter_level >= 0)
        Report(zERR_TYPE_INFO, 0, zSTRING("B: ERR: SetTarget        ") + GetTargetDescription(),
            0, 0, 0, NULL, NULL);
}

zSTRING zERROR::GetTargetDescription()
{
    zSTRING result = "[";
    if (target & 1)
        result += "SPY ";
    if (target & 2)
        result += "FILE ";
    result += "]";
    return result;
}

zSTRING zERROR::GetFilterFlagDescription()
{
    return zSTRING("[") + zSTRING((int)filter_flag) + "]";
}

zSTRING zERROR::GetFilterAuthors()
{
    return filter_authors;
}

bool zERROR::SearchForSpy()
{
    spyHandle = FindWindowA(NULL, "[zSpy]");
    if (spyHandle && filter_level >= 5)
        Report(zERR_TYPE_INFO, 0, "B: ZERR: Spy found.", 5, 0, 0x1d1, NULL, NULL);
    return spyHandle != NULL;
}
