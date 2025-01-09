#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <future>

using namespace daq;

std::string addrListToJson(const ListPtr<IString>& addresses)
{
    SerializerPtr serializer = JsonSerializer();
    addresses.serialize(serializer);
    return serializer.getOutput().toStdString();
}

void verifyIpConfiguration(const StringPtr& ifaceName, const PropertyObjectPtr& config)
{
    bool dhcp4 = config.getPropertyValue("dhcp4");
    bool dhcp6 = config.getPropertyValue("dhcp6");
    StringPtr gateway4 = config.getPropertyValue("gateway4");
    StringPtr gateway6 = config.getPropertyValue("gateway6");

    const std::string scriptWithParams = "/home/opendaq/netplan_manager.py verify " +
                                         ifaceName.toStdString() + " " +
                                         (dhcp4 ? "true" : "false") + " " +
                                         (dhcp6 ? "true" : "false") + " " +
                                         "'" + addrListToJson(config.getPropertyValue("addresses4")) + "' " +
                                         "'" + addrListToJson(config.getPropertyValue("addresses6")) + "' " +
                                         "\"" + gateway4.toStdString() + "\" " +
                                         "\"" + gateway6.toStdString() + "\"";

    // py script runs with root privileges without requiring a password, as specified in sudoers
    const std::string command = "sudo python3 " + scriptWithParams + " 2>&1";
    std::array<char, 256> buffer;
    std::string result;

    // Open the command for reading
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw GeneralErrorException("Failed to start IP modification");

    // Read the output of the command
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();

    // Get the exit status
    int exitCode = pclose(pipe);
    if (exitCode)
        throw InvalidParameterException("Invalid IP configuration: {}", result);
}

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

    bool stopped = false;
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
    instanceBuilder.setModifyIpConfigCallback(
        [&stopped](const StringPtr& ifaceName, const PropertyObjectPtr& config)
        {
            try
            {
                verifyIpConfiguration(ifaceName, config);
            }
            catch (...)
            {
                throw;
            }
            // The new IP configuration has been successfully verified. Gracefully stop the application now
            // to allow it to adopt the updated configuration and reopen network sockets upon relaunch.
            stopped = true;
        }
    );

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

    while (!stopped)
    {
        std::this_thread::sleep_for(100ms);
    }

    // Anticipated to be automatically restarted by the systemd service.
    return 0;
}
