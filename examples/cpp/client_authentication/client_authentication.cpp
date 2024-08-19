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

    // Set username and password
    auto config = instance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");
    generalConfig.setPropertyValue("Username", "root");
    generalConfig.setPropertyValue("Password", "root");

    // Connect to device on localhost
    auto device = instance.addDevice("daq.nd://127.0.0.1", config);
    auto refDevice = device.getDevices()[0];

    // Make a call to the protected function property
    auto sumProp = refDevice.getPropertyValue("Protected.Sum");
    Int result = sumProp.call(1, 2);
    std::cout << "Result: " << result << std::endl;

    return 0;
}
