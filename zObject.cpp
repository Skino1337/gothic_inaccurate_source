#include "zObject.h"
#include "zFile.h"
#include "zModel.h"
#include "zAccount.h"

#include <cstdlib>
#include <cstring>

static int objectState = -1;
static zCClassDef zCObjectClassDef("zCObject", "NULL", NULL, 0, 0x24, 0, "");
zCClassDef* zCObject::classDef = &zCObjectClassDef;
static zSTRING objectFactoryBroadcast("broadcast");
static zSTRING objectFactoryLocalhost("127.0.0.1");
zCArraySort<zCClassDef*>* zCClassDef::classDefList;
zCClassDef* zCClassDef::classDefSearchDummy;
int zCClassDef::startupFinished;

zCClassDef::zCClassDef()
    : baseClassDef(0), createNewInstance(0), createNewInstanceBackup(0),
      classFlags(0), classSize(0), numLivingObjects(0), numCtorCalled(0),
      hashTable(0), objectList(), archiveVersion(0), archiveVersionSum(0)
{
}

zCClassDef::zCClassDef(const zSTRING& name, const zSTRING& baseName,
    zCObject*(*factory)(), unsigned long flags, unsigned long size,
    unsigned short version, const zSTRING& scriptName)
    : className(name), baseClassName(baseName), scriptClassName(scriptName),
      baseClassDef(0), createNewInstance(factory), createNewInstanceBackup(factory),
      classFlags(flags), classSize(size), numLivingObjects(0), numCtorCalled(0),
      hashTable(0), objectList(), archiveVersion(version), archiveVersionSum(0)
{
    if (!classDefList) {
        classDefList = new zCArraySort<zCClassDef*>;
        classDefList->numInArray = 0;
        classDefList->numAlloc = 180;
        classDefList->array = new zCClassDef*[180];
        classDefList->Compare = CompareClassDefNames;
        classDefSearchDummy = new zCClassDef;
    }

    baseClassDef = GetClassDef(baseClassName);

    if (classFlags & 1) {
        hashTable = new zCObject*[1024];
        std::memset(hashTable, 0, 1024 * sizeof(zCObject*));
    }

    classDefList->array[classDefList->numInArray] = this;
    ++classDefList->numInArray;
}

void zCClassDef::Init()
{
    baseClassDef = 0;
    createNewInstance = 0;
    createNewInstanceBackup = 0;
    classFlags = 0;
    classSize = 0;
    numLivingObjects = 0;
    numCtorCalled = 0;
    hashTable = 0;
    archiveVersion = 0;
    archiveVersionSum = 0;
}

zCClassDef::~zCClassDef()
{
    ReportLeaks();
    delete[] hashTable;
    hashTable = 0;
    baseClassDef = 0;
    createNewInstance = 0;
    createNewInstanceBackup = 0;

    if (classDefList) {
        for (int index = 0; index < classDefList->numInArray; ++index) {
            if (classDefList->array[index] == this) {
                --classDefList->numInArray;
                classDefList->array[index] = classDefList->array[classDefList->numInArray];
                break;
            }
        }

        if (classDefList->numInArray <= 0) {
            delete[] classDefList->array;
            classDefList->array = 0;
            delete classDefList;
            classDefList = 0;
            delete classDefSearchDummy;
            classDefSearchDummy = 0;
        }
    }

    delete[] objectList.parray;
    objectList.parray = 0;
}

void zCClassDef::ReportLeaks()
{
    if (numLivingObjects <= 0)
        return;

    zerr->Message(zSTRING("D: OBJ: Memory leaks found!"));

    int reported = 0;

    for (int index = objectList.numInArray - 1; index >= 0 && reported < 4; --index, ++reported) {
        zCObject* object = objectList.parray[index];

        if (object->refCtr)
            zerr->Message(object->objectName);
    }
}

unsigned long zCClassDef::GetHashIndex(const zSTRING& name)
{
    return zCChecksum::GetBufferCRC32(name.ToChar(), name.Length(), 0) & 0x3FF;
}

unsigned long zCClassDef::GetHashIndex(zCObject* object)
{
    return GetHashIndex(object->objectName);
}

void zCClassDef::InsertHashTable(zCObject* object)
{
    const unsigned long hashIndex = GetHashIndex(object);
    object->hashNext = hashTable[hashIndex];
    hashTable[hashIndex] = object;
    object->hashIndex = hashIndex;
}

void zCClassDef::RemoveHashTable(zCObject* object)
{
    zCObject** current = &hashTable[object->hashIndex];

    while (*current && *current != object)
        current = &(*current)->hashNext;

    if (*current) {
        *current = object->hashNext;
        object->hashNext = 0;
        object->hashIndex = 0xFFFF;
    }
}

zCObject* zCClassDef::SearchHashTable(const zSTRING& name)
{
    if (!name.Length())
        return 0;

    zCObject* current = hashTable[GetHashIndex(name)];

    while (current) {
        if (current->objectName == name)
            return current;

        current = current->hashNext;
    }

    return 0;
}

float zCClassDef::CalcHashTableSpread()
{
    if (!hashTable)
        return 0.0f;

    int usedSlots = 0;

    for (int index = 0; index < 1024; ++index) {
        if (hashTable[index])
            ++usedSlots;
    }

    return usedSlots * 0.0009765625f;
}

void zCClassDef::ObjectCreated(zCObject* object, zCClassDef* classDef)
{
    ++classDef->numLivingObjects;
    ++classDef->numCtorCalled;

    if (classDef->classFlags & 1) {
        if (classDef->objectList.numInArray >= classDef->objectList.numAlloc) {
            const int newAlloc = classDef->objectList.numAlloc +
                (classDef->objectList.numAlloc < 16 ? 16 : classDef->objectList.numAlloc / 2);
            zCObject** newArray = new zCObject*[newAlloc];

            for (int index = 0; index < classDef->objectList.numInArray; ++index)
                newArray[index] = classDef->objectList.parray[index];

            delete[] classDef->objectList.parray;
            classDef->objectList.parray = newArray;
            classDef->objectList.numAlloc = newAlloc;
        }

        classDef->objectList.parray[classDef->objectList.numInArray] = object;
        ++classDef->objectList.numInArray;
    }
}

void zCClassDef::ObjectCreated(zCObject* object, zCClassDef* classDef,
    const char* file, int line)
{
    zCAccount::Clear(object);
    ++zCAccount::s_actCounter;
    ObjectCreated(object, classDef);
    zCAccount::Add(object, file, line, 1);
}

void zCClassDef::ObjectDeleted(zCObject* object, zCClassDef* classDef)
{
    --classDef->numLivingObjects;

    if (!(classDef->classFlags & 1))
        return;

    for (int index = 0; index < classDef->objectList.numInArray; ++index) {
        if (classDef->objectList.parray[index] == object) {
            --classDef->objectList.numInArray;
            classDef->objectList.parray[index] =
                classDef->objectList.parray[classDef->objectList.numInArray];
            break;
        }
    }

    if (object->hashIndex == 0xFFFF)
        return;

    zCObject** current = &classDef->hashTable[object->hashIndex];

    while (*current && *current != object)
        current = &(*current)->hashNext;

    if (*current) {
        *current = object->hashNext;
        object->hashNext = 0;
        object->hashIndex = 0xFFFF;
    }
}

void zCClassDef::EndStartup()
{
    if (!classDefList)
        return;

    qsort(classDefList->array, classDefList->numInArray, sizeof(void*), classDefList->Compare);

    for (int index = 0; index < classDefList->numInArray; ++index) {
        zCClassDef* current = classDefList->array[index];
        current->baseClassDef = GetClassDef(current->baseClassName);
    }

    startupFinished = 1;
}

int zCClassDef::CompareClassDefNames(const void* ele1, const void* ele2)
{
    const zCClassDef* left = *static_cast<zCClassDef* const*>(ele1);
    const zCClassDef* right = *static_cast<zCClassDef* const*>(ele2);
    const int result = std::strcmp(left->className.ToChar(), right->className.ToChar());
    return (result > 0) - (result < 0);
}

zCObject* zCClassDef::CreateNewInstance() const
{
    if (createNewInstance)
        return createNewInstance();

    return 0;
}

void zCClassDef::OverwriteCreateNewInstance(zCClassDef* other)
{
    if (!createNewInstance)
        return;

    if (other->createNewInstance)
        createNewInstance = other->createNewInstance;
}

void zCClassDef::ResetCreateNewInstance()
{
    if (createNewInstance)
        createNewInstance = createNewInstanceBackup;
}

unsigned short zCClassDef::GetClassID()
{
    if (classDefList->numInArray <= 0)
        return 0xFFFF;

    int left = 0;
    int right = classDefList->numInArray - 1;

    while (left <= right) {
        const int middle = (left + right) / 2;
        zCClassDef* candidate = classDefList->array[middle];
        const int comparison = classDefList->Compare(&this, &candidate);

        if (comparison > 0) {
            left = middle + 1;
        }
        else if (comparison < 0) {
            right = middle - 1;
        }
        else {
            return middle;
        }
    }

    return 0xFFFF;
}

zCObject* zCClassDef::CreateNewInstanceID(unsigned short classID)
{
    return classDefList->array[classID]->CreateNewInstance();
}

zCClassDef* zCClassDef::GetClassDef(const zSTRING& className)
{
    classDefSearchDummy->className = className;
    int left = 0;
    int right = classDefList->numInArray - 1;

    while (left <= right) {
        const int middle = (left + right) / 2;
        const int comparison = classDefList->Compare(classDefSearchDummy, classDefList->array[middle]);

        if (comparison > 0) {
            left = middle + 1;
        }
        else if (comparison < 0) {
            right = middle - 1;
        }
        else {
            return classDefList->array[middle];
        }
    }

    return 0;
}

zCClassDef* zCClassDef::GetClassDefTypeInsensitive(const zSTRING& className)
{
    zSTRING searchName = className;
    searchName.Upper();

    for (int index = 0; index < classDefList->numInArray; ++index) {
        zCClassDef* candidate = classDefList->array[index];
        zSTRING candidateName = candidate->className;
        candidateName.Upper();

        if (searchName == candidateName)
            return candidate;
    }

    return 0;
}

zCObject::zCObject()
    : refCtr(1), hashIndex(0xFFFF), hashNext(0), objectName()
{
}

zCObject::~zCObject()
{
    refCtr = -999;
}

zCClassDef* zCObject::_GetClassDef() const
{
    return classDef;
}

void zCObject::AddRef()
{
    ++refCtr;
}

int zCObject::Release()
{
    const int result = --refCtr;

    if (result <= 0)
        delete this;

    return result;
}

const zSTRING& zCObject::GetObjectName() const
{
    return objectName;
}

int zCObject::SetObjectName(const zSTRING& name)
{
    objectName = name;
    objectName.Upper();
    return 1;
}

zCObject* zCObject::CreateNewInstance(const zSTRING& className)
{
    zCClassDef* classDef = zCClassDef::GetClassDef(className);

    if (classDef)
        return classDef->CreateNewInstance();

    return 0;
}

zCObject* zCObject::CreateCopy()
{
    zCObject* result = _GetClassDef()->CreateNewInstance();

    if (result) {
        zCArchiver* writer = zarcFactory->CreateArchiverWrite(0, 0, 1, 0);
        writer->WriteObject(this);
        zCBuffer* buffer = writer->GetBuffer();

        if (buffer) {
            buffer->SetMode(0);
            zCArchiver* reader = zarcFactory->CreateArchiverRead(buffer, 0);
            reader->ReadObject(result);
            reader->Close();
            reader->Release();
            buffer->SetMode(1);
        }

        writer->Close();
        writer->Release();
    }

    return result;
}

int zCObject::CheckInheritance(const zCClassDef* baseClass, const zCClassDef* derivedClass)
{
    for (const zCClassDef* current = derivedClass; current; current = current->baseClassDef) {
        if (current == baseClass)
            return 1;
    }

    return 0;
}

int zCObject::CheckInheritance(const zCObject* baseObject, const zCObject* derivedObject)
{
    if (!baseObject || !derivedObject)
        return 0;

    return CheckInheritance(baseObject->_GetClassDef(), derivedObject->_GetClassDef());
}

int zCObject::CheckInheritance(const zSTRING& baseClass, const zSTRING& derivedClass)
{
    return CheckInheritance(zCClassDef::GetClassDef(baseClass), zCClassDef::GetClassDef(derivedClass));
}

void zCObject::GetMemStats(int& numObjects, int& numBytes, zCClassDef* baseClass)
{
    if (!baseClass) {
        numObjects = 0;
        numBytes = 0;
    }

    for (int index = 0; index < zCClassDef::classDefList->numInArray; ++index) {
        zCClassDef* current = zCClassDef::classDefList->array[index];

        if (current->baseClassDef == baseClass) {
            numObjects += current->numLivingObjects;
            numBytes += current->classSize * current->numLivingObjects;
            GetMemStats(numObjects, numBytes, current);
        }
    }
}

void zCObject::Archive(zCArchiver&)
{
}

void zCObject::Unarchive(zCArchiver&)
{
}

int zERROR::GetFilterLevel()
{
    return filter_level;
}

static zCClassDef zCObjectFactoryClassDef("zCObjectFactory", "zCObject", NULL, 0, 0x24, 0, "");
zCClassDef* zCObjectFactory::classDef = &zCObjectFactoryClassDef;

// EXE: 0x005A8B00
zCObject* zCObjectFactory::_CreateNewInstance()
{
    zCObjectFactory* factory = new zCObjectFactory;
    zCClassDef::ObjectCreated(factory, classDef);
    return factory;
}

void zCObject::PrintClassHierarchy(int level, zCClassDef* baseClass,
    int numObjects, int numBytes)
{
    if (!classDefList)
        return;
    for (int i = 0; i < classDefList->numInArray; ++i) {
        zCClassDef* current = classDefList->array[i];
        if (current->baseClassDef != baseClass)
            continue;

        zSTRING line;
        for (int j = 0; j < level * 4; ++j)
            line.Append(" ");
        line.Append(current->className);
        if (current->numLivingObjects > 0) {
            zSTRING objects;
            objects.Sprintf(" [objs %d]", current->numLivingObjects);
            line.Append(objects);
        }
        if (current->numCtorCalled > 0) {
            zSTRING constructors;
            constructors.Sprintf(" [ctor %d]", current->numCtorCalled);
            line.Append(constructors);
        }
        if (current->numLivingObjects > 0 && (current->classFlags & 1)) {
            zSTRING hash;
            hash.Sprintf(" [hash %.2f]", current->CalcHashTableSpread());
            line.Append(hash);
        }
        zerr->Message(line);
        PrintClassHierarchy(level + 1, current,
            numObjects + current->numLivingObjects,
            numBytes + current->classSize * current->numLivingObjects);
    }
}

void zCClassDef::ObjectCreated(zCObject* objects, int bytes, zCClassDef* classDef)
{
    for (int i = 0; i < bytes / 0x24; ++i)
        ObjectCreated(reinterpret_cast<zCObject*>(reinterpret_cast<char*>(objects) + i * 0x24),
            classDef);
}

void zCClassDef::ObjectDeleted(zCObject* objects, int bytes, zCClassDef* classDef)
{
    for (int i = 0; i < bytes / 0x24; ++i)
        ObjectDeleted(reinterpret_cast<zCObject*>(reinterpret_cast<char*>(objects) + i * 0x24),
            classDef);
}

void zCClassDef::GetLivingObjectsState(zCLivingObjectsState& state)
{
    if (!classDefList)
        return;
    state.numList.AllocAbs(classDefList->numInArray);
    state.numList.numInArray = state.numList.numAlloc;
    for (int i = 0; i < classDefList->numInArray; ++i)
        state.numList.parray[i] = classDefList->array[i]->numLivingObjects;
}

void zCClassDef::CompareLivingObjectsState(const zCLivingObjectsState& state)
{
    if (!classDefList || state.numList.numInArray != classDefList->numInArray)
        return;
    for (int i = 0; i < classDefList->numInArray; ++i) {
        zCClassDef* current = classDefList->array[i];
        const int difference = current->numLivingObjects - state.numList.parray[i];
        if (!difference)
            continue;
        zSTRING message;
        message.Sprintf("D: OBJ: %s, diff: %d", current->className.ToChar(), difference);
        zerr->Message(message);
    }
}

static zCClassDef zCNetVobControlClassDef("zCNetVobControl", "zCObject",
    zCNetVobControl::_CreateNewInstance, 0, 0x30, 0, "");
zCClassDef* zCNetVobControl::classDef = &zCNetVobControlClassDef;
static zCClassDef zCWaypointClassDef("zCWaypoint", "zCObject", zCWaypoint::_CreateNewInstance,
    0, 0x7C, 0, "");
zCClassDef* zCWaypoint::classDef = &zCWaypointClassDef;
static int zCCSManagerClassDefState = -1;
static zCClassDef zCCSManagerClassDef("zCCSManager", "zCObject",
    zCCSManager::_CreateNewInstance, 1, 0x70, 0, "");
zCClassDef* zCCSManager::classDef = &zCCSManagerClassDef;

zFILE* zCObjectFactory::CreateZFile(const zSTRING& fileName)
{
    return new zFILE_FILE(fileName);
}

zCEventManager* zCObjectFactory::CreateEventManager(zCVob* vob)
{
    return new zCEventManager(vob);
}

void zCEventManager::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(reinterpret_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCNetVobControl* zCObjectFactory::CreateNetVobControl(zCVob* vob)
{
    return new zCNetVobControl(vob);
}

zCWaypoint* zCObjectFactory::CreateWaypoint()
{
    return new zCWaypoint;
}

void zCWaypoint::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(reinterpret_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCWay* zCObjectFactory::CreateWay()
{
    return new zCWay;
}

zCSession* zCObjectFactory::CreateSession()
{
    return new zCSession;
}

zCCSManager* zCObjectFactory::CreateCSManager()
{
    return new zCCSManager;
}

zCWorld* zCObjectFactory::CreateWorld()
{
    return new zCWorld;
}

void zCWorld::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(reinterpret_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCObjectFactory::_GetClassDef() const
{
    return classDef;
}

zCGameInfo* zCObjectFactory::CreateGameInfo()
{
    return NULL;
}

zCPlayerInfo* zCObjectFactory::CreatePlayerInfo()
{
    return NULL;
}
