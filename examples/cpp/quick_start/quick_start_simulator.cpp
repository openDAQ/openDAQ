/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 */

#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;
    StringPtr loggerPath = "ref_device_simulator.log";

    auto users = List<IUser>();
    users.pushBack(User("opendaq", "$2b$10$bqZWNEd.g1R1Q1inChdAiuDr5lbal33bBNOehlCwuWcxRH5weF3hu")); // password: opendaq
    users.pushBack(User("root", "$2b$10$k/Tj3yqFV7uQz42UCJK2n.4ECd.ySQ2Sfd81Kx.xfuMOeluvA/Vpy", {"admin"})); // password: root
    const AuthenticationProviderPtr authenticationProvider = StaticAuthenticationProvider(true, users);

    PropertyObjectPtr config = PropertyObject();
    config.addProperty(StringProperty("Name", "Reference device simulator"));
    config.addProperty(StringProperty("LocalId", "RefDevSimulator"));
    config.addProperty(StringProperty("SerialNumber", "sim01"));
    config.addProperty(BoolProperty("EnableLogging", true));
    config.addProperty(StringProperty("LoggingPath", loggerPath));

    const InstancePtr instance = InstanceBuilder().addModulePath(MODULE_PATH)
                                                  .addDiscoveryServer("mdns")
                                                  .setRootDevice("daqref://device1", config)
                                                  .setLogger(Logger(loggerPath))
                                                  .setAuthenticationProvider(authenticationProvider)
                                                  .build();

    auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("EnableProtectedChannel", true);
    const auto servers = instance.addStandardServers();

    for (const auto& server : servers)
        server.enableDiscovery();
        
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
