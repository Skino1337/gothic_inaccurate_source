#ifndef ZCONSOLE_H
#define ZCONSOLE_H

#include "zSession.h"

class zCParser;

const int zCON_MAX_EVAL = 15;

enum {
    zCON_TYPE_INT,
    zCON_TYPE_FLOAT,
    zCON_TYPE_STRING,
    zCON_TYPE_STRINGP
};

class zCConDat {
public:

    zCConDat(const zSTRING& hint, const zSTRING& name, int type, void* address, int elements);
    zCConDat(const zSTRING& hint, const zSTRING& name, int type);
    ~zCConDat();

    zSTRING hint;
    zSTRING name;
    int type;
    void* adr;
    int ele;
    int var;
    zCConDat* next;
};

template <class T>
class zList {
public:

    zList()
        : root(NULL), last(NULL), count(0) {}

    void InsertLast(T* value)
    {
        for (T* entry = root; entry; entry = entry->next)
        {
            if (entry == value)
                return;
        }
        value->next = NULL;
        if (last)
            last->next = value;
        else
            root = value;
        last = value;
        ++count;
    }

    void Remove(T* value)
    {
        T* previous = NULL;
        T* entry = root;
        while (entry && entry != value)
        {
            previous = entry;
            entry = entry->next;
        }
        if (!entry)
            return;
        if (previous)
            previous->next = entry->next;
        else
            root = entry->next;
        if (last == entry)
            last = previous;
        --count;
    }

    T* root;
    T* last;
    int count;
};

class zCConsole : public zCInputCallback {
public:

    zCConsole();
    zCConsole(int x, int y, const zSTRING& name);
    ~zCConsole();

    virtual int HandleEvent(int key);
    void SetAutoCompletion(int enabled);
    int HasAutoCompletion();
    void SetWorld(zCWorld* world);
    zCWorld* GetWorld();
    void SetParser(zCParser* parser);
    zCParser* GetParser();
    void AddChar(int character, int printable);
    void InsertVar(const zSTRING& name, const zSTRING& hint, int type, void* address, int elements);
    void Register(const zSTRING& command, const zSTRING& hint, int type);
    void Register(const zSTRING& command, const zSTRING& hint);
    int AutoCompletion(zSTRING& input);
    void SetFocus();
    void RemoveFocus();
    void Show();
    void Hide();
    void Toggle();
    int IsVisible();
    void AddSkip(int amount);
    void Update();
    void ShowInput();
    zCConDat* Get(const zSTRING& name);
    zCConDat* GetBestMatch(const zSTRING& name);
    void Eval(const zSTRING& input);
    int Evaluate(const zSTRING& input);
    void SetPos(int x, int y);
    void SetDim(int width, int height);
    void SetFlags(int flags);
    void AddEvalFunc(int(*evalFunc)(const zSTRING&, zSTRING&));
    void SetChangedFunc(void(*changedFunc)(const zSTRING&));
    void ShowHelp();
    void SaveInfoFile(const zSTRING& fileName);
    int EditInstance(const zSTRING& name, void* instance);
    int EditInstance(int index, void* instance);
    static int EditFunc(const zSTRING& input, zSTRING& message);

    zSTRING id;
    zSTRING instr;
    zSTRING savemsg;
    zCArray<zSTRING> lastcommand;
    int lastCommandPos;
    int px;
    int py;
    int lx;
    int ly;
    int showmax;
    int skip;
    int dynsize;
    int var;
    int autocomplete;
    zList<zCConDat> list;
    zCView* conview;
    int evalcount;
    int(*evalfunc[zCON_MAX_EVAL])(const zSTRING&, zSTRING&);
    void(*changedfunc)(const zSTRING&);
    zCWorld* world;
    zCParser* cparser;
    int edit_index;
    void* edit_adr;

    static zCList<zCConsole> active_consoles;
    static zCConsole* cur_console;
};

extern zCConsole zcon;

#endif
