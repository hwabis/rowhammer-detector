#include "pin.H"
#include <map>
#include <iostream>
#include <fstream>

std::ofstream outputFile;

std::map<ADDRINT, std::string> disAssemblyMap;
int instructionCount;

void ReadsMem(ADDRINT applicationIp, ADDRINT memoryAddressRead, UINT32 memoryReadSize)
{
    printf("0x%lu %s reads %d bytes of memory at 0x%lu\n",
           applicationIp, disAssemblyMap[applicationIp].c_str(),
           memoryReadSize, memoryAddressRead);
    instructionCount++;
}

void Instruction(INS ins, void *v)
{
    if (INS_IsMemoryRead(ins))
    {
        disAssemblyMap[INS_Address(ins)] = INS_Disassemble(ins);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ReadsMem,
                       IARG_INST_PTR, // application IP
                       IARG_MEMORYREAD_EA,
                       IARG_MEMORYREAD_SIZE,
                       IARG_END);
    }

    // if (INS_IsMemoryWrite(ins))
    // {
    //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)printInstruction, IARG_PTR, ins, IARG_END);
    // }
}

void Fini(INT32 code, void *v)
{
    outputFile.open("out.log");
    outputFile << "COUNT: " << instructionCount << "\n";
    outputFile.close();
}

int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
}
