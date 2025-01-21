#include <chrono>
#include <csignal>
#include <thread>
#include <opendaq/opendaq.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>

using namespace daq;

// Atomic flag to signal termination
std::atomic<bool> stopped{false};

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
        server.enableDiscovery();

    auto signalHandler = [](int signal)
    {
        if (signal == SIGINT)
            stopped = true; // Set the flag to exit the loop
    };
    // Register the signal handler for SIGINT
    std::signal(SIGINT, signalHandler);
    while (!stopped)
    {
        std::this_thread::sleep_for(100ms);
    }

    // Anticipated to be automatically restarted by the systemd service.
    return 0;
}
