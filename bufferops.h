#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>

// MACRO DEFINITIONS
#define MIN(x, y) ((x)<=(y)?(x):(y))
#define MAX(x, y) ((x)>=(y)?(x):(y))
// END MACRO DEFINITIONS

// ENUM DEFINITIONS
enum CodecStatusCode {Success, FileReadError, FileWriteError, BadCompressionStream};
// END ENUM DEFINITIONS

namespace buffer {
    bool LoadFile(const std::string* path, std::vector<unsigned char>& buffer) {
        std::ifstream file(path->c_str(), std::fstream::binary);

        if (file) {
            std::noskipws(file);

            file.seekg(0, file.end);
            uint64_t len = file.tellg();
            file.seekg(0, file.beg);

            buffer.resize(len, 0u);
            std::copy(std::istream_iterator<unsigned char>(file),
                      std::istream_iterator<unsigned char>(),
                      &buffer[0]);

            file.close();
            return true;
        }

        return false;
    }

    bool SaveFile(const std::vector<unsigned char>& Buffer, const std::string *Path) {
        std::ofstream file(Path->c_str(), std::fstream::binary);
        std::noskipws(file);
        file.write(reinterpret_cast<const char *>(&Buffer[0]), Buffer.size());
        file.close();
        return file ? true : false;
    }

    template<class Type>
    void EncodeTypeToBuffer(std::vector<unsigned char>& Buffer, const uint64_t Index, const Type* Value) {
        auto* byteptr = reinterpret_cast<const unsigned char*>(Value);
        for (uint64_t i = 0; i < sizeof(Type); i++) {
            Buffer[Index + i] = byteptr[i];
        }
    }

    template<class Type>
    void DecodeTypeFromBuffer(std::vector<unsigned char>& Buffer, const uint64_t Index, Type* DecodedValue) {
        auto* byteptr = reinterpret_cast<unsigned char*>(DecodedValue);
        for (uint64_t i = 0; i < sizeof(Type); i++) {
            byteptr[i] = Buffer[Index + i];
        }
    }

    class CodecByteStream {
    private:
        std::vector<unsigned char> Data;
        uint8_t IBitIndex = 7u;
        uint64_t ByteIndex;
        int PendingBits = 0;
    public:
        CodecByteStream(uint64_t BaseLength) {
            Data.resize(BaseLength+1, 0u);
            ByteIndex = BaseLength;
        }

        void WriteByte(uint8_t Byte) {
            Data[ByteIndex] |= Byte>>(7u-IBitIndex);
            Data.push_back(Byte<<(IBitIndex+1u));
            ByteIndex++;
        }

        void WriteBit(uint8_t Bit) {
            Data[ByteIndex] |= Bit<<IBitIndex;
            if (IBitIndex == 0) {
                ByteIndex++;
                Data.push_back(0u);
                IBitIndex = 7u;
            } else {
                IBitIndex--;
            }
        }

        void WriteBitBuffered(uint8_t Bit) {
            WriteBit(Bit);
            Bit ^= 1u;
            while (PendingBits > 0) {
                WriteBit(Bit);
                PendingBits--;
            }
        }

        void IncPendingBits() {
            PendingBits++;
        }

        void TruncateOne() {
            Data.resize(Data.size()-1);
            ByteIndex--;
            IBitIndex = 7u;
        }

        std::vector<unsigned char>& GetBuffer() {
            return Data;
        }

        unsigned char& operator[](uint64_t index) {
            return Data[index];
        }
    };

    class CodecBufferWrapper {
    private:
        std::vector<unsigned char> Buffer;
        uint8_t BitShift;
        uint8_t IBitShift;
        uint64_t EffectiveSize;
    public:
        CodecBufferWrapper(std::vector<unsigned char>& InBuffer, uint8_t Shift) {
            Buffer = InBuffer;
            BitShift = Shift;
            IBitShift = 8u-Shift;
            EffectiveSize = Buffer.size() - (Shift == 0 ? 0 : 1); // If a shift is used, the buffer size effectively decreases by one
        }

        uint8_t operator[](uint64_t Index) {
            return (Buffer[Index]<<BitShift)+(Buffer[Index+1u]>>IBitShift);
        }

        uint64_t Size() {
            return EffectiveSize;
        }

        std::vector<unsigned char>& GetBuffer() {
            return Buffer;
        }
    };

    class CodecBitIterator {
    private:
        std::vector<unsigned char> Buffer;
        uint64_t ByteIndex;
        uint64_t BitIndex = 7u;
    public:
        CodecBitIterator(std::vector<unsigned char> Buf, uint64_t StartIndex) {
            Buffer = std::move(Buf);
            ByteIndex = StartIndex;
        }

        uint8_t ReadBit() {
            uint8_t bit = (Buffer[ByteIndex]>>BitIndex)&1u;
            if (BitIndex == 0u) {
                BitIndex = 7u;
                ByteIndex++;
            } else {
                BitIndex--;
            }
            return bit;
        }
    };
}