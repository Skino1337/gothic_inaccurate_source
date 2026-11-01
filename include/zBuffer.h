#ifndef ZBUFFER_H
#define ZBUFFER_H

#include "zString.h"
#include "zObject.h"

enum zTBufferMode {
    READ,
    WRITE,
    READWRITE
};

class zCBuffer {
public:

    zCBuffer();
    zCBuffer(const unsigned long& size);
    zCBuffer(void* data, const unsigned long& size);
    zCBuffer(const zCBuffer& other);
    virtual ~zCBuffer();

    zCBuffer& operator=(const zCBuffer& other);
    void       CopyFrom(const zCBuffer& other);
    void       ByteFreq_Analyse();
    static void ByteFreq_Reset();
    static void ByteFreq_Print();
    void       Init();
    zTBufferMode SetMode(zTBufferMode newMode);
    void         Reset();
    void         Resize(unsigned long newSize);
    unsigned long Write(const void* data, unsigned long size);
    unsigned long WriteBits(const void* data, unsigned long numBits);
    unsigned long Read(void* data, unsigned long size);
    unsigned long ReadBits(void* data, unsigned long numBits);
    unsigned long WriteBool(int value, int numBits);
    unsigned long WriteByte(unsigned char value, int numBits);
    unsigned long WriteWord(unsigned short value, int numBits);
    unsigned long WriteDWord(unsigned long value, int numBits);
    unsigned long WriteReal(float value, int numBits);
    unsigned long WriteInt(int value, int numBits);
    unsigned long WriteFloat(float value, int numBits);
    int           ReadBool(int numBits);
    unsigned char ReadByte(int numBits);
    unsigned short ReadWord(int numBits);
    unsigned long ReadDWord(int numBits);
    float         ReadReal(int numBits);
    int           ReadInt(int numBits);
    float         ReadFloat(int numBits);
    unsigned long WriteFloatPacked0_16(float value);
    unsigned long WriteFloatPacked0_8(float value);
    unsigned long WriteFloatPacked8_8(float value);
    float         ReadFloatPacked0_16();
    float         ReadFloatPacked0_8();
    float         ReadFloatPacked8_8();
    unsigned long WriteString(zSTRING value);
    zSTRING       ReadString();
    void          CompleteByte();
    void          SetPos(const unsigned long& position);
    void          SetBitPos(const unsigned long& position);
    unsigned long SetPosRel(int offset);
    unsigned long SetBitPosRel(int offset);
    void          SetPosBegin();
    void          SetPosEnd();
    unsigned long GetPos() const;
    unsigned long GetBitPos() const;
    unsigned long GetMaxSize() const;
    unsigned long GetBitMaxSize() const;
    unsigned long GetSize() const;
    unsigned long GetBitSize() const;
    unsigned long GetRestSize() const;
    unsigned long GetBitRestSize() const;
    unsigned char* GetBuffer() const;
    unsigned char* GetCursor() const;
    void          SkipByte();
    int           GetBitSizeToNextByte() const;
    bool          HasError() const;
    unsigned long WriteObject(zCObject*& object);
    unsigned long ReadObject(zCObject*& object);

    zTBufferMode  mode;
    unsigned char* buffer;
    unsigned long pos_byte;
    unsigned long pos_bit;
    unsigned long maxsize_byte;
    unsigned long maxsize_bit;
    unsigned long size_byte;
    unsigned long size_bit;
    unsigned char vobNr;
    bool          allocated;
    bool          bits_written;
    bool          buffer_error;
};

int operator==(const zCBuffer& left, const zCBuffer& right);

#endif
