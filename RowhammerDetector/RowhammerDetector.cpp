#include "pin.H"
#include <map>
#include <iostream>
#include <fstream>

std::map<ADDRINT, std::string> disassemblyMap;

std::ofstream outputFile;
int memoryWriteCount;

void CountWriteInstruction(ADDRINT instructionAddress, ADDRINT memoryAddressWrote, UINT32 memoryWriteSize)
{
    printf("%s - writes at 0x%lu - %d bytes\n",
           disassemblyMap[instructionAddress].c_str(), memoryAddressWrote, memoryWriteSize);
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
    outputFile << "COUNT: " << memoryWriteCount << "\n";
    outputFile.close();
}

int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
}
