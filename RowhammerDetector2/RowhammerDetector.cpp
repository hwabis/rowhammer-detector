#include "pin.H"
#include <map>
#include <iostream>
#include <fstream>
#include <queue>
#include <set>
#include <algorithm>
#include <math.h>

// #define DEBUG

// The maximum number of other write instructions between write instructions to the same address that is allowed
// for it to count as consecutively writing to that address (does that make sense...)
constexpr int MAXIMUM_INSTRUCTION_GAP = 4;

struct AddressHistory
{
    int consecutiveWriteCount;
    int allowedInstructionsRemaining;
};

std::ofstream outputFile;
std::map<ADDRINT, std::string> disassemblyMap;

std::map<ADDRINT, AddressHistory> addressHistoryMap; // Tracks all addresses that have been written to in the past `maximumInstructionGap` writes
int maxConsecutiveWrites = 0;

std::map<ADDRINT, std::string> writeDataMap; // Key is address being written to, value is the string of bytes being written
double totalProportionOfFlippedBits = 0;
int memoryWriteCount = 0;

void CountWriteInstruction(ADDRINT instructionAddress, ADDRINT memoryAddressWrote, UINT32 memoryWriteSize)
{
    unsigned char previousData[memoryWriteSize]; // oh my god this needs to be unsigned... i wasted hours
    PIN_SafeCopy(previousData, (VOID *)memoryAddressWrote, memoryWriteSize);

    // Compare byte-by-byte the number of flipped bits. So at this point,
    // writeDataMap stores the bytes from two instructions ago, and previousData is from one instruction ago
    unsigned char *mapDataCopy = new unsigned char[writeDataMap[memoryAddressWrote].size()];
    std::copy(writeDataMap[memoryAddressWrote].begin(), writeDataMap[memoryAddressWrote].end(), mapDataCopy);
    long unsigned flippedBitsCount = 0;
    long unsigned maxPossibleFlippedBitsCount = 0;
    for (UINT32 i = 0; i < std::min<size_t>(memoryWriteSize, writeDataMap[memoryAddressWrote].size()); i++)
    {
        unsigned char previousDataByte = previousData[i];
        unsigned char mapDataByte = mapDataCopy[i];

        unsigned char XORByte = previousDataByte ^ mapDataByte;
        for (int i = 0; i < 8; i++)
        {
            if ((XORByte >> i) & 1)
            {
                flippedBitsCount++;
            }
        }
        maxPossibleFlippedBitsCount += 8;
    }
    double proportionOfFlippedBits = static_cast<double>(flippedBitsCount) / maxPossibleFlippedBitsCount;
    if (!isnan(proportionOfFlippedBits))
    {
        totalProportionOfFlippedBits += proportionOfFlippedBits;
    }

    std::string previousDataString(reinterpret_cast<char *>(previousData), memoryWriteSize);
    writeDataMap[memoryAddressWrote] = previousDataString;

#ifdef DEBUG
    printf("Flipped bits: %lu/%lu\n", flippedBitsCount, maxPossibleFlippedBitsCount);
    printf("Data was previously: ");
    for (unsigned int i = 0; i < memoryWriteSize; i++)
    {
        printf("%02x", previousData[i]);
    }
    printf("\n");
    printf("%s - writes at %lu - %d bytes\n",
           disassemblyMap[instructionAddress].c_str(), memoryAddressWrote, memoryWriteSize);
    // No idea why %s is suddenly printing out some garbage.
    // At least we still have the constant data that's being written
#endif

    auto iter = addressHistoryMap.find(memoryAddressWrote);
    if (iter != addressHistoryMap.end())
    {
        // We found the element
        AddressHistory &entry = addressHistoryMap.at(iter->first);
        entry.consecutiveWriteCount += 1;
        entry.allowedInstructionsRemaining = MAXIMUM_INSTRUCTION_GAP; // reset it
        if (entry.consecutiveWriteCount > maxConsecutiveWrites)
        {
            maxConsecutiveWrites = entry.consecutiveWriteCount;
        }
    }
    else
    {
        AddressHistory newEntry = {1, MAXIMUM_INSTRUCTION_GAP};
        addressHistoryMap[memoryAddressWrote] = newEntry;
    }

    // Go through every element, decrementallowedInstructionsRemaining, and remove any that have fallen out
    iter = addressHistoryMap.begin();
    while (iter != addressHistoryMap.end())
    {
        AddressHistory &entry = iter->second;
        entry.allowedInstructionsRemaining--;
        if (entry.allowedInstructionsRemaining < 0)
        {
            iter = addressHistoryMap.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    memoryWriteCount++;
}

void Instruction(INS ins, void *v)
{
    if (INS_IsMemoryWrite(ins))
    {
        disassemblyMap[INS_Address(ins)] = INS_Disassemble(ins);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountWriteInstruction,
                       IARG_INST_PTR,
                       IARG_MEMORYWRITE_EA,
                       IARG_MEMORYWRITE_SIZE,
                       IARG_END);
    }
}

void Fini(INT32 code, void *v)
{
    outputFile.open("out.log");

    outputFile << "An address was written this many times in a row: " << maxConsecutiveWrites << "\n";
    outputFile << "In that address, the average proportion of flipped bits was: " << totalProportionOfFlippedBits / memoryWriteCount << "\n";

    outputFile.close();
}

int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
}
