#pragma once

#include <iostream>
#include <utility>
#include <vector>
#include <cmath>
#include <bitset>
#include <cstring>
#include <algorithm>
#include <chrono>

#include "bufferops.h"

namespace arith {
    void ComputeProbabilities(uint32_t* ProbabilityTable, std::vector<unsigned char>& UncompressedBuffer, uint8_t BitShift) {
        memset(ProbabilityTable, 0u, 256*4);
        uint8_t val;

        for (uint32_t i = 0; i < MIN(UncompressedBuffer.size() - 1, UINT32_MAX); i++) {
            val = ((uint8_t) UncompressedBuffer[i] << BitShift) +
                  ((uint8_t) UncompressedBuffer[i + 1] >> (8 - BitShift));
            ProbabilityTable[val]++;
        }
    }

    double ArithmeticMean(const uint32_t* Data, uint64_t Size) {
        double sum = 0;

        for (uint64_t i = 0; i < Size; i++) {
            sum += double(*(Data+i));
        }

        return sum / (double) Size;
    }

    double StandardDeviation(const uint32_t* Data, uint64_t Size) {
        double mean = ArithmeticMean(Data, Size);
        double sum = 0;

        for (uint64_t i = 0; i < Size; i++) {
            sum += pow((double(*(Data+i)) - mean), 2);
        }

        return sqrt(sum / ((double) Size - 1));
    }

    class CodecProbabilityTable {
    private:
        uint32_t Frequencies[257] = {0};
        uint8_t BitShift;
    public:
        void GenerateTable(std::vector<unsigned char>& UncompressedBuffer) {
            uint32_t tables[256*8] = {0};
            double deviations[8] = {0};

            for (uint16_t i = 0; i < 8; i++) {
                ComputeProbabilities(&tables[i*256], UncompressedBuffer, (uint8_t) i);
                deviations[i] = StandardDeviation(&tables[i*256], 256);
            }

            uint16_t max = 0u;
            double pmax = 0;

            for (uint16_t i = 0; i < 8; i++) {
                if (deviations[i] > pmax) {
                    pmax = deviations[i];
                    max = i;
                }
            }

            std::copy(tables+256*max, tables+256*(max+1), Frequencies + 1);

            for (uint16_t i = 1; i < 257; i++) {
                Frequencies[i] += Frequencies[i-1];
            }

            BitShift = max;
        }

        void DumpTable() {
            std::cout.precision(3);
            std::cout << std::scientific;

            for (int i = 0; i < 16; i++) {
                for (int j = 0; j < 15; j++) {
                    std::cout << double(*(Frequencies + i * 16 + j + 1)) / double(Frequencies[256]) << " ";
                }
                std::cout << double(*(Frequencies + i * 16 + 15 + 1)) / double(Frequencies[256]) << std::endl;
            }
        }

        void DumpNumerators() {
            for (int i = 0; i < 16; i++) {
                for (int j = 0; j < 15; j++) {
                    std::cout << (int) *(Frequencies + i * 16 + j + 1) << " ";
                }
                std::cout << (int) *(Frequencies + i * 16 + 15 + 1) << std::endl;
            }
        }

        std::tuple<uint32_t, uint32_t, uint8_t> DecodeFromCount(uint64_t count) {
            for (uint16_t i = 0; i < 256; i++) {
                if (Frequencies[i+1] > count) {
                    return std::make_tuple(Frequencies[i], Frequencies[i+1], i);
                }
            }
            throw std::runtime_error("Bad Value Encountered In Decode.");
        }

        uint8_t GetShift() {
            return BitShift;
        }

        uint32_t GetDenom() {
            return Frequencies[256];
        }

        std::tuple<uint32_t, uint32_t, uint32_t> GetProbability(unsigned char Byte) {
            return std::make_tuple(Frequencies[Byte], Frequencies[uint16_t(Byte) + 1u], Frequencies[256]);
        }

        void EncodeToBuffer(std::vector<unsigned char>& Buffer, uint64_t StartIndex) {
            for (int i = 1; i < 257; i++) {
                buffer::EncodeTypeToBuffer<uint32_t>(Buffer, StartIndex+4*(i-1), &Frequencies[i]);
            }
        }

        void DecodeFromBuffer(std::vector<unsigned char>& Buffer, uint64_t StartIndex) {
            for (int i = 1; i < 257; i++) {
                buffer::DecodeTypeFromBuffer<uint32_t>(Buffer, StartIndex+4*(i-1), &Frequencies[i]);
            }
        }
    };

    const uint64_t MAXVAL = UINT32_MAX; // ONLY USING 32 BITS OUT OF 64
    const uint64_t QUARTER = (MAXVAL >> 2ull) + 1ull;
    const uint64_t HALF = QUARTER*2ull;
    const uint64_t THREE_QUARTERS = QUARTER*3ull;


    void CompressBuffer(std::vector<unsigned char>& InputBuffer,
                        CodecProbabilityTable& ProbabilityTable,
                        buffer::CodecByteStream& OutputBuffer) {
        uint64_t high = MAXVAL;
        uint64_t low = 0ull;
        uint8_t shift = ProbabilityTable.GetShift();
        buffer::CodecBufferWrapper Buffer(InputBuffer, shift);
        OutputBuffer.WriteByte(shift);
        OutputBuffer.WriteByte((InputBuffer[InputBuffer.size()-1]<<shift)+(InputBuffer[0]>>(8u-shift))); // residual due to shift
        uint64_t pLow, pUp, pDenom, range;

        // Rate Stuff
        auto StartTime = std::chrono::high_resolution_clock::now();
        uint64_t BufSizePct = Buffer.Size()/100ull;


        for (uint64_t i = 0; i < Buffer.Size(); i++) {

            if ((i<<44ll>>44ull) == 0ull) {
                std::cout << "\33[2K\r";
                std::cout << "Compressing... " << i/BufSizePct+1 << "%  @" << double(i)/std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - StartTime).count() << " Bytes/Second." << std::flush;
            }

            range = high - low + 1ull;
            std::tie(pLow, pUp, pDenom) = ProbabilityTable.GetProbability(Buffer[i]);
            high = low + (range * pUp / pDenom) - 1ull;
            low = low + (range * pLow / pDenom);

            while (true) {
                if (high < HALF) {
                    OutputBuffer.WriteBitBuffered(0u);
                } else if (low >= HALF) {
                    OutputBuffer.WriteBitBuffered(1u);
                } else if (high < THREE_QUARTERS && low >= QUARTER) {
                    OutputBuffer.IncPendingBits();
                    low -= QUARTER;
                    high -= QUARTER;
                } else {
                    break;
                }
                high <<= 1ull;
                high++;
                low <<= 1ull;
                high &= MAXVAL;
                low &= MAXVAL;
            }
        }

        OutputBuffer.IncPendingBits();
        if (low < QUARTER) {
            OutputBuffer.WriteBitBuffered(0);
        } else {
            OutputBuffer.WriteBitBuffered(1);
        }

        std::cout << std::endl;
    }

    void UncompressBuffer(std::vector<unsigned char>& InputBuffer,
                          CodecProbabilityTable& ProbabilityTable,
                          buffer::CodecByteStream& OutputBuffer,
                          uint64_t UncompressedSize) {
        uint8_t shift = InputBuffer[256*4+8];
        uint8_t residual_byte = InputBuffer[256*4+9];
        buffer::CodecBitIterator Buffer(InputBuffer, 256*4+10);
        uint64_t high = MAXVAL;
        uint64_t low = 0ull;
        uint64_t value = 0ull;
        uint64_t range, denom, count, pLow, pUp;
        denom = ProbabilityTable.GetDenom();
        uint8_t byte;

        // Rate Stuff
        auto StartTime = std::chrono::high_resolution_clock::now();
        uint64_t StartSize = UncompressedSize;
        uint64_t BufSizePct = UncompressedSize/100ull;

        if (shift != 0) {
            for (uint8_t i = 0; i < shift; i++) {
                OutputBuffer.WriteBit((residual_byte>>(shift-i-1u))&1u);
            }
        }

        for (uint8_t i = 0; i < 32; i++) {
            value <<= 1ull;
            value += Buffer.ReadBit();
        }

        while (UncompressedSize > 1) {

            if ((UncompressedSize<<44ll>>44ull) == 0ull) {
                std::cout << "\33[2K\r";
                std::cout << "Uncompressing... " << 101ull-UncompressedSize/BufSizePct << "%  @" << (StartSize-UncompressedSize)/std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - StartTime).count() << " Bytes/Second." << std::flush;
            }

            range = high-low+1ull;
            count = ((value - low + 1ull) * denom - 1ull) / range;
            std::tie(pLow, pUp, byte) = ProbabilityTable.DecodeFromCount(count);
            OutputBuffer.WriteByte(byte);
            high = low + (range*pUp)/denom - 1ull;
            low = low + (range*pLow)/denom;

            while (true) {
                if (high < HALF) {
                    //pass;
                } else if (low >= HALF) {
                    value -= HALF;
                    low -= HALF;
                    high -= HALF;
                } else if (high < THREE_QUARTERS && low >= QUARTER) {
                    value -= QUARTER;
                    low -= QUARTER;
                    high -= QUARTER;
                } else {
                    break;
                }
                low <<= 1ull;
                high <<= 1ull;
                high++;
                value <<= 1ull;
                value += Buffer.ReadBit();
            }


            UncompressedSize--;
        }

        for (uint8_t i = 0; i < 8u-shift; i++) {
            OutputBuffer.WriteBit((residual_byte>>(7u-i))&1u);
        }

        std::cout << std::endl;
    }

    // Buffer Compression/Decompression API
    CodecStatusCode CompressBuffer(std::vector<unsigned char>& UncompressedBuffer, buffer::CodecByteStream& CompressedBuffer) {
        std::cout << "Initializing Compressor..." << std::endl;
        CodecProbabilityTable ProbabilityTable;
        ProbabilityTable.GenerateTable(UncompressedBuffer);
        CompressBuffer(UncompressedBuffer, ProbabilityTable, CompressedBuffer);
        uint64_t UncompressedBufferSize = UncompressedBuffer.size();
        buffer::EncodeTypeToBuffer<uint64_t>(CompressedBuffer.GetBuffer(), 0, &UncompressedBufferSize);
        ProbabilityTable.EncodeToBuffer(CompressedBuffer.GetBuffer(), 8);
        return Success;
    }

    CodecStatusCode UncompressBuffer(buffer::CodecByteStream& UncompressedBuffer, std::vector<unsigned char>& CompressedBuffer) {
        std::cout << "Initializing Uncompressor..." << std::endl;
        uint64_t UncompressedBufferSize;
        CodecProbabilityTable ProbabilityTable;

        buffer::DecodeTypeFromBuffer<uint64_t>(CompressedBuffer, 0, &UncompressedBufferSize);
        ProbabilityTable.DecodeFromBuffer(CompressedBuffer, 8);

        UncompressBuffer(CompressedBuffer,
                         ProbabilityTable,
                         UncompressedBuffer,
                         UncompressedBufferSize);

        UncompressedBuffer.TruncateOne();

        return Success;
    }

    // File Compression/Decompression API
    CodecStatusCode CompressFile(const std::string& InFile, const std::string& OutFile) {
        std::vector<unsigned char> UncompressedBuffer;

        if (buffer::LoadFile(&InFile, UncompressedBuffer)) {
            buffer::CodecByteStream CompressedBuffer(256*4+8);
            arith::CompressBuffer(UncompressedBuffer, CompressedBuffer);
            if (buffer::SaveFile(CompressedBuffer.GetBuffer(), &OutFile)) {
                return Success;
            } else {
                std::cerr << "File Write Error." << std::endl;
                return FileWriteError;
            }
        } else {
            std::cerr << "File Read Error." << std::endl;
            return FileReadError;
        }
    }

    CodecStatusCode UncompressFile(const std::string& InFile, const std::string& OutFile) {
        std::vector<unsigned char> CompressedBuffer;

        if (buffer::LoadFile(&InFile, CompressedBuffer)) {
            buffer::CodecByteStream UncompressedBuffer(0);
            try {
                arith::UncompressBuffer(UncompressedBuffer, CompressedBuffer);
            } catch (std::exception& e) {
                std::cerr << e.what() << std::endl;
                return BadCompressionStream;
            }

            if (buffer::SaveFile(UncompressedBuffer.GetBuffer(), &OutFile)) {
                return Success;
            } else {
                std::cerr << "File Write Error." << std::endl;
                return FileWriteError;
            }
        } else {
            std::cerr << "File Read Error." << std::endl;
            return FileReadError;
        }
    }
}
