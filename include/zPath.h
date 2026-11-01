#ifndef ZPATH_H
#define ZPATH_H

#include "zFile.h"

class zPATH {
public:

    zPATH();
    zPATH(zSTRING path);
    virtual ~zPATH();

    zSTRING GetDirectoryPath();
    zSTRING GetPath();
    zSTRING GetFilename();
    zSTRING GetExt();
    zSTRING GetDir();
    int     SearchFile(const zSTRING& filePath, const zSTRING& fileName, int recursive);
    zSTRING GetFile();
    zSTRING GetDrive();

    zFILE_FILE* path;
};

#endif
