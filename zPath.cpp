#include "zPath.h"

zPATH::zPATH()
{
    path = new zFILE_FILE(zSTRING());
}

zPATH::zPATH(zSTRING path)
{
    this->path = new zFILE_FILE(path);
}

zPATH::~zPATH()
{
    delete path;
    path = 0;
}

zSTRING zPATH::GetDirectoryPath()
{
    return path->GetDirectoryPath();
}

zSTRING zPATH::GetPath()
{
    return path->GetPath();
}

zSTRING zPATH::GetFilename()
{
    return path->GetFilename();
}

zSTRING zPATH::GetExt()
{
    return path->GetExt();
}

zSTRING zPATH::GetDir()
{
    return path->GetDir();
}

int zPATH::SearchFile(const zSTRING& filePath, const zSTRING& fileName, int recursive)
{
    return path->SearchFile(filePath, fileName, recursive);
}

zSTRING zPATH::GetFile()
{
    return path->GetFile();
}

zSTRING zPATH::GetDrive()
{
    return path->GetDrive();
}
