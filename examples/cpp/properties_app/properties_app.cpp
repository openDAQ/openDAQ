/**
 * Properties application
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    DeviceInfoPtr info;

    auto availableDevices = instance.getAvailableDevices();

    for (const auto& dev : availableDevices)
    {
        if (dev.getName() == "PropertiesDevice")
        {
            info = dev;
        }
    }

    auto connectionString = info.getConnectionString();

    auto device = instance.addDevice(connectionString);

    if (device.assigned())
    {
        auto properties = device.getAllProperties();
        for (const auto& prop : properties)
        {
            std::cout << "Property: " << prop.getName() << "\n";
            std::cout << "Value: " << prop.getValue() << "\n\n";
        }
    }

    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
}
