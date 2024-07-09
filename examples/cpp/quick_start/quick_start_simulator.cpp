/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    PropertyObjectPtr config = PropertyObject();
    config.addProperty(StringProperty("Name", "Reference device simulator"));
    config.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    config.addProperty(StringProperty("serialNumber", "sim01"));

    const InstancePtr instance = InstanceBuilder()
                                 .addModulePath(MODULE_PATH)
                                 .addDiscoveryServer("mdns")
                                 .setRootDevice("daqref://device1", config)
                                 .build();

    const auto servers = instance.addStandardServers();

    for (const auto& server : servers)
        server.enableDiscovery();
        
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
