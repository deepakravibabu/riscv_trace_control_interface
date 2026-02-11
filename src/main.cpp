/*
    With VSCode, if you like to use CMake, the project can be built with the following command. 
    <PowerShell terminal>
        PS D:\Project\riscv_trace_control_interface> cmake -S . -B .\build\
        MSBuild.exe .\build\tci.vcxproj
        .\build\Debug\tci.exe
*/


#include <iostream>
#include <vector>
#include <cstdint>

#include "TraceSystem.h"
#include "TraceControllerInterface.h"
#include "ProbeHwAccess.h"


using namespace tci;

int main() {
    // Instantiate the TraceSystem and TraceControllerInterface
    TraceSystem trSystem;

    // Create a ProbeHwAccess that uses the TraceSystem's MMIO bus
    ProbeHwAccess probeAccess(trSystem.mmioBus);

    // TraceControllerInterface takes the probe access(ReadMemory and WriteMemory stubs) and the base addresses of the components
    tci::TraceControllerInterface trController(probeAccess, TraceSystem::TR_TE_BASE, 
        TraceSystem::TR_FUNNEL_BASE, TraceSystem::TR_RAM_SINK_BASE);

    // Configure the trace system via the TraceControllerInterface
    trController.configure();

    // Simulate trace generation by setting TraceEncoder active and emitting some trace data
    trController.start();

    // In a real system, the TraceEncoder would emit trace data based on the execution of instructions.
    // For this simulation, emitTrace() is called to generate some trace data.
    trSystem.encoder.emitTrace(0x3000, 0xDEADBEEF); // example pc and opcode
    trSystem.encoder.emitTrace(0x3004, 0xCAFEBABE); // another example pc and opcode

    // Stop the trace system 
    trController.stop();

    // Fetch trace data from TraceRamSink via TraceControllerInterface
    std::vector<uint32_t> wordsFetched = trController.fetch(4); // fetch trace data n words from TraceRamSink
    
    // Print fetched trace data
    std::cout << "Fetched trace data words: ";
    for (const auto& word : wordsFetched) {
        std::cout << std::hex << word << " ";
    }
    std::cout << std::dec << std::endl; // reset to decimal

    std::cout << "Main function finished execution" << std::endl;
    return 0;
}