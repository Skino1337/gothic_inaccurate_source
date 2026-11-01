#ifndef ZFILECSV_H
#define ZFILECSV_H

#include "zFile.h"

class zCFileCSV : public zFILE_FILE {
public:

    zCFileCSV();
    zCFileCSV(const zSTRING& path);
    virtual ~zCFileCSV() {}
    void InitCSV();
    void SetFormat(bool hasHeader, const zSTRING& delimiter, const zSTRING& frame);
    void AddNextField(const zSTRING& value);
    void AddNextHeader(const zSTRING& value);
    int WriteSet();
    zSTRING GetNextField(const zSTRING& line);
    int ReadSet();
    int ReadHeader();
    int WriteHeader();

    zCArray<zSTRING> columnList;
    bool headerProcessed;
    zSTRING current_set;
    bool csvHasHeader;
    zSTRING csvFieldDelimiter;
    zSTRING csvFieldFrame;
    zSTRING csvCompleteDelimiter;
    int column;
};

#endif
