#include "zString.h"
#include "include/zMemPool.h"

static int stringState = -1;
static int stringBufferState = -1;
static int stringContainerState = -1;
static int stringMemoryState = -1;
static int stringViewState = -1;
zSTRING zSTR_SKIP = "\r\t ";
zSTRING zSTR_EMPTY;
zCMemPool<zSTRING> zSTRING::s_MemPool(0x1000, 0x7000);

#include <cstdlib>
#include <cstring>
#include <cstdio>

zSTRING::zSTRING()
    : allocater(0), vector(0), length(0), reserved(0)
{
}

zSTRING::zSTRING(const char* value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    length = (int)std::strlen(value);
    reserved = length;
    vector = new char[length + 1];
    std::memcpy(vector, value, length + 1);
}

zSTRING::zSTRING(char value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    length = 1;
    reserved = 1;
    vector = new char[2];
    vector[0] = value;
    vector[1] = 0;
}

zSTRING::zSTRING(const zSTRING& value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    *this = value;
}

zSTRING::~zSTRING()
{
    delete[] vector;
    vector = 0;
    length = 0;
    reserved = 0;
}

bool zSTRING::operator<(const zSTRING& value) const
{
    return std::strcmp(ToChar(), value.ToChar()) < 0;
}

void zSTRING::Init()
{
}

zSTRING::zSTRING(long value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    char text[32];
    std::sprintf(text, "%ld", value);
    length = (int)std::strlen(text);
    reserved = length;
    vector = new char[length + 1];
    std::memcpy(vector, text, length + 1);
}

zSTRING::zSTRING(int value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    char text[32];
    std::sprintf(text, "%d", value);
    length = (int)std::strlen(text);
    reserved = length;
    vector = new char[length + 1];
    std::memcpy(vector, text, length + 1);
}

zSTRING::zSTRING(unsigned long value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    char text[32];
    std::sprintf(text, "%lu", value);
    length = (int)std::strlen(text);
    reserved = length;
    vector = new char[length + 1];
    std::memcpy(vector, text, length + 1);
}

zSTRING::zSTRING(float value, int precision)
    : allocater(0), vector(0), length(0), reserved(0)
{
    if (precision < 0 || precision > 9)
        precision = 9;
    char text[64];
    std::sprintf(text, "%.*f", precision, value);
    length = (int)std::strlen(text);
    reserved = length;
    vector = new char[length + 1];
    std::memcpy(vector, text, length + 1);
}

zSTRING::zSTRING(double value, int precision)
    : allocater(0), vector(0), length(0), reserved(0)
{
    if (precision < 0 || precision > 9)
        precision = 9;
    char text[64];
    std::sprintf(text, "%.*f", precision, value);
    length = (int)std::strlen(text);
    reserved = length;
    vector = new char[length + 1];
    std::memcpy(vector, text, length + 1);
}

int operator==(const zSTRING& first, const char* second)
{
    return std::strcmp(first.ToChar(), second ? second : "") == 0;
}

zSTRING& zSTRING::operator=(const zSTRING& value)
{
    if (this == &value)
        return *this;

    const int newLength = value.length;
    char* newVector = new char[newLength + 1];
    if (newLength > 0)
        std::memcpy(newVector, value.vector, newLength);
    newVector[newLength] = 0;
    delete[] vector;
    vector = newVector;
    length = newLength;
    reserved = newLength;
    return *this;
}

zSTRING& zSTRING::operator=(const char* value)
{
    zSTRING newValue(value);
    return *this = newValue;
}

void zSTRING::Clear()
{
    length = 0;
    if (vector)
        vector[0] = 0;
}

int zSTRING::Length() const
{
    return length;
}

char& zSTRING::operator[](unsigned int index)
{
    static char spaceChar = ' ';
    if (index >= static_cast<unsigned int>(length))
        return spaceChar;
    return vector[index];
}

long zSTRING::ToInt() const
{
    return std::atol(vector ? vector : "");
}

float zSTRING::ToFloat() const
{
    return (float)std::atof(vector ? vector : "");
}

int zSTRING::Search(const zSTRING& value, unsigned int start) const
{
    if (!vector || start > static_cast<unsigned int>(length))
        return -1;
    const char* found = std::strstr(vector + start, value.vector ? value.vector : "");
    return found ? static_cast<int>(found - vector) : -1;
}

zSTRING operator+(const zSTRING& first, const zSTRING& second)
{
    zSTRING result(first);
    result += second.ToChar();
    return result;
}

zSTRING operator+(const char* first, const zSTRING& second)
{
    zSTRING result(first);
    result += second.ToChar();
    return result;
}

zSTRING operator+(const zSTRING& first, const char* second)
{
    zSTRING result(first);
    result += second;
    return result;
}

zSTRING operator+(const zSTRING& first, char second)
{
    zSTRING result(first);
    const char text[2] = { second, 0 };
    result += text;
    return result;
}

zSTRING zSTRING::Copied(unsigned int index, unsigned long count) const
{
    if (!vector || index >= static_cast<unsigned int>(length))
        return zSTRING("");
    if (index + count > static_cast<unsigned int>(length))
        count = length - index;
    char* text = new char[count + 1];
    std::memcpy(text, vector + index, count);
    text[count] = 0;
    zSTRING result(text);
    delete[] text;
    return result;
}

#include <cctype>

zSTRING Characters(char value, int count)
{
    char* text = new char[count + 1];
    std::memset(text, value, count);
    text[count] = 0;
    zSTRING result(text);
    delete[] text;
    return result;
}

zSTRING Spaces(int count)
{
    return Characters(' ', count);
}

zSTRING& zSTRING::Lower()
{
    for (int i = 0; i < length; ++i)
        vector[i] = (char)std::tolower((unsigned char)vector[i]);
    return *this;
}

zSTRING& zSTRING::Upper()
{
    for (int i = 0; i < length; ++i)
        vector[i] = (char)std::toupper((unsigned char)vector[i]);
    return *this;
}

zSTRING& zSTRING::UpperFirstLowerRest()
{
    Lower();
    if (length > 0)
        vector[0] = (char)std::toupper((unsigned char)vector[0]);
    return *this;
}

zSTRING& zSTRING::Align(zTSTR_ALIGN alignment, int width, char fill)
{
    if (fill == 0)
        fill = ' ';
    int missing = width - length;
    if (missing <= 0)
        return *this;

    if (alignment == zSTR_RIGHT)
        Insert(0, Characters(fill, missing));
    else if (alignment == zSTR_CENTER)
    {
        int right = missing / 2;
        Insert(0, Characters(fill, missing - right));
        Append(Characters(fill, right));
    }
    else if (alignment == zSTR_LEFT)
        Append(Characters(fill, missing));
    return *this;
}

const char* zSTRING::PickWordPos(unsigned int index, const zSTRING& separators,
    const zSTRING& quotes) const
{
    unsigned int current = 1;
    int position = 0;
    char quote = 0;

    while (position < length)
    {
        char value = vector[position];
        if (quote)
        {
            if (value == quote)
                quote = 0;
        }
        else if (quotes.Search(value) >= 0)
            quote = value;
        else if (separators.Search(value) >= 0)
        {
            if (current == index)
                return vector + position;
            ++current;
        }
        else if (current == index)
            return vector + position;
        ++position;
    }

    return NULL;
}

zSTRING zSTRING::PickWord(unsigned int index, const zSTRING& separators,
    const zSTRING& quotes) const
{
    const char* start = PickWordPos(index, separators, quotes);
    if (!start)
        return "";
    if (separators.Search(*start) >= 0)
        return zSTRING(*start);

    zSTRING result;
    while (*start && separators.Search(*start) < 0 && quotes.Search(*start) < 0)
    {
        result += zSTRING(*start).ToChar();
        ++start;
    }
    return result;
}

zSTRING zSTRING::PickWord_Old(int index, const zSTRING& separators) const
{
    return PickWord(index, separators, "");
}

int zSTRING::Insert(unsigned int index, const zSTRING& value)
{
    if (index > (unsigned int)length)
        index = length;
    zSTRING result = Copied(0, index) + value + Copied(index, length - index);
    *this = result;
    return 0;
}

zSTRING zSTRING::Inserted(unsigned int index, const zSTRING& value) const
{
    zSTRING result(*this);
    result.Insert(index, value);
    return result;
}

int zSTRING::Overwrite(unsigned int index, const zSTRING& value)
{
    if (index >= (unsigned int)length)
        return 0;
    int count = value.Length();
    if (count > length - (int)index)
        count = length - index;
    zSTRING result = Copied(0, index) + value.Copied(0, count) +
        Copied(index + count, length - index - count);
    *this = result;
    return 0;
}

zSTRING zSTRING::Overwritten(unsigned int index, const zSTRING& value) const
{
    zSTRING result(*this);
    result.Overwrite(index, value);
    return result;
}

int zSTRING::DeleteRight(unsigned int count)
{
    if (count > (unsigned int)length)
        count = length;
    return Delete(length - count, count);
}

int zSTRING::Delete(unsigned int index, unsigned long count)
{
    if (index >= (unsigned int)length)
        return 0;
    if (index + count > (unsigned int)length)
        count = length - index;
    zSTRING result = Copied(0, index) + Copied(index + count, length - index - count);
    *this = result;
    return 0;
}

zSTRING zSTRING::Deleted(unsigned int index, unsigned long count) const
{
    zSTRING result(*this);
    result.Delete(index, count);
    return result;
}

int zSTRING::Delete(const zSTRING& value, zTSTR_KIND kind)
{
    int position = Search(value);
    if (position < 0)
        return 0;

    switch (kind)
    {
    case zSTR_ONLY:
        return Delete(position, value.Length());
    case zSTR_BEFORE:
        return Delete(0, position);
    case zSTR_AFTER:
        return Delete(position + value.Length(), length - position - value.Length());
    case zSTR_TO:
        return Delete(0, position + value.Length());
    case zSTR_FROM:
        return Delete(position, length - position);
    }
    return 0;
}

zSTRING zSTRING::Deleted(const zSTRING& value, zTSTR_KIND kind) const
{
    zSTRING result(*this);
    result.Delete(value, kind);
    return result;
}

int zSTRING::Copy(unsigned int index, unsigned long count)
{
    *this = Copied(index, count);
    return 0;
}

zSTRING& zSTRING::Append(const zSTRING& value)
{
    Insert(length, value);
    return *this;
}

zSTRING& zSTRING::operator+=(const char* value)
{
    return Append(zSTRING(value));
}

zSTRING::zSTRING(char value)
    : allocater(0), vector(0), length(0), reserved(0)
{
    length = 1;
    reserved = 1;
    vector = new char[2];
    vector[0] = value;
    vector[1] = 0;
}

int zSTRING::Copy(const zSTRING& value, const zTSTR_KIND& kind)
{
    int position = Search(value);
    if (position < 0)
    {
        Clear();
        return 0;
    }

    switch (kind)
    {
    case zSTR_ONLY:
        *this = Copied(position, value.Length());
        break;
    case zSTR_BEFORE:
        *this = Copied(0, position);
        break;
    case zSTR_AFTER:
        *this = Copied(position + value.Length(), length - position - value.Length());
        break;
    case zSTR_TO:
        *this = Copied(0, position + value.Length());
        break;
    case zSTR_FROM:
        *this = Copied(position, length - position);
        break;
    }
    return 0;
}

zSTRING zSTRING::Copied(const zSTRING& value, const zTSTR_KIND& kind) const
{
    zSTRING result(*this);
    result.Copy(value, kind);
    return result;
}

void zSTRING::TrimLeft(char value)
{
    int count = 0;
    while (count < length && vector[count] == value)
        ++count;
    Delete(0, count);
}

void zSTRING::TrimRight(char value)
{
    int count = 0;
    while (count < length && vector[length - count - 1] == value)
        ++count;
    DeleteRight(count);
}

int zSTRING::Search(int start, const char* value, unsigned int occurrence) const
{
    if (!value || !value[0])
        return start;
    const char* current = vector + start;

    for (unsigned int i = 0; i < occurrence; ++i)
    {
        current = std::strstr(current, value);
        if (!current)
            return -1;
        if (i + 1 == occurrence)
            return (int)(current - vector);
        ++current;
    }
    return -1;
}

int zSTRING::SearchRev(const zSTRING& value, unsigned int occurrence) const
{
    if (value.IsEmpty())
        return -1;
    unsigned int found = 0;

    for (int i = length - value.Length(); i >= 0; --i)
    {
        if (std::memcmp(vector + i, value.ToChar(), value.Length()) == 0 && ++found == occurrence)
            return i;
    }
    return -1;
}

#include <cstdarg>

bool zSTRING::Sprintf(const char* format, ...)
{
    if (format)
    {
        va_list arguments;
        va_start(arguments, format);
        std::vsprintf(ToChar(), format, arguments);
        va_end(arguments);
    }
    return true;
}
