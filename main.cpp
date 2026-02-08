#include "traceEncoder.h"
#include "traceFunnel.h"
#include "traceRamSink.h"
#include "MmioBus.h"

#include <iostream>

/*
    If CMake is used then the project can be built with the following command. 
    <PowerShell terminal>
        PS D:\Project\riscv_trace_control_interface\build> cmake -S .. -B .
        MSBuild.exe .\tci.vcxproj
    It builds the project and the .exe file is found under ./Debug/tci.exe
*/

int main() {
    TraceEncoder encoder;
    TraceFunnel funnel;
    TraceRamSink sink;
    MmioBus mmioBus;
    
    
    // Connect the components
    encoder.connect(&funnel);
    funnel.connect(&sink);

    // Map all components via MMIOBus with 0x1000 size each (4 KB)
    mmioBus.addMapping(0x1000, 0x1000, &encoder); // TraceEncoder at 0x1000 - 0x1FFF
    mmioBus.addMapping(0x2000, 0x1000, &funnel); // TraceFunnel at 0x2000 - 0x2FFF
    mmioBus.addMapping(0x3000, 0x1000, &sink); // TraceRamSink at 0x3000 - 0x3FFF
    
    // Enable TraceEncoder via MMIO
    mmioBus.write32(0x1000 + TR_TE_CONTROL, 1);
    // Enable TraceFunnel via MMIO
    mmioBus.write32(0x2000 + TR_FUNNEL_CONTROL, 1);
    // Enable TraceRamSink via MMIO
    mmioBus.write32(0x3000 + TR_RAM_CONTROL, 1);

    // Emit an instruction trace
    encoder.emitInstr();

    // sink.printDataBuffer();
    // std::uint32_t sinkRamRP = mmioBus.read32(0x3000 + TR_RAM_RP_LOW); // read RP_LOW to see how many bytes have been read
    // std::uint32_t sinkRamWP = mmioBus.read32(0x3000 + TR_RAM_WP_LOW); // read WP_LOW to see how many bytes have been written
    // std::uint32_t sinkDataBuffer = 0;
    // std::cout << "RP_LOW: " << sinkRamRP << std::endl;
    // std::cout << "WP_LOW: " << sinkRamWP << std::endl;

    // while(sinkRamRP <= sinkRamWP) {
    //     std::cout << "Waiting for data to be read from TraceRamSink..." << std::endl;
    //     sinkDataBuffer = mmioBus.read32(0x3000 + TR_RAM_DATA); // read from TraceRamSink to see the data
    //     std::cout << "Data buffer contents: 0x" << std::hex << sinkDataBuffer << std::dec << std::endl;
    // }

    for(;;){
        std::uint32_t sinkRamRP = mmioBus.read32(0x3000 + TR_RAM_RP_LOW); // read RP_LOW to see how many bytes have been read
        std::uint32_t sinkRamWP = mmioBus.read32(0x3000 + TR_RAM_WP_LOW); // read WP_LOW to see how many bytes have been written

        if(sinkRamRP == sinkRamWP) break;

        std::uint32_t sinkDataBufferValue = mmioBus.read32(0x3000 + TR_RAM_DATA); // read from TraceRamSink to see the data
        std::cout << "Data buffer contents: 0x" << std::hex << sinkDataBufferValue << std::dec << std::endl;
    }

    std::cout << "Main function finished execution" << std::endl;
    return 0;
}

