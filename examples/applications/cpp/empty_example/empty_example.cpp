/**
 * Empty example
 */

#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);
    //auto dev = instance.addDevice("daq.simulator://");
    auto dev = instance.addDevice("daqref://device0");
    auto fb = instance.addFunctionBlock("RefFBModuleRenderer");
    fb.getInputPorts()[0].connect(dev.getSignalsRecursive()[0]);
    //dev.getChannelsRecursive()[0].setPropertyValue("SampleRateDivider", 2);
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
