/**
 * Connects to all discoverable devices and outputs their names and connection strings.
 * Requires discoverable openDAQ devices to be available on the network.
 */

#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

void printObject(const PropertyObjectPtr& obj, int ident)
{
    // for(const auto& prop : obj.getVisibleProperties())
    for (const auto& prop : obj.getAllProperties())
    {
        auto propName = prop.getName();

        auto propValue = obj.getPropertyValue(propName);
        auto propObj = propValue.asPtrOrNull<IPropertyObject>(true);

        if (propObj.assigned())
        {
            fmt::println("{:\t<{}}{:<25}: {}", "\t", ident, propName, "");
            printObject(propObj, ident + 1);
        }
        else
        {
            fmt::println("{:\t<{}}{:<25}: {}", "\t", ident, propName, propValue);
        }
        
    }
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    // Discover all available devices, filter out all of which connection strings
    // do not start with "daq.opcua://" or "daq.lt://" or "daq.ns://"
    const auto deviceInfo = instance.getAvailableDevices();
    auto devices = List<IDevice>();
    for (const auto& info : deviceInfo)
    {
        auto device = instance.addDevice(info.getConnectionString());
        devices.pushBack(device);
    }

    // Output the names and connection strings of all connected-to devices
    fmt::println("Connected devices:");

    for (auto device : devices)
    {
        DeviceInfoPtr info = device.getInfo();

        fmt::println("---------------");
        
        printObject(info, 1);

        fmt::println("---------------");
    }


    fmt::println(R"(Press "enter" to exit the application...)");
    std::cin.get();
    return 0;
}
