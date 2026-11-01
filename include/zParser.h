#ifndef ZPARSER_H
#define ZPARSER_H

#include "zFile.h"

class zCPar_Symbol;

class zCPar_SymbolTable {
public:

    zCPar_SymbolTable(int size = 0);
    ~zCPar_SymbolTable();
    void PreAlloc(int count);
    void SetSize(int size);
    void Clear();
    void Save(zFILE* file);
    void Load(zFILE* file);
    void Show();
    int IsIn(zCPar_Symbol* symbol);
    int IsIn(const zSTRING& name);
    int GetIndex(const zSTRING& name);
    int GetIndex(const zSTRING& name, int start);
    int GetIndex(zCPar_Symbol* symbol);
    int Search(const zSTRING& name, int first, int last);
    int GetNumInList();
    int Insert(zCPar_Symbol* symbol);
    void InsertEnd(zCPar_Symbol* symbol);
    void ShrinkToFit();
    zCPar_Symbol* GetSymbol(const zSTRING& name);
    zCPar_Symbol* GetSymbol(int index);
    zCPar_Symbol* GetLastSymbol();
    zCPar_Symbol* GetFirstSymbol();
    static int Compare(const void* first, const void* second);

    zCPar_Symbol* preAllocatedSymbols;
    int nextPreAllocated;
    zCArray<zCPar_Symbol*> table;
    zCArraySort<int> tablesort;
    zCPar_Symbol* lastsym;
    zCPar_Symbol* firstsym;

    static zCPar_SymbolTable* cur_table;
};

class zCPar_StringTable {
public:

    zCPar_StringTable(int size = 0);
    ~zCPar_StringTable();
    void Clear();
    zSTRING* Insert(zSTRING& value);
    int GetLastEntry();
    zSTRING* GetString(int index);
    void Save(zSTRING& fileName);
    void Load(zSTRING& fileName);
    void ShrinkToFit();
    void Show();

    zCArray<zSTRING*> array;
};

class zCPar_Stack {
public:

    zCPar_Stack(int size = 0);
    ~zCPar_Stack();
    void Clear();
    void Save(zFILE* file);
    void Load(zFILE* file);
    void CheckOverflow(int size);
    void PushByte(unsigned char value);
    void PushWord(unsigned short value);
    void PushInt(int value);
    void PushString(zSTRING& value);
    unsigned char PopByte();
    unsigned short PopWord();
    int PopInt();
    void PopString(zSTRING& value);
    int GetDynSize();
    int GetStatSize();
    void Set(int position, int value);
    void SetPopPos(int position);
    int GetPopPos();
    int ReachedEnd();

    unsigned char* stack;
    unsigned char* stackptr;
    unsigned char* stacklast;
    int stacksize;
};

class zCPar_DataStack {
public:

    zCPar_DataStack();
    ~zCPar_DataStack();
    void Clear();
    int Pop();
    float PopFloat();
    int IsEmpty();
    void Push(int value);

    int stack[2048];
    int sptr;
};

class zCPar_TreeNode {
public:

    zCPar_TreeNode();
    zCPar_TreeNode(int token, int value);
    zCPar_TreeNode(int token, zSTRING& name, int info);
    ~zCPar_TreeNode();
    void SetLeft(zCPar_TreeNode* node);
    void SetRight(zCPar_TreeNode* node);
    void SetNext(zCPar_TreeNode* node);
    static zCPar_TreeNode* SeekEndTree(zCPar_TreeNode* node);
    static zCPar_TreeNode* MakeDyadicNode(zCPar_TreeNode* left, int token, zCPar_TreeNode* right);
    static zCPar_TreeNode* MakeMonadicNode(int token, zCPar_TreeNode* left);

    zSTRING name;
    unsigned char token;
    union {
        int value;
        float fvalue;
    };
    int info;
    zCPar_TreeNode* left;
    zCPar_TreeNode* right;
    zCPar_TreeNode* next;
    int cNr;
};

class zCPar_File {
public:

    zCPar_File();
    ~zCPar_File();
    int IsNewerDate(zDATE& first, zDATE& second);
    int IsExactDate(zDATE& first, zDATE& second);
    void SetFileNumber(int number);
    int GetFileNumber();
    zSTRING GetName();
    void EnableTreeLoad(int enabled);
    int Load(zSTRING& fileName, zCPar_SymbolTable& symbols, zDATE& date);
    char* GetStartAddress();
    char* GetEndAddress();
    void Dispose();
    void SetTree(zCPar_TreeNode* tree);
    zCPar_TreeNode* GetTree();
    void SetLabelCount(int count);
    int GetLabelCount();
    void SaveTree(int fileNumber, zCPar_Symbol* symbol, zDATE& date);
    void SaveTreeNode(zFILE* file, zCPar_TreeNode* node);
    zCPar_TreeNode* LoadTreeNode(zFILE* file);
    int LoadTree(zFILE_STATS& stats, zCPar_SymbolTable& symbols);
    void DeleteTree();

    int fileid;
    zSTRING name;
    char* begin_adr;
    int size;
    int labels;
    int treeload;
    zCPar_TreeNode* tree;
};

#endif
