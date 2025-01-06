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
    const auto deviceInfo = instance.getAvailableDevices();
    for (const auto& info : deviceInfo)
    {
        for (const auto& cap : info.getServerCapabilities())
        {
            std::cout << cap.getAddresses() << std::endl;
        }
    }

    fmt::println(R"(Press "enter" to exit the application...)");
    std::cin.get();
    return 0;
}
