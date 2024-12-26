#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <future>

using namespace daq;

void modifyIpConfiguration(const StringPtr& ifaceName, const PropertyObjectPtr& config)
{
    bool dhcp4 = config.getPropertyValue("dhcp4");
    bool dhcp6 = config.getPropertyValue("dhcp6");

    SerializerPtr serializer = JsonSerializer();
    ListPtr<IString> addr4List = config.getPropertyValue("addresses4");
    addr4List.serialize(serializer);
    StringPtr addresses4 = serializer.getOutput();
    serializer.reset();
    ListPtr<IString> addr6List = config.getPropertyValue("addresses6");
    addr6List.serialize(serializer);
    StringPtr addresses6 = serializer.getOutput();

    StringPtr gateway4 = config.getPropertyValue("gateway4");
    StringPtr gateway6 = config.getPropertyValue("gateway6");

    const std::string scriptWithParams = "/home/opendaq/netplan_manager.py verify " +
                                         ifaceName.toStdString() + " " +
                                         std::to_string(dhcp4) + " " +
                                         std::to_string(dhcp6) + " " +
                                         "'" + addresses4.toStdString() + "' " +
                                         "'" + addresses6.toStdString() + "' " +
                                         gateway4.toStdString() + " " +
                                         gateway6.toStdString();

    const std::string command = "sudo python3 " + scriptWithParams + " 2>&1"; // Redirect stderr to stdout
    std::array<char, 256> buffer;
    std::string result;

    // Open the command for reading
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        throw GeneralErrorException("Failed to run the IP modification command");

    // Read the output of the command
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();

    // Get the exit status
    int exitCode = pclose(pipe);
    if (exitCode)
        throw InvalidParameterException("IP modification failed: {}", result);

    // Schedule applying changes
    (void)std::async(std::launch::async, []() { (void)std::system("sudo python3 /home/opendaq/netplan_manager.py apply"); });
}

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
    instanceBuilder.setRootDevice("daqref://device0");
    instanceBuilder.setModifyIpConfigCallback(
        [](const StringPtr& ifaceName, const PropertyObjectPtr& config)
        {
            modifyIpConfiguration(ifaceName, config);
        }
    );

    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);

    auto refDevice = instance.getRootDevice();
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
