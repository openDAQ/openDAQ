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

    StringPtr loggerPath = "ref_device_simulator.log";

    PropertyObjectPtr config = PropertyObject();
    config.addProperty(StringProperty("Name", "Reference device simulator"));
    config.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    config.addProperty(StringProperty("SerialNumber", "sim01"));
    config.addProperty(BoolProperty("EnableLogging", true));
    config.addProperty(StringProperty("LoggingPath", loggerPath));

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addDiscoveryServer("mdns");
    instanceBuilder.setRootDevice("daqref://device1", config);
    instanceBuilder.addLoggerSink(BasicFileLoggerSink(loggerPath));
    for (const auto& sink : DefaultSinks())
        instanceBuilder.addLoggerSink(sink);

    const InstancePtr instance = instanceBuilder.build();

    const auto servers = instance.addStandardServers();

    for (const auto& server : servers)
        server.enableDiscovery();
        
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
