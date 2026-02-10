/*
    With VSCode, if you like to use CMake, the project can be built with the following command. 
    <PowerShell terminal>
        PS D:\Project\riscv_trace_control_interface\build> cmake -S .. -B .
        MSBuild.exe .\tci.vcxproj
    It builds the project and the .exe file is found under ./Debug/tci.exe
*/


#include "TraceSystem.h"
#include "TraceControllerInterface.h"
#include <iostream>
#include <vector>
#include <cstdint>

using namespace tci;

int main(){
    TraceSystem trSystem;
    tci::TraceControllerInterface trController(trSystem.mmioBus, TraceSystem::TR_TE_BASE, 
        TraceSystem::TR_FUNNEL_BASE, TraceSystem::TR_RAM_SINK_BASE);

    trController.configure();
    trController.start();
    trSystem.encoder.emitInstr(0x2000, 0xDEADBEEF); // example pc and opcode
    trSystem.encoder.emitInstr(0x2004, 0xCAFEBABE); // another example pc and opcode

    // CHANGE FETCH() - we fetch in bytes or words?
    std::vector<uint32_t> bytesFetched = trController.fetch(32); // fetch 16 bytes of trace data
    
    
    trController.stop();

    std::cout << "Fetched trace data words: ";
    for (const auto& word : bytesFetched) {
        std::cout << std::hex << word << " ";
    }
    std::cout << std::dec << std::endl; // reset to decimal

    std::cout << "Main function finished execution" << std::endl;
    return 0;
}