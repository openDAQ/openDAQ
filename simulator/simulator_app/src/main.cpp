#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const ConfigProviderPtr configProvider = JsonConfigProvider();

    auto users = List<IUser>();
    users.pushBack(User("opendaq", "$2b$10$bqZWNEd.g1R1Q1inChdAiuDr5lbal33bBNOehlCwuWcxRH5weF3hu")); // password: opendaq
    users.pushBack(User("root", "$2b$10$k/Tj3yqFV7uQz42UCJK2n.4ECd.ySQ2Sfd81Kx.xfuMOeluvA/Vpy", {"admin"})); // password: root
    const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

    const InstanceBuilderPtr instanceBuilder = InstanceBuilder();
    instanceBuilder.addConfigProvider(configProvider);
    instanceBuilder.setAuthenticationProvider(authenticationProvider);
    instanceBuilder.addDiscoveryServer("mdns");

    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);

    auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("EnableProtectedChannel", true);

    auto servers = instance.addStandardServers();
    for (const auto& server : servers)
    {
        // OPC UA server uses Avahi service for discovery for example purposes
        if (server.getId() != "OpenDAQOPCUAServerModule")
            server.enableDiscovery();
    }

    while (true)
    {
        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
