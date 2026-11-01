#ifndef ZOBJECT_H
#define ZOBJECT_H

class zCObject;
class zSTRING;
class zFILE;
class zCCSPlayer;
class zCCSManager;
class zCPlayerInfo;
template <class T>
class zCTree;
struct zCLivingObjectsState;

int zArraySortDefaultCompare(const void* ele1, const void* ele2);

class zCBuffer {
public:

    void SetMode(int mode);
};

class zCArchiver : public zCObject {
public:

    struct zTChunkRecord {
        zTChunkRecord();
        ~zTChunkRecord();
        zTChunkRecord& operator=(const zTChunkRecord& other);

        unsigned long  startPos;
        unsigned long  size;
        unsigned short version;
        unsigned long  objectIndex;
        zSTRING        name;
        zSTRING        className;
    };

    virtual ~zCArchiver();
    virtual zCClassDef* _GetClassDef() const;
    virtual zCObject* ReadObjectAccount(const char* file, int line, zCObject* object);
    virtual zCObject* ReadObjectAccount(const char* file, int line, const char* comment,
        zCObject* object);
    void      WriteObject(zCObject* object);
    zCObject* ReadObject(zCObject* object);
    void      WriteChunkStart(const char* name, unsigned short version);
    void      WriteChunkEnd();
    void      WriteGroupBegin(const char* name);
    void      WriteGroupEnd();
    void      WriteColor(unsigned long& value);
    void      ReadColor(unsigned long& value);
    int       ReadChunkStart(zSTRING& chunkName, unsigned short& version);
    void      SkipOpenChunk();
    unsigned short GetCurrentChunkVersion();
    zFILE*    GetFile() const;
    void      WriteString(const zSTRING& value);
    void      WriteString(const zSTRING& name, const zSTRING& value);
    zSTRING   ReadString();
    zSTRING   ReadString(const zSTRING& name);
    int       ReadInt(const zSTRING& name);
    void      WriteInt(const zSTRING& name, int value);
    void      WriteRaw(const void* data, long size);
    void      ReadRaw(void* data, long size);
    int       InBinaryMode() const;
    void      WriteVec3(const struct zVEC3& value);
    void      WriteByte(int value);
    void      WriteBool(int value);
    void      WriteFloat(float value);
    void      WriteInt(int value);
    void      WriteEnum(int value);
    int       ReadEnum();
    int       InSaveGame() const;
    int       InProperties() const;
    struct zVEC3 ReadVec3();
    int       ReadByte();
    int       ReadBool();
    int       ReadInt();
    float     ReadFloat();
    zCBuffer* GetBuffer();
    void      Close();
    int       Release();

    int medium;
    int mode;

    static zCClassDef* classDef;
    static zSTRING stringEOL;
};

class zCArchiverFactory {
public:

    zCArchiver* CreateArchiverWrite(zCBuffer* buffer, int archiveMode, int flags, int saveGame);
    zCArchiver* CreateArchiverWrite(const zSTRING& fileName, int archiveMode, int flags,
        int saveGame);
    zCArchiver* CreateArchiverRead(zCBuffer* buffer, int mode);
    zCArchiver* CreateArchiverRead(const zSTRING& fileName, int mode);
};

extern zCArchiverFactory* zarcFactory;

class zCChecksum {
public:

    static unsigned long GetBufferCRC32(const char* buffer, unsigned long length, unsigned long seed);
    static void BeginChecksum(unsigned long* checksum);
    static void CalcBufferChecksum(unsigned long* checksum, const void* buffer, unsigned long size);
    static unsigned long EndChecksum(unsigned long* checksum);
};

enum zERROR_TYPE {
    zERR_TYPE_OK,
    zERR_TYPE_INFO,
    zERR_TYPE_WARN,
    zERR_TYPE_FAULT,
    zERR_TYPE_FATAL
};

typedef int zERROR_LEVEL;
class zCMutex;

class zERROR {
public:

    zERROR();
    virtual ~zERROR();
    void Init(zSTRING parameters);
    void SendToSpy(zSTRING& message);
    int  Report(zERROR_LEVEL type, int id, const zSTRING& message, signed char level,
        unsigned int target, int line, char* file, char* author);
    int  Report(zERROR_TYPE type, int id, const zSTRING& message, signed char level,
        unsigned int target, int line, char* file, char* author);
    void BlockBegin(char* file, int line);
    void BlockEnd(char* file, int line);
    int  Message(const zSTRING& message);
    int  Warning(const zSTRING& message);
    int  Fatal(const zSTRING& message);
    int  Fault(const zSTRING& message);
    void Separator(const zSTRING& text);
    void ShowSpy(int show);
    bool SearchForSpy();
    void SetFilterFlags(unsigned long flags);
    void SetFilterAuthors(zSTRING authors);
    void SetFilterLevel(int level);
    void SetTarget(int value);
    zSTRING GetTargetDescription();
    zSTRING GetFilterFlagDescription();
    zSTRING GetFilterAuthors();
    int  GetFilterLevel();

    void (*on_exit)();
    zSTRING filter_authors;
    unsigned int filter_flag;
    signed char filter_level;
    int target;
    int ack_type;
    zFILE* log_file;
    unsigned char indent_depth;
    void* spyHandle;
    zCMutex* spyMutex;
};

extern zERROR* zerr;

template <class T>
struct zCArrayAdapt {
    ~zCArrayAdapt()
    {
    }

    unsigned long SetArray(void* memory, int count)
    {
        array = static_cast<T*>(memory);
        numInArray = count;
        return count * sizeof(T);
    }

    int numInArray;
    T* array;
};

template <class T>
class zCArray {
public:

    zCArray(int count = 0)
        : parray(0), numAlloc(count), numInArray(0)
    {
        if (count > 0)
            parray = new T[count];
    }

    zCArray(const zCArray& source)
        : parray(0), numAlloc(0), numInArray(0)
    {
        AllocAbs(source.numInArray);
        numInArray = source.numInArray;
        for (int i = 0; i < numInArray; ++i)
            parray[i] = source.parray[i];
    }

    ~zCArray()
    {
        delete[] parray;
        parray = 0;
    }

    void operator=(const zCArray& source)
    {
        numInArray = 0;
        AllocAbs(source.numInArray);
        numInArray = source.numInArray;
        for (int i = 0; i < numInArray; ++i)
            parray[i] = source.parray[i];
    }

    T& operator[](int index)
    {
        return parray[index];
    }

    const T& operator[](int index) const
    {
        return parray[index];
    }

    void AllocAbs(int count)
    {
        if (numAlloc >= count)
            return;
        AllocDelta(count - numAlloc);
    }

    void AllocDelta(int delta)
    {
        if (delta <= 0)
            return;
        T* newArray = new T[numAlloc + delta];
        for (int i = 0; i < numInArray; ++i)
            newArray[i] = parray[i];
        delete[] parray;
        parray = newArray;
        numAlloc += delta;
    }

    void InsertEnd(const T& value)
    {
        if (numAlloc < numInArray + 1)
            AllocDelta(numAlloc >= 16 ? numAlloc / 2 : 16);
        parray[numInArray++] = value;
    }

    void Insert(const T& value)
    {
        InsertEnd(value);
    }

    void InsertAtPos(const T& value, int position)
    {
        if (numAlloc < numInArray + 1)
            AllocDelta(numAlloc >= 16 ? numAlloc / 2 : 16);
        for (int i = numInArray; i > position; --i)
            parray[i] = parray[i - 1];
        parray[position] = value;
        ++numInArray;
    }

    void Remove(const T& value)
    {
        for (int i = 0; i < numInArray; ++i) {
            if (parray[i] == value) {
                --numInArray;
                if (i != numInArray)
                    parray[i] = parray[numInArray];
                return;
            }
        }
    }

    void RemoveIndex(int index)
    {
        if (index < 0 || index >= numInArray)
            return;
        --numInArray;
        if (index != numInArray)
            parray[index] = parray[numInArray];
    }

    void RemoveOrderIndex(int index)
    {
        if (index < 0 || index >= numInArray)
            return;
        for (int i = index; i < numInArray - 1; ++i)
            parray[i] = parray[i + 1];
        --numInArray;
    }

    void RemoveOrder(const T& value)
    {
        for (int i = 0; i < numInArray; ++i) {
            if (parray[i] == value) {
                RemoveOrderIndex(i);
                return;
            }
        }
    }

    void ShrinkToFit()
    {
        if (numInArray <= 0)
        {
            delete[] parray;
            parray = 0;
            numAlloc = 0;
            numInArray = 0;
            return;
        }
        if (numAlloc > numInArray)
        {
            T* newArray = new T[numInArray];
            for (int i = 0; i < numInArray; ++i)
                newArray[i] = parray[i];
            delete[] parray;
            parray = newArray;
            numAlloc = numInArray;
        }
    }

    int IsInList(const T& value) const
    {
        for (int i = 0; i < numInArray; ++i) {
            if (parray[i] == value)
                return 1;
        }
        return 0;
    }

    int Search(const T& value) const
    {
        for (int i = 0; i < numInArray; ++i) {
            if (parray[i] == value)
                return i;
        }
        return -1;
    }

    void DeleteList()
    {
        delete[] parray;
        parray = 0;
        numAlloc = 0;
        numInArray = 0;
    }

    T& GetSafe(int index) const
    {
        if (index < 0)
            index = 0;
        else if (index >= numInArray)
            index = numInArray - 1;
        return parray[index];
    }

    T*      parray;
    int     numAlloc;
    int     numInArray;
};

template <class T>
unsigned long CalcArraySize(zCArray<T> array)
{
    return array.numAlloc * sizeof(T);
}

template <class T>
void SetAndCopyArrayAdapt(unsigned char*& data, zCArrayAdapt<T>& destination,
    const zCArray<T>& source)
{
    destination.array = reinterpret_cast<T*>(data);
    destination.numInArray = source.numInArray;
    data += source.numInArray * sizeof(T);
    for (int i = 0; i < source.numInArray; ++i)
        destination.array[i] = source.parray[i];
}

template <class T>
class zCArraySort {
public:

    zCArraySort(int count = 0)
        : array(0), numAlloc(count), numInArray(0), Compare(zArraySortDefaultCompare)
    {
        if (count > 0)
            array = new T[count];
    }

    ~zCArraySort()
    {
        delete[] array;
        array = 0;
    }

    void AllocDelta(int delta)
    {
        if (delta <= 0)
            return;
        T* newArray = new T[numAlloc + delta];
        for (int i = 0; i < numInArray; ++i)
            newArray[i] = array[i];
        delete[] array;
        array = newArray;
        numAlloc += delta;
    }

    void InsertEnd(const T& value)
    {
        if (numAlloc < numInArray + 1)
            AllocDelta(numAlloc >= 16 ? numAlloc / 2 : 16);
        array[numInArray++] = value;
    }

    void DeleteList()
    {
        delete[] array;
        array = 0;
        numAlloc = 0;
        numInArray = 0;
    }

    void InsertSort(const T& value)
    {
        if (numAlloc < numInArray + 1)
            AllocDelta(numAlloc >= 16 ? numAlloc / 2 : 16);
        int index = numInArray;
        while (index > 0 && Compare && Compare(&value, &array[index - 1]) < 0) {
            array[index] = array[index - 1];
            --index;
        }
        array[index] = value;
        ++numInArray;
    }

    int Search(const T& value) const
    {
        int first = 0;
        int last = numInArray - 1;
        while (first <= last) {
            const int middle = (first + last) / 2;
            const int comparison = Compare(&value, &array[middle]);
            if (comparison == 0)
                return middle;
            if (comparison < 0)
                last = middle - 1;
            else
                first = middle + 1;
        }
        return -1;
    }

    void RemoveOrderIndex(int index)
    {
        if (index < 0 || index >= numInArray)
            return;
        for (int i = index; i < numInArray - 1; ++i)
            array[i] = array[i + 1];
        --numInArray;
    }

    void RemoveOrder(const T& value)
    {
        const int index = Search(value);
        if (index >= 0)
            RemoveOrderIndex(index);
    }

    T& GetSafe(int index) const
    {
        if (index < 0)
            index = 0;
        else if (index >= numInArray)
            index = numInArray - 1;
        return array[index];
    }

    void QuickSort()
    {
        for (int i = 0; i < numInArray; ++i) {
            for (int j = i + 1; j < numInArray; ++j) {
                if (Compare && Compare(&array[j], &array[i]) < 0) {
                    const T temporary = array[i];
                    array[i] = array[j];
                    array[j] = temporary;
                }
            }
        }
    }

    int GetNumInList() const
    {
        return numInArray;
    }

    T*      array;
    int     numAlloc;
    int     numInArray;
    int(*Compare)(const void* ele1, const void* ele2);
};

class zCClassDef {
public:

    zCClassDef();
    zCClassDef(const zSTRING& className, const zSTRING& baseClassName,
        zCObject*(*createNewInstance)(), unsigned long classFlags, unsigned long classSize,
        unsigned short archiveVersion, const zSTRING& scriptClassName);
    ~zCClassDef();

    void               Init();
    void               ReportLeaks();
    zCObject*          CreateNewInstance() const;
    unsigned short     GetClassID();
    static zCObject*   CreateNewInstanceID(unsigned short classID);
    void               OverwriteCreateNewInstance(zCClassDef* other);
    void               ResetCreateNewInstance();
    static int         CompareClassDefNames(const void* ele1, const void* ele2);
    static zCClassDef* GetClassDef(const zSTRING& className);
    static zCClassDef* GetClassDefTypeInsensitive(const zSTRING& className);
    unsigned long      GetHashIndex(const zSTRING& name);
    unsigned long      GetHashIndex(zCObject* object);
    void               InsertHashTable(zCObject* object);
    void               RemoveHashTable(zCObject* object);
    zCObject*          SearchHashTable(const zSTRING& name);
    float              CalcHashTableSpread();
    static void        ObjectCreated(zCObject* object, zCClassDef* classDef);
    static void        ObjectCreated(zCObject* object, zCClassDef* classDef,
        const char* file, int line);
    static void        ObjectDeleted(zCObject* object, zCClassDef* classDef);
    static void        ObjectCreated(zCObject* objects, int bytes, zCClassDef* classDef);
    static void        ObjectDeleted(zCObject* objects, int bytes, zCClassDef* classDef);
    static void        GetLivingObjectsState(zCLivingObjectsState& state);
    static void        CompareLivingObjectsState(const zCLivingObjectsState& state);
    static void        EndStartup();

    static zCArraySort<zCClassDef*>* classDefList;
    static zCClassDef* classDefSearchDummy;
    static int         startupFinished;

    zSTRING    className;
    zSTRING    baseClassName;
    zSTRING    scriptClassName;
    zCClassDef* baseClassDef;
    zCObject*(*createNewInstance)();
    zCObject*(*createNewInstanceBackup)();
    unsigned long classFlags;
    unsigned long classSize;
    int numLivingObjects;
    int numCtorCalled;
    zCObject** hashTable;
    zCArray<zCObject*> objectList;
    unsigned short archiveVersion;
    unsigned short archiveVersionSum;
};

class zCObject {
public:

    zCObject();

    void                AddRef();
    int                 Release();
    zCObject*           CreateCopy();
    const zSTRING&      GetObjectName() const;
    int                 SetObjectName(const zSTRING& name);
    static zCObject*    CreateNewInstance(const zSTRING& className);
    static void         GetMemStats(int& numObjects, int& numBytes, zCClassDef* baseClass);
    static int          CheckInheritance(const zCClassDef* baseClass, const zCClassDef* derivedClass);
    static int          CheckInheritance(const zCObject* baseObject, const zCObject* derivedObject);
    static int          CheckInheritance(const zSTRING& baseClass, const zSTRING& derivedClass);
    virtual zCClassDef* _GetClassDef() const;
    virtual void        Archive(zCArchiver& archiver);
    virtual void        Unarchive(zCArchiver& archiver);
    virtual             ~zCObject();

    static zCClassDef* classDef;
    static void        PrintClassHierarchy(int level, zCClassDef* baseClass,
        int numObjects, int numBytes);

    int           refCtr;
    unsigned short hashIndex;
    zCObject*     hashNext;
    zSTRING       objectName;
};

struct zCLivingObjectsState {
    zCArray<int> numList;
};

class zFILE;
class zCVob;
class zCWorld;

extern const zSTRING CS_PLAYER_ROLENAME;
extern const zSTRING CS_MAIN_ROLENAME;

class zCCSRole : public zCObject {
public:

    zCCSRole();
    zCCSRole(zSTRING& name);
    virtual ~zCCSRole();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void SetRoleVob(zCVob* vob);
    zCVob* GetRoleVob();

    zSTRING roleName;
    int mustBeAlive;
    int rFlag;
    zCVob* roleVob;

    static zCClassDef* classDef;
};

class zCEventManager;

class zCCSPoolItem : public zCObject {
public:

    zCCSPoolItem();
    zCCSPoolItem(zCCSCutsceneContext* context);
    virtual ~zCCSPoolItem();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void PrintDebugInfo();
    void Init();
    static void operator delete(void* memory);
    int IsAllowedToPlay();
    void SetRunBehaviour(int behaviour, int value);
    int GetRunBehaviour();
    int GetRunBehaviour(int& value);
    void TestedDeactivation();
    int HasFlags(int flags) const;
    void SetFlags(int flags);
    void ClrFlags(int flags);

    int deactivated;
    int runBehaviour;
    int runBehaviourValue;
    int flags;
    int numPlayed;
    int playCounter;
    zSTRING poolName;

    static zCClassDef* classDef;
};

class zCEventMessage;
class zCCSBlock;
class zCCSBlockPosition;

class zCCSBlockBase : public zCObject {
public:

    zCCSBlockBase() {}
    virtual ~zCCSBlockBase();
    virtual zCEventMessage* GetCommand();
    virtual void InsertBlock(zCCSBlockBase* block, int index);
    virtual zCCSBlockBase* GetChild(int index);
    virtual int NewBlock(int first, int second);
    virtual int GetNumOfChilds();
    virtual int GetRoleNumOfChild(int index);
    virtual void BlockCorrection();
    virtual void BlockAsFocus(int index);
    virtual zSTRING GetRoleName();
    virtual void SetRoleName(zSTRING& name);
    virtual void DeleteChild(int index);
    virtual zCCSBlockPosition* GetChildPos(int index);
    virtual void OrganizeRoles(zCArray<zCVob*>& vobs, zCArray<zCCSRole*>& roles);
    virtual void Play(const zCArray<zCEventManager*>& managers);
    virtual void Play(const zCArray<zCEventManager*>& managers, float first, float second);
    virtual void Play(zCEventManager* manager);
    virtual void Play(zCEventManager* manager, float first, float second);
    virtual float GetMinTime();
    virtual int IsFinished(const zCArray<zCEventManager*>& managers);
    virtual int IsFinished(zCEventManager* manager);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    static zCClassDef* classDef;
};

class zCCSAtomicBlock : public zCCSBlockBase {
public:

    zCCSAtomicBlock();
    virtual ~zCCSAtomicBlock();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void OrganizeRoles(zCArray<zCVob*>& vobs, zCArray<zCCSRole*>& roles);
    virtual void Play(const zCArray<zCEventManager*>& managers);
    virtual void Play(const zCArray<zCEventManager*>& managers, float first, float second);
    virtual void Play(zCEventManager* manager);
    virtual void Play(zCEventManager* manager, float first, float second);
    virtual float GetMinTime();
    virtual int IsFinished(const zCArray<zCEventManager*>& managers);
    virtual int IsFinished(zCEventManager* manager);
    virtual zCEventMessage* GetCommand();

    zCVob* roleVob;
    zCEventMessage* commandRef;
    zCEventMessage* command;
    int commandState;

    static zCClassDef* classDef;
};

class zCCSProps : public zCObject {
public:

    zCCSProps();
    virtual ~zCCSProps();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int AcceptRole(zCCSRole* role, zCVob* vob);
    virtual int CheckRole(zCCSRole* role, zCVob* vob);
    virtual int GetCheckRoleResult();
    virtual int CheckDistance(float distance);
    virtual int CheckRange(float range);
    virtual void PrintDebugInfo();
    zSTRING GetName();
    zSTRING GetScriptFuncOnStop();

    zSTRING name;
    int csLoop;
    int globalCutscene;
    float distance;
    int hasToBeTriggerd;
    float range;
    int ignore;
    int numLockedBlocks;
    zSTRING stageName;
    zSTRING scriptFuncOnStop;
    int runBehaviour;
    int runBehaviourValue;

    static void operator delete(void* memory);
    static zCClassDef* classDef;
};

class zCCSBlockPosition {
public:

    zCCSBlockPosition() : position(0.0f), block(NULL) {}
    zCCSBlockPosition(zCCSBlockBase* block);
    zCCSBlockPosition(float position, zCCSBlockBase* block);
    virtual ~zCCSBlockPosition();
    zCCSBlockPosition& operator=(const zCCSBlockPosition& other);

    float position;
    zCCSBlockBase* block;
};

class zCCSBlock : public zCCSBlockBase {
public:

    zCCSBlock() {}
    virtual ~zCCSBlock();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual zCCSBlockBase* GetChild(int index);
    virtual zCCSBlockPosition* GetChildPos(int index);
    virtual int GetNumOfChilds();
    virtual void DeleteChild(int index);
    virtual zSTRING GetRoleName();
    virtual void SetRoleName(zSTRING& name);
    virtual void OrganizeRoles(zCArray<zCVob*>& vobs, zCArray<zCCSRole*>& roles);
    virtual void Play(const zCArray<zCEventManager*>& managers);
    virtual void Play(const zCArray<zCEventManager*>& managers, float first, float second);
    virtual void Play(zCEventManager* manager);
    virtual void Play(zCEventManager* manager, float first, float second);
    virtual float GetMinTime();
    virtual int IsFinished(const zCArray<zCEventManager*>& managers);
    virtual int IsFinished(zCEventManager* manager);
    virtual void InsertBlock(zCCSBlockBase* block, int index);
    virtual int NewBlock(int first, int second);
    virtual void BlockAsFocus(int index);
    virtual void BlockCorrection();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCArray<zCCSBlockPosition> blocks;
    zSTRING roleName;

    static zCClassDef* classDef;
};

class zCCSSyncBlock : public zCCSBlock {
public:

    zCCSSyncBlock() {}
    virtual ~zCCSSyncBlock();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsFinished(const zCArray<zCEventManager*>& managers);
    virtual void Play(zCEventManager* manager);
    virtual void Play(const zCArray<zCEventManager*>& managers);
    virtual void Play(const zCArray<zCEventManager*>& managers, float first, float second);
    virtual int GetRoleNumOfChild(int index);
    virtual void InsertBlock(zCCSBlockBase* block, int index);
    virtual int NewBlock(int first, int second);
    virtual void BlockAsFocus(int index);
    virtual void DeleteChild(int index);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void AssignerCorrection(int index);

    zCArray<int> roleAss;

    static zCClassDef* classDef;
};

struct zTCSBlockPosition {
    ~zTCSBlockPosition() {}

    zCArray<zCCSBlockBase*> flatData;
    int index;
    int numInList;
};

class zCCutscene : public zCCSBlock {
public:

    zCCutscene();
    virtual ~zCCutscene();
    virtual zCClassDef* _GetClassDef() const;
    virtual int IsCutscene();
    zCCSProps* GetProperties();
    virtual void SetOwnerPlayer(zCCSPlayer* player);
    int IsMember(zCVob* vob);
    void SetRoleListForDebugging(zCArray<zCCSRole*>& roles);
    void SaveRolesVobs(zCBuffer& buffer);
    void RestoreRoleVobs(zCBuffer& buffer);
    int CheckMainRole(zCVob* vob);
    int CheckPlayState();
    virtual zCVob* GetStage();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int StartPlaying();
    virtual void Reset();
    virtual void ClearCutscene();
    virtual void StopRole(zCVob* vob);
    zCEventManager* SolveEventManager(int index);
    virtual void Stop();
    virtual void Interrupt();
    virtual int ResumeActBlock();
    void ResumeAtBlock(int index);
    virtual int PlaySub(float first, float second);
    virtual int IsFinished();
    virtual int NewBlock(int first, int second);
    virtual void NewCS(zCCSManager* manager);
    void NewRole(zSTRING& name);
    void DeleteRole(int index);
    void RegisterAdditionalVob(zCTree<zCVob>* node);
    virtual void PrepareBlock();
    virtual zCVob* LoadStage();
    virtual zCVob* CatchPlayer(zCVob* vob);
    virtual int AddMainRole(zCVob* vob);
    virtual void AddRoleOU(zCVob* vob);
    virtual void RemoveRole(zCVob* vob);
    virtual int CSCheck(int& reaction);
    virtual void PrintDebugInfo(const zSTRING& prefix);
    zCVob* FirstRoleUsed();
    virtual int CheckRoles(int& reaction);
    int GetLastCheckResult(zSTRING& text);
    int TryToCancel();
    zTCSBlockPosition GetFirstBlockPosition();
    zCEventMessage* GetNextMessage(zTCSBlockPosition& position);
    zCCSBlockBase* GetNextBlock(zTCSBlockPosition& position);
    void SerializeCutscene(zCCutscene* cutscene, zTCSBlockPosition& position);
    void SerializeBlock(zCCSBlockBase* block, zTCSBlockPosition& position);
    int GetActualBlockNum() const;

    int lastCheckResult;
    zCArray<zCCSRole*> roleList;
    zCArray<zCEventManager*> roleEvMList;
    zCArray<zCVob*> roleVobList;
    zCVob* mainRoleVob;
    zCCSProps* properties;
    zCCSPlayer* ownerPlayer;
    int actualBlockNum;
    zCCSBlockBase* actualBlock;

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    static zCClassDef* classDef;
};

class zCCSCutsceneContext : public zCCutscene {
public:

    enum zTCSPlayResult {
        CS_PLAY_STARTEDPLAYING,
        CS_PLAY_STOPPED,
        CS_PLAY_INTERRUPTED,
        CS_PLAY_RESUMED,
        CS_PLAY_CONTINUED,
        CS_PLAY_DISABLED,
        CS_PLAY_STARTFAILED,
        CS_PLAY_RESUMEFAILED,
        CS_PLAY_TOOFAR,
        CS_PLAY_MAXREACHED,
        CS_PLAY_NOTTRIGGERED,
        CS_PLAY_CHECKFAILED
    };

    zCCSCutsceneContext();
    zCCSCutsceneContext(zSTRING& name);
    zCCSCutsceneContext(zCCSBlock* block, int reference, int outputUnit);
    virtual ~zCCSCutsceneContext();
    void UseThisSubBlock(zCCSBlock* block);
    zSTRING GetCSName() const;
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int StartPlaying();
    virtual void Stop();
    virtual void Interrupt();
    virtual void ForcedInterrupt();
    virtual int Play(zVEC3 position);
    virtual int IsCutscene();
    virtual int IsOutputUnit();
    void HandleTrigger();
    void HandleUntrigger();
    float GetLastTimePoint();
    float GetLastTimeStamp();
    zTCSPlayResult GetLastPlayResult(zSTRING& text);
    int AreU(const zSTRING& name);
    int AreU(int reference);
    int IsForcedInterrupted();
    virtual int IsPlaying();
    virtual int IsFinished();
    int ReplayCutscene();
    virtual void SetPoolItem(zCCSPoolItem* item);
    virtual void PrepareBlock();
    virtual zCVob* GetStage();
    virtual void SetStage(zCVob* vob);
    virtual zCVob* CatchPlayer(zCVob* vob);
    void ForcedResume();
    virtual int CheckRoleReactions();
    virtual int AddMainRole(zCVob* vob);
    virtual void PrintDebugInfo(const zSTRING& prefix);
    static int DisablePlayCutscenes(int disabled);

    int lastPlayResult;
    int allowedToPlay;
    zCVob* alternativeStageVob;
    int wasTriggered;
    int isCutscene;
    int loaded;
    int reference;
    zCVob* stage;
    zCCSPoolItem* poolItem;
    float lastTimeStamp;
    float lastTimePoint;

    static zCClassDef* classDef;
    static int playDisabled;
};

class zCNetVobControl;

class zCEventManager : public zCObject {
public:

    zCEventManager();
    zCEventManager(zCVob* vob);
    virtual ~zCEventManager();
    static zCObject* _CreateNewInstance();
    static void DoFrameActivity();
    virtual zCClassDef* _GetClassDef() const;
    virtual zCNetVobControl* GetNetVobControl(int create);
    void KillMessages();
    zCCutscene* GetCutscene();
    virtual void SetCutscene(zCCutscene* cutscene);
    virtual int GetCutsceneMode();
    virtual void OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void ProcessMessageList();
    virtual void SendMessageToHost(zCEventMessage* message, zCVob* host, zCVob* sender);
    virtual void InsertInList(zCEventMessage* message);
    virtual void RemoveFromList(zCEventMessage* message);
    virtual void Delete(zCEventMessage* message);
    virtual void SetActive(int active);
    virtual void Clear();
    void ShowMessageCommunication(zCVob* sender, zCVob* host);
    static void SetShowMessageCommunication(int show);
    static int GetShowMessageCommunication();
    void Print_db(const zSTRING& text, zCVob* vob);
    virtual void OnTouch(zCVob* other);
    virtual void OnUntouch(zCVob* other);
    virtual void OnTouchLevel();
    virtual void OnTrigger(zCVob* other, zCVob* instigator);
    virtual void OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
        const zVEC3& hitLocation);
    virtual void ShowList(int x, int y);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    zCEventMessage* GetActiveMessage();
    zCEventMessage* GetEventMessage(int index);
    int GetNumMessages();
    int IsRunning();
    int IsRunning(zCEventMessage* message);
    int IsEmpty(int onlyNonOverlay = 0);

    int cleared;
    int active;
    zCCutscene* cutscene;
    zCArray<zCEventMessage*> messageList;
    zCVob* hostVob;

    static zCClassDef* classDef;
    static void operator delete(void* memory);
    static int disableEventManagers;
    static zCArray<zCEventManager*> activeEM;
    static int showMsgCommunication;
};

class zCNetVobControl : public zCObject {
public:

    zCNetVobControl();
    zCNetVobControl(zCVob* vob) { Init(vob); }
    virtual ~zCNetVobControl();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    static int HandleNetMessage(zCNetMessage* message, const unsigned short& messageType,
        zCWorld* world);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void Init(zCVob* vob);
    virtual int IsLocalChained();
    virtual int IsLocalControlled();
    virtual int IsPlayerVob();
    virtual void SetLocalChained(int value);
    virtual void SetCtrlPlayer(zCPlayerInfo* player);
    virtual void CopyCtrlState(zCVob* vob);
    virtual int Process();
    virtual int HandOverToPlayer(zCPlayerInfo* player);
    virtual zCNetMessage* CreateNetMessage(const unsigned short& messageType);

    zCVob* hostVob;
    zCPlayerInfo* ctrlPlayer;
    int isLocalChained;

    static zCClassDef* classDef;
};

class zCWay;

class zCWaypoint : public zCObject {
public:

    zCWaypoint();
    virtual ~zCWaypoint();
    virtual zCClassDef* _GetClassDef() const;
    void     Init(zCVob* vob);
    void     Init(zVEC3& position);
    void     Init(float x, float y, float z);
    void     SetName(const zSTRING& name);
    zSTRING  GetName() const;
    zCVob*   GetVob() const;
    zVEC3    GetPositionWorld();
    void     UpdatePositionWorld();
    void     Free();
    zCWay*   HasWay(zCWaypoint* waypoint);
    void     InsertWay(zCWay* way);
    void     RemoveWay(zCWay* way);
    int      GetNumberOfWays();
    zCList<zCWay>& GetWayList();
    void     SetWaypointVob(zCVob* vob);
    void     CorrectHeight(zCWorld* world);
    void     CalcProperties(zCWorld* world);
    void     Draw();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int CanBeUsed(const zCVob* vob);

    int routeCtr;
    int curCost;
    int estCost;
    int score;
    int curList;
    zCWay* parent;
    int waterDepth;
    int underWater;
    zVEC3 pos;
    zVEC3 dir;
    zSTRING name;
    zCVob* wpvob;
    zCList<zCWay> wayList;

    static zCClassDef* classDef;
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
};

class zCWay {
public:

    zCWay();
    zCWay(zCWaypoint* left, zCWaypoint* right);
    virtual ~zCWay();
    virtual void Init(zCWaypoint* left, zCWaypoint* right);
    void Free();
    zCWaypoint* GetGoalWaypoint(zCWaypoint* waypoint);
    void EstimateCost();
    float GetLength();
    zVEC3 GetDistancePos(float distance, zCWaypoint* start);
    virtual float GetChasm();
    virtual int CanJump();
    virtual int CanBeUsed(const zCVob* vob);
    virtual int IsObjectOnWay(const zCVob* vob);
    float GetJumpDistance();
    void Draw(zCCamera* camera);
    zVEC3 GetFloor(zCWorld* world, const zVEC3& position);
    virtual void CalcProperties(zCWorld* world);

    int cost;
    int usedCtr;
    float chasmDepth;
    int chasm;
    int jump;
    zCWaypoint* left;
    zCWaypoint* right;
};

class zCSession {
public:

    zCCSManager* GetCutsceneManager();
};

class zCCSLib;

class zCCSManager : public zCObject {
public:

    zCCSManager();
    virtual ~zCCSManager();
    virtual zCClassDef* _GetClassDef() const;
    virtual zCEventMessage* CreateMessage(int command);
    virtual zCEventMessage* CreateOuMessage();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCCSPlayer* CreateCutscenePlayer(zCWorld* world);
    virtual void RemoveCutscenePlayer(zCCSPlayer* player);
    virtual void RemovePlayerFromList(zCCSPlayer* player);
    int LibValidateOU(int index);
    int LibValidateOU(zSTRING& name);
    void PoolClrFlags(const zSTRING& name, int flags);
    void AddCommand(const zSTRING& command);
    virtual void InsertPlayerInList(zCCSPlayer* player);
    zCCSBlock* LibGet(int index);
    virtual zSTRING LibGetSvmModuleName(int index);
    virtual int LibIsSvmModuleRunning(int index);
    virtual int LibSvmModuleStart(int index);
    virtual int LibSvmModuleStop(int index);
    virtual zCCSCutsceneContext* InitiateCSwithContext(zCCSCutsceneContext* context);
    virtual zCCSCutsceneContext* InitiateOUwithContext(zCCSCutsceneContext* context);
    virtual zCCSCutsceneContext* CreateNewCutscene(zSTRING& name);
    virtual zCCSCutsceneContext* CreateNewOutputUnit(int unit);
    virtual zCCSPoolItem* PoolFindItem(const zSTRING& name);
    virtual void PoolInsertItem(zCCSPoolItem* item);
    int IsDeactivated(zSTRING& name);
    void PoolCountStart(zSTRING& name);
    void PoolCountStop(zSTRING& name);
    int PoolHasFlags(zSTRING& name, int flags);
    void PoolSetFlags(zSTRING& name, int flags);
    int PoolNumPlayed(zSTRING& name);
    int PoolAllowedToStart(zSTRING& name);
    void PoolResetAll();
    void PoolResetByHour();
    void PoolResetByDay();
    int GetNumOfShortCom();
    zSTRING GetShortCom(int index);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void LibLoad(int format);
    virtual void LibCheckLoaded(int format);
    virtual zCCSBlock* LibGetFirst();
    virtual zCCSBlock* LibGetNext();
    virtual zCCSBlock* LibGetFirst(int& index);
    virtual zCCSBlock* LibGetNext(int& index);
    virtual int LibAddOU(zCCSBlock* block);
    virtual void LibDelOU(int index);
    virtual void LibNullOU(int index);
    virtual void LibStore(int format);
    virtual void LibForceToLoad();
    virtual int LibIsLoaded();
    virtual zCCSProps* CreateProperties();

    zCArray<zSTRING> libSvmModuleList;
    zCArray<zSTRING> commandStrings;
    zCArray<zCCSPoolItem*> csPool;
    zCArray<zCCSPlayer*> csPlayerList;
    zCCSLib* library;
    int iterator;

    static zCClassDef* classDef;
};

class zCGameInfo;

class zCObjectFactory : public zCObject {
public:

    zCObjectFactory() {}
    virtual ~zCObjectFactory() {}
    virtual zCClassDef* _GetClassDef() const;
    virtual zCGameInfo* CreateGameInfo();
    virtual zCPlayerInfo* CreatePlayerInfo();
    zFILE*           CreateZFile(const zSTRING& fileName);
    zCEventManager*  CreateEventManager(zCVob* vob);
    zCNetVobControl* CreateNetVobControl(zCVob* vob);
    zCWaypoint*      CreateWaypoint();
    zCWay*           CreateWay();
    zCSession*       CreateSession();
    zCCSManager*     CreateCSManager();
    zCWorld*         CreateWorld();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);

    static zCClassDef* classDef;
};

extern zCObjectFactory* zfactory;

#endif
