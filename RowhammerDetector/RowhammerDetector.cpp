#include "pin.H"
#include <map>
#include <iostream>
#include <fstream>
#include <cmath>

// #define DEBUG

std::ofstream outputFile;
std::map<ADDRINT, std::string> disassemblyMap;
std::map<ADDRINT, int> writeAddressCountMap;
int memoryWriteCount;

void CountWriteInstruction(ADDRINT instructionAddress, ADDRINT memoryAddressWrote, UINT32 memoryWriteSize)
{
#ifdef DEBUG
    printf("%s - writes at %lu - %d bytes\n",
           disassemblyMap[instructionAddress].c_str(), memoryAddressWrote, memoryWriteSize);
#endif
    writeAddressCountMap[memoryAddressWrote]++;
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

    outputFile << "Total write instructions: " << memoryWriteCount << "\n\n";
    // Output all the written addresses and their counts
    for (auto const &pair : writeAddressCountMap)
    {
        outputFile << pair.first << ':' << pair.second << "\n";
    }

    // Calculate mean of values
    double meanWritesPerAddress = static_cast<double>(memoryWriteCount) / writeAddressCountMap.size();
    outputFile << "Mean: " << meanWritesPerAddress << "\n";
    double distancesFromMeanSquaredSum = 0;
    for (auto const &pair : writeAddressCountMap)
    {
        double distance = pair.second - meanWritesPerAddress;
        distancesFromMeanSquaredSum += distance * distance;
    }
    double standardDeviation = sqrt(distancesFromMeanSquaredSum / writeAddressCountMap.size());
    outputFile << "Std dev: " << standardDeviation << "\n";
    // Check if there are any points with z-score higher than the threshold
    constexpr int zScoreThreshold = 5; // Kinda really arbitrary...
    for (auto const &pair : writeAddressCountMap)
    {
        double zScore = (pair.second - meanWritesPerAddress) / standardDeviation;
        if (zScore > zScoreThreshold)
        {
            outputFile << "Address" << pair.first << " is read " << pair.second << " times with z-score " << zScore << "\n";
        }
    }

    outputFile.close();
}

int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
}
