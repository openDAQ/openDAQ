#include <chrono>
#include <csignal>
#include <thread>
#include <opendaq/opendaq.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <future>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    {
        // Applies the latest IP configuration before initializing the openDAQ instance.
        // If unsuccessful, attempts to restore the previous configuration from the backup.
        // py script runs with root privileges without requiring a password, as specified in sudoers
        int result = std::system("sudo python3 /home/opendaq/netplan_manager.py apply");
        (void)result;
    }

    const ConfigProviderPtr configProvider = JsonConfigProvider();

    auto users = List<IUser>();
    users.pushBack(User("opendaq", "$2b$10$bqZWNEd.g1R1Q1inChdAiuDr5lbal33bBNOehlCwuWcxRH5weF3hu")); // password: opendaq
    users.pushBack(User("root", "$2b$10$k/Tj3yqFV7uQz42UCJK2n.4ECd.ySQ2Sfd81Kx.xfuMOeluvA/Vpy", {"admin"})); // password: root
    const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder();
    instanceBuilder.addConfigProvider(configProvider);
    instanceBuilder.setAuthenticationProvider(authenticationProvider);
    instanceBuilder.addDiscoveryServer("mdns");
    instanceBuilder.setRootDevice("daqref://device0");

    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);

    auto refDevice = instance.getRootDevice();
    refDevice.setPropertyValue("EnableProtectedChannel", true);

    auto servers = instance.addStandardServers();
    for (const auto& server : servers)
    {
        // OPC UA server uses Avahi service for discovery for example purposes
        if (server.getId() != "OpenDAQOPCUA")
            server.enableDiscovery();
    }

    while (true)
    {
        std::this_thread::sleep_for(100ms);
    }

    // Anticipated to be automatically restarted by the systemd service.
    return 0;
}
