#ifndef ZSTRING_H
#define ZSTRING_H

enum zTSTR_KIND {
    zSTR_ONLY,
    zSTR_BEFORE,
    zSTR_AFTER,
    zSTR_TO,
    zSTR_FROM
};

enum zTSTR_ALIGN {
    zSTR_RIGHT,
    zSTR_CENTER,
    zSTR_LEFT
};

zSTRING Characters(char value, int count);
zSTRING Spaces(int count);

template <class T>
class zCMemPool;

extern zSTRING zSTR_SKIP;
extern zSTRING zSTR_EMPTY;

class zSTRING {
    int   allocater;
    char* vector;
    int   length;
    int   reserved;

public:

    zSTRING();
    zSTRING(long value);
    zSTRING(int value);
    zSTRING(unsigned long value);
    zSTRING(float value, int precision);
    zSTRING(double value, int precision);
    zSTRING(char value);
    zSTRING(const char* value);
    zSTRING(const zSTRING& value);
    virtual ~zSTRING();
    static void operator delete(void* memory);
    static zCMemPool<zSTRING> s_MemPool;

    zSTRING& operator=(const zSTRING& value);
    zSTRING& operator=(const char* value);
    zSTRING& operator+=(const char* value);
    bool      operator==(const zSTRING& value) const;
    bool      operator<(const zSTRING& value) const;
    bool      IsEmpty() const;
    char*     ToChar() const;
    int       Length() const;
    zSTRING&  Upper();
    zSTRING&  Lower();
    zSTRING&  UpperFirstLowerRest();
    zSTRING&  Align(zTSTR_ALIGN alignment, int width, char fill);
    char&     operator[](unsigned int index);
    int       Search(const zSTRING& value, unsigned int start = 0) const;
    const char* PickWordPos(unsigned int index, const zSTRING& separators,
        const zSTRING& quotes) const;
    zSTRING   Copied(unsigned int index, unsigned long count) const;
    zSTRING   Inserted(unsigned int index, const zSTRING& value) const;
    zSTRING   Overwritten(unsigned int index, const zSTRING& value) const;
    zSTRING   Deleted(unsigned int index, unsigned long length) const;
    zSTRING   Deleted(const zSTRING& value, zTSTR_KIND kind) const;
    int       DeleteRight(unsigned int count);
    int       Delete(unsigned int index, unsigned long length);
    int       Delete(const zSTRING& value, zTSTR_KIND kind);
    int       Copy(unsigned int index, unsigned long length);
    int       Copy(const zSTRING& value, const zTSTR_KIND& kind);
    zSTRING   Copied(const zSTRING& value, const zTSTR_KIND& kind) const;
    int       Insert(unsigned int index, const zSTRING& value);
    int       Overwrite(unsigned int index, const zSTRING& value);
    void      TrimLeft(char value);
    void      TrimRight(char value);
    int       Search(int start, const char* value, unsigned int occurrence) const;
    int       SearchRev(const zSTRING& value, unsigned int occurrence) const;
    zSTRING&  Append(const zSTRING& value);
    void      Clear();
    bool      Sprintf(const char* format, ...);
    zSTRING   PickWord(unsigned int index, const zSTRING& separators, const zSTRING& quotes) const;
    zSTRING   PickWord_Old(int index, const zSTRING& separators) const;
    float     ToFloat() const;
    long      ToInt() const;
    void      Init();
};

int operator==(const zSTRING& first, const char* second);
zSTRING operator+(const zSTRING& first, const zSTRING& second);
zSTRING operator+(const char* first, const zSTRING& second);
zSTRING operator+(const zSTRING& first, const char* second);
zSTRING operator+(const zSTRING& first, char second);

#endif
