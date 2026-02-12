/*
    With VSCode, if you like to use CMake, the project can be built with the following command. 
    <PowerShell terminal>
        PS D:\Project\riscv_trace_control_interface> cmake -S . -B .\build\
        MSBuild.exe .\build\tci.vcxproj
        .\build\Debug\tci.exe
*/

/* 
    Bash Terminal
        Set GoogleTests OFF and Build 'tci_demo' exec
        cmake -S . -B build -DTCI_BUILD_GTESTS=OFF
        cmake --build build --target tci_demo
        ./build/Debug/tci_demo

        Set GoogleTests ON - Built both executables
        cmake -S . -B build -DTCI_BUILD_GTESTS=ON
        cmake --build build --target tci_demo
        cmake --build build --target tci_gtests
        ./build/Debug/tci_gtests
*/

#include <iostream>
#include <vector>
#include <cstdint>

#include "TraceSystem.h"
#include "TraceControllerInterface.h"
#include "ProbeHwAccess.h"


using namespace tci;

int main() {
    // Instantiate the TraceSystem with a specified buffer size (in bytes) for TraceRamSink
    TraceSystem trSystem(1024); // Increase buffer size if needed to hold more trace data

    // Create a ProbeHwAccess that uses the TraceSystem's MMIO bus
    // ProbeHwAccess probeAccess(trSystem.mmioBus);

    // Define the component regions for logging purposes
    std::vector<ProbeHwAccess::ComponentRegion> componentRegions = {
        {TraceSystem::TR_TE_BASE, 0x1000, "TraceEncoder"},
        {TraceSystem::TR_FUNNEL_BASE, 0x1000, "TraceFunnel"},
        {TraceSystem::TR_RAM_SINK_BASE, 0x1000, "TraceRamSink"}
    };
    ProbeHwAccess probeAccess(trSystem.mmioBus, componentRegions);
    
    // TraceControllerInterface takes the probe access(ReadMemory and WriteMemory stubs) and the base addresses of the components
    tci::TraceControllerInterface trController(probeAccess, TraceSystem::TR_TE_BASE, 
        TraceSystem::TR_FUNNEL_BASE, TraceSystem::TR_RAM_SINK_BASE);

    // Configure the trace system via the TraceControllerInterface
    std::cout << "\n Configuring Trace System... " << std::endl;
    trController.configure();

    // Simulate trace generation by setting TraceEncoder active and emitting some trace data
    std::cout << "\n Starting Trace System..." << std::endl;
    trController.start();

    // In a real system, the TraceEncoder would emit trace data based on the execution of instructions.
    // For this simulation, emitTrace() is called to generate some trace data.
    trSystem.emitTrace(0x3000, 0xDEADBEEF); // example pc and opcode
    trSystem.emitTrace(0x3004, 0xCAFEBABE); // another example pc and opcode
    trSystem.emitTrace(0x3008, 0xAAFE1000); // another example pc and opcode
    trSystem.emitTrace(0x300C, 0xBAFE1004); // another example pc and opcode


    // Stop the trace system 
    std::cout << "\n Stopping Trace System..." << std::endl;
    trController.stop();

    // Fetch trace data from TraceRamSink via TraceControllerInterface
    std::uint32_t numfetchWords = 10; // number of 32-bit words to fetch from TraceRamSink
    std::cout << "\n Fetching trace data from TraceRamSink (size: " << numfetchWords << " words)..." << std::endl;
    std::vector<uint32_t> wordsFetched = trController.fetch(numfetchWords); // fetch trace data n words from TraceRamSink
    
    // Print fetched trace data
    std::cout << "\n Fetched trace data words: ";
    for (const auto& word : wordsFetched) {
        std::cout << std::hex << word << " ";
    }
    std::cout << std::dec << std::endl; // reset to decimal

    std::cout << "\n Main function finished execution" << std::endl;
    return 0;
}