#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const ConfigProviderPtr configProvider = JsonConfigProvider();
    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().addConfigProvider(configProvider)
                                                                .addDiscoveryServer("mdns");
    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);

    auto servers = instance.addStandardServers();
    for (const auto& server : servers)
    {
        // OpcUa server uses Avahi service for discovery for example purposes
        if (server.getId() != "OpcUaServer")
            server.enableDiscovery();
    }

    while (true)
    {
        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
