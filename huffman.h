#pragma once

#include <iostream>
#include <queue>
#include <map>
#include <algorithm>
#include <set>

#include "bufferops.h"

namespace huffman {
    template<class SymbolType, class ValueType>
    class HuffmanNode {
    private:
        HuffmanNode* nodes[2];
        bool isRoot;
        SymbolType sym;
        ValueType value = 0;
    public:
        HuffmanNode (HuffmanNode* x1, HuffmanNode* x2) {
            nodes[0] = x1;
            nodes[1] = x2;
            isRoot = false;
            value = (*x1)+(*x2);
        }

        HuffmanNode (ValueType x1, SymbolType Symbol) {
            value = x1;
            isRoot = true;
            sym = Symbol;
        }

        SymbolType GetSymbol() {
            return sym;
        }

        ValueType GetValue() {
            return value;
        }

        HuffmanNode* operator[](uint8_t index) {
            return nodes[index];
        }

        ValueType operator+(HuffmanNode& x) {
            return value+x.value;
        }

        bool operator<(HuffmanNode& x) {
            return value<x.value;
        }

        bool IsRoot() {
            return isRoot;
        }

        // useful for destructor
        void SetRoot() {
            isRoot = true;
        }

        void FillValues() {
            if (!isRoot) {
                nodes[0]->FillValues();
                nodes[1]->FillValues();
                value = (*nodes[0])+(*nodes[1]);
            }
        }
    };

    template<class T>
    bool ComparePointerObjects(T* x1, T* x2) {return (*x1)<(*x2);};

    template<class SymbolType, class ValueType>
    class RootNodeIterator;

    template<class SymbolType, class ValueType>
    class HuffmanTree {
    private:
        HuffmanNode<SymbolType, ValueType>* ParentNode;
        std::map <SymbolType, std::vector<bool>> SymbolTable;
        std::vector<HuffmanNode<SymbolType, ValueType>*> AllNodes;
    public:
        explicit HuffmanTree(const std::map<SymbolType, ValueType>& SortedFrequencyTable) {
            std::deque<HuffmanNode<SymbolType, ValueType>*> SymbolQueue;
            HuffmanNode<SymbolType, ValueType>* NodePtr;

            for (auto SymbolFrequency : SortedFrequencyTable) {
                AllNodes.push_back(new HuffmanNode<SymbolType, ValueType>(std::get<1>(SymbolFrequency), std::get<0>(SymbolFrequency)));
                SymbolQueue.push_front(AllNodes.back());
            }

            while (SymbolQueue.size() > 1) {
                std::sort(SymbolQueue.rbegin(), SymbolQueue.rend(), ComparePointerObjects<HuffmanNode<SymbolType, ValueType>>);
                //std::cout << SymbolQueue.back()->GetValue() << std::endl;
                NodePtr = SymbolQueue.back();
                SymbolQueue.pop_back();
                AllNodes.push_back(new HuffmanNode<SymbolType, ValueType>(NodePtr, SymbolQueue.back()));
                SymbolQueue.push_front(AllNodes.back());
                SymbolQueue.pop_back();
            }

            ParentNode = SymbolQueue[0];
        }

        void ConstructSymbolTable() {
            RootNodeIterator<SymbolType, ValueType> RootIterator(this);
            HuffmanNode<SymbolType, ValueType>* RootNode;
            do {
                RootNode = &RootIterator.GetCurrentRootNode();
                //std::cout << "Symbol: " << (int) RootNode->GetSymbol() << std::endl;
                SymbolTable[RootNode->GetSymbol()] = RootIterator.GetCurrentIndex();
            } while (RootIterator++);

        }

        std::map <SymbolType, std::vector<bool>> GetSymbolTable() {
            return SymbolTable;
        }

        HuffmanNode<SymbolType, ValueType>& operator[](std::vector<bool>& index) {
            HuffmanNode<SymbolType, ValueType>* ptr = ParentNode;
            for (auto branch : index) {
                ptr = (*ptr)[branch];
            }
            return (*ptr);
        }

        ~HuffmanTree() {
            for (auto ptr: AllNodes) {
                delete ptr;
            }
        }
    };

    template<class SymbolType, class ValueType>
    class RootNodeIterator {
    private:
        HuffmanTree<SymbolType, ValueType>* Tree;
        std::vector<bool> Index;
    public:
        explicit RootNodeIterator(HuffmanTree<SymbolType, ValueType>* ITree) {
            Tree = ITree;
            do {
                Index.push_back(0);
            } while (!(*Tree)[Index].IsRoot());
        }

        HuffmanNode<SymbolType, ValueType>& GetCurrentRootNode() {
            return (*Tree)[Index];
        }

        std::vector<bool> GetCurrentIndex() {
            return Index;
        }

        void DumpIndex() {
            std::cout << "Index: ";
            for (uint64_t i = 0; i < Index.size(); i++) {
                std::cout << (int) Index[i];
            }
            std::cout << std::endl << std::flush;
        }

        bool operator++(int) {
            auto LastLeftBranch = std::find(Index.rbegin(), Index.rend(), false);
            if (LastLeftBranch != Index.rend()) {
                uint64_t LeftBranch = Index.size()-1ull-(LastLeftBranch-Index.rbegin());
                Index.resize(LeftBranch+1ull);
                Index[LeftBranch] = 1;
                while (!(*Tree)[Index].IsRoot()) {
                    Index.push_back(0);
                }
                //this->DumpIndex();
                return true;
            } else {
                return false;
            }

        }

    };

    struct SetElementComparator {
        template<typename T>
        bool operator()(const T& l, const T& r) const
        {
            if (l.second != r.second)
                return l.second < r.second;

            return l.first < r.first;
        }
    };

    template<class SymbolType, class ValueType>
    void SortSTLMap(std::map<SymbolType, ValueType>& Table) {
        std::set<std::pair<SymbolType, ValueType>, SetElementComparator> set(Table.begin(), Table.end());
        for (auto pair : set) {
            Table[pair.first] = pair.second;
        }
    }

    template<class SymbolType, class ValueType>
    void PopulateFrequencyTable(std::map<SymbolType, ValueType>& SortedFrequencyTable, std::vector<SymbolType>& DataBuffer) {
        for (uint64_t i = 0; i < DataBuffer.size(); i++) {
            SortedFrequencyTable[DataBuffer[i]]++;
        }
        SortSTLMap(SortedFrequencyTable);
    }

    template<class SymbolType, class ValueType>
    void CompressBuffer(std::vector<SymbolType>& UncompressedBuffer, buffer::CodecByteStream& CompressedBuffer) {
        std::map<SymbolType, ValueType> SortedFrequencyTable;
        PopulateFrequencyTable(SortedFrequencyTable, UncompressedBuffer);
        HuffmanTree<SymbolType, ValueType> HuffTree(SortedFrequencyTable);
        HuffTree.ConstructSymbolTable();
        auto SymTable = HuffTree.GetSymbolTable();
        uint64_t index = sizeof(uint16_t)+sizeof(uint64_t);
        for (auto keyval : SortedFrequencyTable) {
            buffer::EncodeTypeToBuffer<SymbolType>(CompressedBuffer.GetBuffer(), index, &keyval.first);
            index += sizeof(SymbolType);
            buffer::EncodeTypeToBuffer<ValueType>(CompressedBuffer.GetBuffer(), index, &keyval.second);
            index += sizeof(ValueType);
        }
        auto ushrt = uint16_t(index);
        buffer::EncodeTypeToBuffer<uint16_t>(CompressedBuffer.GetBuffer(), 0, &ushrt);
        auto ull = uint64_t(UncompressedBuffer.size());
        buffer::EncodeTypeToBuffer<uint64_t>(CompressedBuffer.GetBuffer(), sizeof(uint16_t), &ull);

        std::vector<bool> code;
        for (SymbolType symbol : UncompressedBuffer) {
            code = SymTable[symbol];
            for (bool bit : code) {
                CompressedBuffer.WriteBit(bit);
            }
        }
    }

    template<class SymbolType, class ValueType>
    void UncompressBuffer(std::vector<SymbolType>& UncompressedBuffer, std::vector<unsigned char>& CompressedBuffer) {
        std::map<SymbolType, ValueType> SortedFrequencyTable;
        uint16_t mindex;
        uint64_t UncompressedSize;
        buffer::DecodeTypeFromBuffer<uint16_t>(CompressedBuffer, 0, &mindex);
        buffer::DecodeTypeFromBuffer<uint64_t>(CompressedBuffer, sizeof(uint16_t), &UncompressedSize);
        uint16_t index = sizeof(uint16_t)+sizeof(uint64_t);
        SymbolType key;
        ValueType val;

        while (index < mindex) {
            buffer::DecodeTypeFromBuffer<SymbolType>(CompressedBuffer, index, &key);
            index += sizeof(SymbolType);
            buffer::DecodeTypeFromBuffer<ValueType>(CompressedBuffer, index, &val);
            index += sizeof(ValueType);
            SortedFrequencyTable[key] = val;
        }

        HuffmanTree<SymbolType, ValueType> HuffTree(SortedFrequencyTable);

        buffer::CodecBitIterator BitStream(CompressedBuffer, index);
        std::vector<bool> TreeIndex;

        while (UncompressedBuffer.size() < UncompressedSize) {
            TreeIndex.push_back(BitStream.ReadBit());
            if (HuffTree[TreeIndex].IsRoot()) {
                UncompressedBuffer.push_back(HuffTree[TreeIndex].GetSymbol());
                TreeIndex.resize(0);
            }
        }
    }

    CodecStatusCode CompressFile(const std::string& InFile, const std::string& OutFile) {
        std::vector<unsigned char> UncompressedBuffer;

        if (buffer::LoadFile(&InFile, UncompressedBuffer)) {
            buffer::CodecByteStream CompressedBuffer(2+8+256*(1+8));
            CompressBuffer<unsigned char, uint64_t>(UncompressedBuffer, CompressedBuffer);
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
        std::vector<unsigned char> UncompressedBuffer;

        if (buffer::LoadFile(&InFile, CompressedBuffer)) {
            try {
                UncompressBuffer<unsigned char, uint64_t>(UncompressedBuffer, CompressedBuffer);
            } catch (std::exception& e) {
                std::cerr << e.what() << std::endl;
                return BadCompressionStream;
            }

            if (buffer::SaveFile(UncompressedBuffer, &OutFile)) {
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