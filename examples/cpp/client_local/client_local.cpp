/**
 * Connects to a device on localhost and outputs its name and connection string.
 * Requires the "device" example to be running.
 */

#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);
    
    // Connect to device on localhost
    auto device = instance.addDevice("daq.opcua://127.0.0.1");
    
    // Output the name and connection string of connected-to device
    std::cout << "Connected device:" << std::endl;
    auto info = device.getInfo();
    std::cout << "Name: " << info.getName() << ", Connection string: " << info.getConnectionString() << std::endl;

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
