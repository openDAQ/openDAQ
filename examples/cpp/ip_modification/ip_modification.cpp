#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    using namespace daq;
    auto availableDevices = instance.getAvailableDevices();

    for (const auto& deviceInfo : availableDevices)
    {
        if (deviceInfo.getName() == "Reference device simulator")
        {
            std::cout << "Found simulator device: " << deviceInfo.getConnectionString() << std::endl;

            const auto dhcp4 = False;
            const auto addresses4 = List<IString>("192.168.56.155/24");
            const auto gateway4 = String("192.168.56.1");
            const auto dhcp6 = True;
            const auto addresses6 = List<IString>();
            const auto gateway6 = String("");

            auto ipConfig = deviceInfo.getNetworkInterface("enp0s3").createDefaultConfiguration();
            ipConfig.setPropertyValue("dhcp4", dhcp4);
            ipConfig.setPropertyValue("addresses4", addresses4);
            ipConfig.setPropertyValue("gateway4", gateway4);
            ipConfig.setPropertyValue("dhcp6", dhcp6);
            ipConfig.setPropertyValue("addresses6", addresses6);
            ipConfig.setPropertyValue("gateway6", gateway6);

            try
            {
                std::cout << "Device " << deviceInfo.getConnectionString() << " submitting new IP config..." << std::endl;
                deviceInfo.getNetworkInterface("enp0s3").submitConfiguration(ipConfig);
                std::cout << "Done!" << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cout << "Error: " << e.what() << std::endl;
            }
            return 0;
        }
    }

    std::cout << "No simulator device found" << std::endl;
    return 0;
}
