#include "include/zScanDir.h"
#include "include/zFile.h"

static int scanDirState = -1;
zCScanDir dirScanner;

zCScanDirFileHandler::zCScanDirFileHandler()
{
}

zCScanDirFileHandler::~zCScanDirFileHandler()
{
}

int zCScanDirFileHandler::CanHandleFile(zSTRING& fileName)
{
    ClearMessages();
    zFILE_FILE file(fileName);
    zSTRING extension = file.GetExt();

    for (int i = 0; i < extList.numInArray; ++i)
    {
        if (extension == extList[i])
            return 1;
    }
    return 0;
}

int zCScanDirFileHandler::GetNumMessages()
{
    return msgList.numInArray;
}

zSTRING zCScanDirFileHandler::GetMessageA(int index)
{
    if (index < 0 || index >= msgList.numInArray)
        return "";
    return msgList[index];
}

void zCScanDirFileHandler::RegisterFileType(zSTRING extension)
{
    extension.Upper();
    extList.Insert(extension);
}

void zCScanDirFileHandler::AddMessage(zSTRING message)
{
    msgList.Insert(message);
}

void zCScanDirFileHandler::ClearMessages()
{
    msgList.DeleteList();
}

#include <direct.h>
#include <cstring>

int zCScanDirFileHandler::HandleFile(const zSTRING&, const char*, const zFINDDATA&)
{
    return 0;
}

zCScanDir::zCScanDir()
    : statFoundFiles(0), statHandledFiles(0), statTouchedFiles(0)
{
    cbList.next = NULL;
    cbList.data = NULL;
}

zCScanDir::~zCScanDir()
{
    cbList.DeleteList();
}

void zCScanDir::RegisterFileHandler(zCScanDirFileHandler* handler, const zSTRING& directory)
{
    if (!directory.IsEmpty())
        handler->SetName(directory);

    for (zCList<zCScanDirFileHandler>* entry = cbList.next; entry; entry = entry->next)
    {
        if (entry->data == handler)
            return;
    }
    cbList.Insert(handler);
}

void zCScanDir::ScanX()
{
    zFILE_FILE file;
    if (!file.FindFirst("*"))
        return;

    do
    {
        zSTRING name = file.find_fileinfo.name;
        if (file.find_fileinfo.attrib & 0x10)
        {
            if (name[0] != '.')
            {
                if (_chdir(name.ToChar()) == 0)
                {
                    ScanX();
                    _chdir("..");
                }
            }
            continue;
        }

        name.Upper();
        char currentPath[260];
        _getcwd(currentPath, sizeof(currentPath));
        zFINDDATA findData;
        findData.attributes = file.find_fileinfo.attrib;
        findData.size = (unsigned long)file.find_fileinfo.size;
        std::strncpy(findData.name, file.find_fileinfo.name, sizeof(findData.name));
        findData.name[sizeof(findData.name) - 1] = 0;

        int touched = 0;
        for (zCList<zCScanDirFileHandler>* entry = cbList.next; entry && !touched;
            entry = entry->next)
        {
            zFILE::DirectFileAccess(true);
            if (entry->data->CanHandleFile(name))
            {
                touched = entry->data->HandleFile(currentPath, findData.name, findData);
                ++statHandledFiles;
            }
            zFILE::DirectFileAccess(false);
            _chdir(currentPath);
        }
        ++statFoundFiles;
        if (touched)
            ++statTouchedFiles;
    } while (file.FindNext());
}

void zCScanDir::Scan(const zSTRING& directory)
{
    if (!zFILE::LockDirScan())
        return;

    zFILE_FILE oldDirectory;
    oldDirectory.SetCurrentDir();
    statFoundFiles = 0;
    statTouchedFiles = 0;
    statHandledFiles = 0;
    zFILE_FILE scanDirectory(directory);
    scanDirectory.ChangeDir(false);
    zerr->Report(zERR_TYPE_INFO, 0,
        "B: DSCAN: Scanning directory \"" + scanDirectory.GetPath() + "\"", 0, 0, 195,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zScanDir.cpp", NULL);
    zFILE::DirectFileAccess(true);
    ScanX();
    zFILE::DirectFileAccess(false);
    zerr->Report(zERR_TYPE_INFO, 0, "B: DSCAN: " + zSTRING(statFoundFiles) + " found files.",
        0, 0, 201, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zScanDir.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0, "B: DSCAN: " + zSTRING(statHandledFiles) + " handled files.",
        0, 0, 202, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zScanDir.cpp", NULL);
    zerr->Report(zERR_TYPE_INFO, 0, "B: DSCAN: " + zSTRING(statTouchedFiles) + " touched files.",
        0, 0, 203, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zScanDir.cpp", NULL);
    oldDirectory.ChangeDir(false);
    zFILE::UnlockDirScan();
}
