#include <opendaq/opendaq.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace date;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
    daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    // Find and connect to a simulator device
    const auto availableDevices = instance.getAvailableDevices();
    daq::DevicePtr device = instance.addDevice("daq.opcua://localhost:4840");

    // Exit if no device is found
    if (!device.assigned())
    {
        std::cout << "No device found" << std::endl;
        return 0;
    }
    auto sync = device.getSyncComponent();
    // auto syncLocked = sync.getSyncLocked();
    auto interfaces = sync.getInterfaces();

    std::cout << "Sync locked: " << bool(sync.getSyncLocked()) << std::endl;
    std::cout << "Sync source: " << sync.getSelectedSource() << std::endl;
    for (auto& interface : interfaces)
    {
        std::cout << "Interface: " << interface.getClassName() << std::endl;
        for (auto& prop : interface.getAllProperties())
        {
            std::cout << "Property: " << prop.getName() << std::endl;
            if (prop.getName() == "Parameters")
            {
                daq::PropertyObjectPtr params = interface.getPropertyValue(prop.getName());
                for (auto& param : params.getAllProperties())
                {
                    std::cout << "Parameter: " << param.getName() << std::endl;
                    if (param.getName() == "Configuration")
                    {
                        auto config = params.getPropertyValue(param.getName());
                    }
                }     
            }
        }
    }

    return 0;
}
