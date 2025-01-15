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

    for (const auto& devInfo : availableDevices)
    {
        if (devInfo.getConnectionString().toStdString().find("daq://openDAQ_") != std::string::npos)
        {
            std::cout << "Found simulator device: " << devInfo.getConnectionString() << std::endl;
            std::cout << "Count of network interfaces: " << devInfo.getNetworkInterfaces().getCount() << std::endl;
            if (devInfo.getNetworkInterfaces().hasKey("enp0s3"))
            {
                const auto dhcp4 = True;
                const auto addresses4 = List<IString>();
                const auto gateway4 = String("");
                const auto dhcp6 = True;
                const auto addresses6 = List<IString>();
                const auto gateway6 = String("");

                auto ipConfig = devInfo.getNetworkInterface("enp0s3").createDefaultConfiguration();
                ipConfig.setPropertyValue("dhcp4", dhcp4);
                ipConfig.setPropertyValue("addresses4", addresses4);
                ipConfig.setPropertyValue("gateway4", gateway4);
                ipConfig.setPropertyValue("dhcp6", dhcp6);
                ipConfig.setPropertyValue("addresses6", addresses6);
                ipConfig.setPropertyValue("gateway6", gateway6);

                try
                {
                    std::cout << "Device " << devInfo.getConnectionString() << " submitting new IP config..." << std::endl;
                    devInfo.getNetworkInterface("enp0s3").submitConfiguration(ipConfig);
                    std::cout << "success!" << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            }
        }
    }

    return 0;
}
