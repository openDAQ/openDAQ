#include <opendaq/opendaq.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace date;
using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("OpenDAQNativeStreaming", nullptr);

    auto clientInstance = Instance();

    try
    {
        clientInstance.addDevice("daq.nd://127.0.0.1");
        std::cout << "Device added" << std::endl;
        return 1;
    }
    catch (...)
    {
    }
    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "wrongPass");
    
    try
    {
        clientInstance.addDevice("daq.nd://127.0.0.1", config);
        std::cout << "Device added" << std::endl;
        return 1;
    }
    catch (...)
    {
    }

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "jure123");
    auto deviceJure = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    if (!deviceJure.assigned())
        return 1;
    clientInstance.removeDevice(deviceJure);

    generalConfig.setPropertyValue("Username", "tomaz");
    generalConfig.setPropertyValue("Password", "tomaz123");
    auto deviceTomaz = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    if(!deviceTomaz.assigned())
        return 1;

    return 0;
}