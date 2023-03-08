#include "pin.H"
#include <map>
#include <iostream>
#include <fstream>
#include <queue>
#include <set>

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

void CountWriteInstruction(ADDRINT instructionAddress, ADDRINT memoryAddressWrote, UINT32 memoryWriteSize)
{
#ifdef DEBUG
    printf("%s - writes at %lu - %d bytes\n",
           disassemblyMap[instructionAddress].c_str(), memoryAddressWrote, memoryWriteSize);
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

    outputFile.close();
}

int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
}
