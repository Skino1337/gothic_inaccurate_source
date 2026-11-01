#include "zBuffer.h"

#include <cstring>

zCBuffer::zCBuffer()
{
    Init();
}

zCBuffer::zCBuffer(const unsigned long& size)
{
    Init();
    buffer = new unsigned char[size];
    allocated = true;
    std::memset(buffer, 0, size);
    mode = WRITE;
    maxsize_byte = size;
    maxsize_bit = size << 3;
}

zCBuffer::zCBuffer(void* data, const unsigned long& size)
    : mode(READ), buffer(static_cast<unsigned char*>(data)), pos_byte(0), pos_bit(0),
      maxsize_byte(size), maxsize_bit(size << 3), size_byte(size), size_bit(size << 3),
      vobNr(0), allocated(false), bits_written(false), buffer_error(false)
{
}

zCBuffer::~zCBuffer()
{
    if (allocated)
        delete[] buffer;

    buffer = 0;
}

void zCBuffer::Init()
{
    mode = READWRITE;
    buffer = 0;
    pos_byte = 0;
    pos_bit = 0;
    maxsize_byte = 0;
    maxsize_bit = 0;
    size_byte = 0;
    size_bit = 0;
    vobNr = 0;
    allocated = false;
    bits_written = false;
    buffer_error = false;
}

zTBufferMode zCBuffer::SetMode(zTBufferMode newMode)
{
    const zTBufferMode oldMode = mode;
    mode = newMode;
    return oldMode;
}

void zCBuffer::Reset()
{
    pos_byte = 0;
    pos_bit = 0;
    size_byte = 0;
    size_bit = 0;
    bits_written = false;
}

void zCBuffer::Resize(unsigned long newSize)
{
    if (newSize < maxsize_byte)
        return;

    if (newSize < 0x20)
        newSize = 0x20;

    unsigned char* newBuffer = new unsigned char[newSize];
    std::memset(newBuffer, 0, newSize);

    if (buffer)
        std::memcpy(newBuffer, buffer, size_byte);

    delete[] buffer;
    buffer = newBuffer;
    maxsize_byte = newSize;
    maxsize_bit = newSize << 3;
}

unsigned long zCBuffer::Write(const void* data, unsigned long size)
{
    return (WriteBits(data, size << 3) + 7) >> 3;
}

unsigned long zCBuffer::WriteBits(const void* data, unsigned long numBits)
{
    if (buffer_error)
        return 0;

    if (pos_bit + numBits > maxsize_bit) {
        unsigned long newBitSize = (pos_bit >> 1) + pos_bit;

        if (newBitSize < numBits)
            newBitSize += numBits;

        Resize(((newBitSize + 7) >> 3) + 1);
    }

    const unsigned char* source = static_cast<const unsigned char*>(data);

    if ((pos_bit & 7) || (numBits & 7)) {
        for (unsigned long index = 0; index < numBits; ++index) {
            unsigned char& target = buffer[pos_bit >> 3];
            const unsigned char mask = 1 << (pos_bit & 7);

            if (source[index >> 3] & (1 << (index & 7))) {
                target |= mask;
            }
            else {
                target &= ~mask;
            }

            ++pos_bit;
        }
    }
    else {
        const unsigned long byteCount = (numBits + 7) >> 3;
        std::memcpy(buffer + pos_byte, source, byteCount);
        pos_bit += numBits;
    }

    pos_byte = (pos_bit + 7) >> 3;

    if (pos_bit >= size_bit) {
        size_bit = pos_bit;
        size_byte = pos_byte;
    }

    return numBits;
}

unsigned long zCBuffer::Read(void* data, unsigned long size)
{
    return (ReadBits(data, size << 3) + 7) >> 3;
}

unsigned long zCBuffer::ReadBits(void* data, unsigned long numBits)
{
    unsigned char* target = static_cast<unsigned char*>(data);

    if ((pos_bit & 7) || (numBits & 7)) {
        for (unsigned long index = 0; index < numBits; ++index) {
            const unsigned char mask = 1 << (index & 7);

            if (buffer[pos_bit >> 3] & (1 << (pos_bit & 7))) {
                target[index >> 3] |= mask;
            }
            else {
                target[index >> 3] &= ~mask;
            }

            ++pos_bit;
        }
    }
    else {
        const unsigned long byteCount = (numBits + 7) >> 3;
        std::memcpy(target, buffer + pos_byte, byteCount);
        pos_bit += numBits;
    }

    pos_byte = (pos_bit + 7) >> 3;
    return numBits;
}

unsigned long zCBuffer::WriteBool(int value, int numBits)
{
    return WriteBits(&value, numBits);
}

unsigned long zCBuffer::WriteByte(unsigned char value, int numBits)
{
    return WriteBits(&value, numBits);
}

unsigned long zCBuffer::WriteWord(unsigned short value, int numBits)
{
    return WriteBits(&value, numBits);
}

unsigned long zCBuffer::WriteDWord(unsigned long value, int numBits)
{
    return WriteBits(&value, numBits);
}

unsigned long zCBuffer::WriteReal(float value, int numBits)
{
    return WriteBits(&value, numBits);
}

unsigned long zCBuffer::WriteInt(int value, int numBits)
{
    return WriteBits(&value, numBits);
}

unsigned long zCBuffer::WriteFloat(float value, int numBits)
{
    return WriteBits(&value, numBits);
}

int zCBuffer::ReadBool(int numBits)
{
    int value = 0;
    ReadBits(&value, numBits);
    return value;
}

unsigned char zCBuffer::ReadByte(int numBits)
{
    unsigned char value = 0;
    ReadBits(&value, numBits);
    return value;
}

unsigned short zCBuffer::ReadWord(int numBits)
{
    unsigned short value = 0;
    ReadBits(&value, numBits);
    return value;
}

unsigned long zCBuffer::ReadDWord(int numBits)
{
    unsigned long value = 0;
    ReadBits(&value, numBits);
    return value;
}

float zCBuffer::ReadReal(int numBits)
{
    float value = 0.0f;
    ReadBits(&value, numBits);
    return value;
}

int zCBuffer::ReadInt(int numBits)
{
    int value = 0;
    ReadBits(&value, numBits);
    return value;
}

float zCBuffer::ReadFloat(int numBits)
{
    float value = 0.0f;
    ReadBits(&value, numBits);
    return value;
}

unsigned long zCBuffer::WriteFloatPacked0_16(float value)
{
    const int packed = static_cast<int>(value * 65535.0f);
    return (WriteBits(&packed, 16) + 7) >> 3;
}

unsigned long zCBuffer::WriteFloatPacked0_8(float value)
{
    const int packed = static_cast<int>(value * 255.0f);
    return (WriteBits(&packed, 8) + 7) >> 3;
}

unsigned long zCBuffer::WriteFloatPacked8_8(float value)
{
    if (value > 127.0f) {
        value = 127.0f;
    }
    else if (value < -127.0f) {
        value = -127.0f;
    }

    const int packed = static_cast<int>(value * 256.0f);
    return (WriteBits(&packed, 16) + 7) >> 3;
}

float zCBuffer::ReadFloatPacked0_16()
{
    unsigned short value = 0;
    ReadBits(&value, 16);
    return value * 1.52590219e-05f;
}

float zCBuffer::ReadFloatPacked0_8()
{
    unsigned char value = 0;
    ReadBits(&value, 8);
    return value * 0.00392156886f;
}

float zCBuffer::ReadFloatPacked8_8()
{
    short value = 0;
    ReadBits(&value, 16);
    return value * 0.00390625f;
}

unsigned long zCBuffer::WriteString(zSTRING value)
{
    return (WriteBits(value.ToChar(), (value.Length() << 3) + 8) + 7) >> 3;
}

void zCBuffer::CompleteByte()
{
    int value = 0;
    WriteBits(&value, pos_byte - ((pos_bit + 7) >> 3));
}

zSTRING zCBuffer::ReadString()
{
    zSTRING result;

    while (true) {
        const char value = ReadByte(8);

        if (!value)
            break;

        result.Insert(result.Length(), zSTRING(value));
    }

    return result;
}

void zCBuffer::SetPos(const unsigned long& position)
{
    pos_bit = position << 3;
    pos_byte = (pos_bit + 7) >> 3;
}

void zCBuffer::SetBitPos(const unsigned long& position)
{
    pos_bit = position;
    pos_byte = (pos_bit + 7) >> 3;
}

unsigned long zCBuffer::SetPosRel(int offset)
{
    pos_bit += offset << 3;
    pos_byte = (pos_bit + 7) >> 3;
    return pos_byte;
}

unsigned long zCBuffer::SetBitPosRel(int offset)
{
    pos_bit += offset;
    pos_byte = (pos_bit + 7) >> 3;
    return pos_bit;
}

void zCBuffer::SetPosBegin()
{
    pos_bit = 0;
    pos_byte = 0;
}

void zCBuffer::SetPosEnd()
{
    pos_bit = size_byte << 3;
    pos_byte = (pos_bit + 7) >> 3;
}

unsigned long zCBuffer::GetPos() const
{
    return pos_byte;
}

unsigned long zCBuffer::GetBitPos() const
{
    return pos_bit;
}

unsigned long zCBuffer::GetMaxSize() const
{
    return maxsize_byte;
}

unsigned long zCBuffer::GetBitMaxSize() const
{
    return maxsize_bit;
}

unsigned long zCBuffer::GetSize() const
{
    return size_byte;
}

unsigned long zCBuffer::GetBitSize() const
{
    return size_bit;
}

unsigned long zCBuffer::GetRestSize() const
{
    return maxsize_byte - pos_byte;
}

unsigned long zCBuffer::GetBitRestSize() const
{
    return maxsize_bit - pos_bit;
}

unsigned char* zCBuffer::GetBuffer() const
{
    return buffer;
}

unsigned char* zCBuffer::GetCursor() const
{
    return buffer + pos_byte;
}

void zCBuffer::SkipByte()
{
    const unsigned long bits = pos_byte - ((pos_bit + 7) >> 3);

    if (!bits)
        return;

    pos_bit += bits;
    pos_byte = (pos_bit + 7) >> 3;
}

int zCBuffer::GetBitSizeToNextByte() const
{
    return pos_byte - ((pos_bit + 7) >> 3);
}

bool zCBuffer::HasError() const
{
    return buffer_error;
}

unsigned long zCBuffer::WriteObject(zCObject*& object)
{
    ++vobNr;
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(0, 0, 1, 0);
    archiver->WriteObject(object);
    zCBuffer* objectBuffer = archiver->GetBuffer();
    const unsigned long size = objectBuffer->size_byte;
    WriteBits(&size, 32);
    WriteBits(objectBuffer->buffer, size << 3);
    archiver->Close();
    archiver->Release();
    return size;
}

unsigned long zCBuffer::ReadObject(zCObject*& object)
{
    ++vobNr;
    unsigned long size = 0;
    ReadBits(&size, 32);
    zCBuffer objectBuffer(buffer + pos_byte, size);
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(&objectBuffer, 0);
    object = archiver->ReadObject(0);
    archiver->Close();
    archiver->Release();
    return size;
}

zCBuffer::zCBuffer(const zCBuffer& other)
{
    Init();
    CopyFrom(other);
}

zCBuffer& zCBuffer::operator=(const zCBuffer& other)
{
    if (this != &other)
        CopyFrom(other);

    return *this;
}

void zCBuffer::CopyFrom(const zCBuffer& other)
{
    delete[] buffer;
    buffer = new unsigned char[other.size_byte];
    allocated = true;
    std::memcpy(buffer, other.buffer, other.size_byte);
    mode = other.mode;
    maxsize_byte = other.maxsize_byte;
    maxsize_bit = other.maxsize_bit;
    bits_written = other.bits_written;
    buffer_error = other.buffer_error;
    pos_bit = other.pos_bit;
    pos_byte = other.pos_byte;
    size_bit = other.size_bit;
    size_byte = other.size_byte;
    vobNr = other.vobNr;
}

static unsigned long byteFrequency[256];

void zCBuffer::ByteFreq_Reset()
{
    std::memset(byteFrequency, 0, sizeof(byteFrequency));
}

void zCBuffer::ByteFreq_Analyse()
{
    for (unsigned long index = 0; index < size_byte; ++index)
        ++byteFrequency[buffer[index]];
}

void zCBuffer::ByteFreq_Print()
{
    zerr->Message(zSTRING("B: BUF: Byte-Frequency"));
    zSTRING message;
    for (int count = 0; count < 256; ++count) {
        int bestByte = 0;
        long bestFrequency = -1;
        for (int value = 0; value < 256; ++value) {
            if ((long)byteFrequency[value] >= bestFrequency) {
                bestByte = value;
                bestFrequency = (long)byteFrequency[value];
            }
        }
        zSTRING frequency = zSTRING(bestFrequency).Align(zSTR_RIGHT, 3, spaceChar);
        zSTRING byte = zSTRING(bestByte).Align(zSTR_LEFT, 2, spaceChar);
        message += frequency + "  " + byte + "=";
        byteFrequency[bestByte] = 0xFFFFFFFF;
        if (!((count + 1) % 9)) {
            zerr->Message("B: BUF: " + message);
            message.Clear();
        }
    }
}

int operator==(const zCBuffer& left, const zCBuffer& right)
{
    const int result = std::memcmp(left.buffer, right.buffer, left.size_byte);
    return (result > 0) - (result < 0);
}
