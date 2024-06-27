/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 */

#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addDiscoveryServer("mdns");
    instanceBuilder.setRootDevice("daqref://device1");
    
    const InstancePtr instance = instanceBuilder.build();

    instance.addServer("openDAQ LT Streaming", nullptr);
    instance.addStandardServers();

    for (const auto& server : instance.getServers())
    {
        server.enableDiscovery();
    }

    while(true)
        std::this_thread::sleep_for(100ms);
}
