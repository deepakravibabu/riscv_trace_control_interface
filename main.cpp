#include "traceEncoder.h"
#include "traceFunnel.h"
#include "traceRamSink.h"
#include "MmioBus.h"

#include <iostream>

int main() {
    TraceEncoder encoder;
    TraceFunnel funnel;
    TraceRamSink sink;
    MmioBus mmioBus;
    
    
    // Connect the components
    encoder.connect(&funnel);
    funnel.connect(&sink);

    // Map all components via MMIOBus
    mmioBus.addMapping(0x1000, 0x10, &encoder); // TraceEncoder at 0x1000 - 0x100F
    mmioBus.addMapping(0x2000, 0x10, &funnel); // TraceFunnel at 0x2000 - 0x200F
    mmioBus.addMapping(0x3000, 0x10, &sink); // TraceRamSink at 0x3000 - 0x300F
    
    // Enable TraceEncoder via MMIO
    mmioBus.write32(0x1000 + TR_TE_CONTROL, 1);
    // Enable TraceFunnel via MMIO
    mmioBus.write32(0x2000 + TR_FUNNEL_CONTROL, 1);
    // Enable TraceRamSink via MMIO
    mmioBus.write32(0x3000 + TR_RAM_CONTROL, 1);

    // Emit an instruction trace
    encoder.emitInstr();

    sink.printDataBuffer();

    std::cout << "Main function finished execution" << std::endl;

    return 0;
}

