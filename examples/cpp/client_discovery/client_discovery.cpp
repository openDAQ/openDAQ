/**
 * Connects to all discoverable devices and outputs their names and connection strings.
 * Requires discoverable openDAQ devices to be available on the network.
 */

#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    // Discover all available devices, filter out all of which connection strings
    // do not start with "daq.opcua://" or "daq.ws://" or "daq.nsd://"
    const auto deviceInfo = instance.getAvailableDevices();
    auto devices = List<IDevice>();
    for (auto info : deviceInfo)
    {
        for (const auto & capability : info.getServerCapabilities())
        {
            auto device = instance.addDevice(capability.getPrimaryConnectionString());
            devices.pushBack(device);
        }
    }

    // Output the names and connection strings of all connected-to devices
    std::cout << "Connected devices:" << std::endl;
    for (auto device : devices)
    {
        auto info = device.getInfo();
        std::cout << "Name: " << info.getName() << ", Connection string: " << info.getConnectionString() << std::endl;
    }

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
