#include "include/zFileCsv.h"

static int csvState = -1;
static zSTRING csvDefaultFieldDelimiter = ";";
static zSTRING csvDefaultFieldFrame = "\"";

zCFileCSV::zCFileCSV()
{
    InitCSV();
}

zCFileCSV::zCFileCSV(const zSTRING& path)
    : zFILE_FILE(path)
{
    InitCSV();
}

void zCFileCSV::InitCSV()
{
    columnList.numInArray = 0;
    headerProcessed = false;
    current_set.Clear();
    csvHasHeader = false;
    csvFieldDelimiter = ";";
    csvFieldFrame = "\"";
    csvCompleteDelimiter = ";";
    column = 0;
}

void zCFileCSV::SetFormat(bool hasHeader, const zSTRING& delimiter, const zSTRING& frame)
{
    csvHasHeader = hasHeader;
    csvFieldDelimiter = delimiter;
    csvFieldFrame = frame;
    csvCompleteDelimiter = delimiter + frame;
}

void zCFileCSV::AddNextField(const zSTRING& value)
{
    current_set += value + csvCompleteDelimiter;
}

void zCFileCSV::AddNextHeader(const zSTRING& value)
{
    columnList.InsertEnd(value);
    current_set += value + csvCompleteDelimiter;
}

int zCFileCSV::WriteSet()
{
    if (current_set.IsEmpty() && columnList.numInArray == 0)
        return 0;
    const int result = Write(current_set + "\n");
    current_set.Clear();
    columnList.numInArray = 0;
    return result != 0;
}

zSTRING zCFileCSV::GetNextField(const zSTRING& line)
{
    current_set = line;
    int start = 0;
    for (int i = 0; i < column; ++i) {
        const int next = current_set.Search(csvFieldDelimiter, start);
        if (next < 0)
            return "";
        start = next + csvFieldDelimiter.Length();
    }
    int end = current_set.Search(csvFieldDelimiter, start);
    if (end < 0)
        end = current_set.Length();
    if (end < start)
        return "";
    zSTRING result = current_set.Copied(start, end - start);
    ++column;
    if (!csvFieldFrame.IsEmpty()) {
        if (result.Length() >= csvFieldFrame.Length() &&
            result.Search(csvFieldFrame) == 0)
            result.Delete(0, csvFieldFrame.Length());
        const int frameStart = result.Length() - csvFieldFrame.Length();
        if (frameStart >= 0 && result.Search(csvFieldFrame, frameStart) == frameStart)
            result.DeleteRight(csvFieldFrame.Length());
    }
    return result;
}

int zCFileCSV::ReadSet()
{
    current_set.Clear();
    Read(current_set);
    column = 0;
    return !Eof();
}

int zCFileCSV::ReadHeader()
{
    if (!ReadSet())
        return 0;
    while (true) {
        const zSTRING field = GetNextField(current_set);
        if (field.IsEmpty())
            break;
        columnList.InsertEnd(field);
    }
    headerProcessed = true;
    return 1;
}

int zCFileCSV::WriteHeader()
{
    if (current_set.IsEmpty() && columnList.numInArray == 0)
        return 0;
    const int result = Write(current_set + "\n");
    current_set.Clear();
    columnList.numInArray = 0;
    headerProcessed = true;
    return result != 0;
}
