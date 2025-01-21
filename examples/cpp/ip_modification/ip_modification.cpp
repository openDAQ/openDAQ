#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

void retrieveIpConfiguration(const daq::NetworkInterfacePtr& iface)
{
    try
    {
        auto config = iface.requestCurrentConfiguration();
        std::cout << "Parameters:"
                  << "\n- dhcp4: " << config.getPropertyValue("dhcp4")
                  << "\n- address4: " << config.getPropertyValue("address4")
                  << "\n- gateway4: " << config.getPropertyValue("gateway4")
                  << "\n- dhcp6: " << config.getPropertyValue("dhcp6")
                  << "\n- address6: " << config.getPropertyValue("address6")
                  << "\n- gateway6: " << config.getPropertyValue("gateway6")
                  << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace daq;

    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    auto availableDevices = instance.getAvailableDevices();

    for (const auto& deviceInfo : availableDevices)
    {
        if (deviceInfo.getName() == "Reference device simulator")
        {
            std::cout << "Found simulator device: " << deviceInfo.getConnectionString() << std::endl;
            std::cout << "Retrieving old IP config..." << std::endl;
            retrieveIpConfiguration(deviceInfo.getNetworkInterface("enp0s3"));

            auto config = deviceInfo.getNetworkInterface("enp0s3").createDefaultConfiguration();
            config.setPropertyValue("dhcp4", False);
            config.setPropertyValue("address4", String("192.168.56.155/24"));
            config.setPropertyValue("gateway4", String("192.168.56.1"));

            try
            {
                std::cout << "Device " << deviceInfo.getConnectionString() << " submitting new IP config..." << std::endl;
                deviceInfo.getNetworkInterface("enp0s3").submitConfiguration(config);
            }
            catch (const std::exception& e)
            {
                std::cout << "Error: " << e.what() << std::endl;
                return 0;
            }

            std::cout << "Wait 5 seconds for new configuration applied ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));

            std::cout << "Retrieving updated IP config..." << std::endl;
            retrieveIpConfiguration(deviceInfo.getNetworkInterface("enp0s3"));

            return 0;
        }
    }

    std::cout << "No simulator device found" << std::endl;
    return 0;
}
