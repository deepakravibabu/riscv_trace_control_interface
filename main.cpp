#include "traceEncoder.h"
#include "traceFunnel.h"
#include "traceRamSink.h"

#include <iostream>

int main() {
    TraceEncoder encoder;
    TraceFunnel funnel;
    TraceRamSink sink;
    
    
    // Connect the components
    encoder.connect(&funnel);
    funnel.connect(&sink);
    
    // Emit an instruction trace
    encoder.emitInstr();

    sink.printDataBuffer();

    std::cout << "Main function executed" << std::endl;

    return 0;
}

