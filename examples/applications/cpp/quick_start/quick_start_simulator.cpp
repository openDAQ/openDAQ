/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 *
 * The "--native" option selects which channels the Native server opens:
 *
 *     quick_start_simulator                                                  # plaintext only (default)
 *     quick_start_simulator --native tls  --cert server.crt --key server.key # TLS only
 *     quick_start_simulator --native both --cert server.crt --key server.key # plaintext and TLS
 *
 * "--ca ca.crt" additionally asks clients for a certificate signed by that authority (mutual TLS).
 * The test build places sample secrets in "native_secrets/" next to the binaries.
 */

#include <opendaq/opendaq.h>
#include <coretypes/filesystem.h>
#include <iostream>
#include <string>

using namespace daq;

namespace
{

struct Options
{
    std::string native = "plain";
    std::string cert;
    std::string key;
    std::string ca;
};

void printUsage(const char* app)
{
    std::cout << "Usage: " << app << " [--native plain|tls|both] [--cert PATH --key PATH] [--ca PATH]" << std::endl;
}

bool parseOptions(int argc, const char* argv[], Options& options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string option = argv[i];
        if (option == "--help" || option == "-h")
            return false;

        if (i + 1 >= argc)
        {
            std::cerr << "Missing value for " << option << std::endl;
            return false;
        }

        const std::string value = argv[++i];
        if (option == "--native")
            options.native = value;
        else if (option == "--cert")
            options.cert = value;
        else if (option == "--key")
            options.key = value;
        else if (option == "--ca")
            options.ca = value;
        else
        {
            std::cerr << "Unknown option " << option << std::endl;
            return false;
        }
    }

    if (options.native != "plain" && options.native != "tls" && options.native != "both")
    {
        std::cerr << "--native must be plain, tls or both" << std::endl;
        return false;
    }

    const bool tlsOptionsGiven = !options.cert.empty() || !options.key.empty() || !options.ca.empty();
    if (options.native == "plain")
    {
        if (tlsOptionsGiven)
        {
            std::cerr << "--cert, --key and --ca need --native tls or --native both" << std::endl;
            return false;
        }
        return true;
    }

    if (options.cert.empty() || options.key.empty())
    {
        std::cerr << "--native " << options.native << " needs --cert and --key" << std::endl;
        return false;
    }

    for (const auto& path : {options.cert, options.key, options.ca})
    {
        if (!path.empty() && !fs::exists(path))
        {
            std::cerr << "File not found: " << path << std::endl;
            return false;
        }
    }

    return true;
}

// Native server with the TLS channel open, and the plaintext one too for "--native both"
PropertyObjectPtr createNativeServerConfig(const InstancePtr& instance, const Options& options)
{
    auto config = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    config.setPropertyValue("EnablePort", options.native == "both");
    config.setPropertyValue("EnableTlsPort", true);
    config.setPropertyValue("CertificateFilePath", options.cert);
    config.setPropertyValue("KeyFilePath", options.key);

    if (!options.ca.empty())
    {
        // missing when the Native modules are built without certificate checks
        if (!config.hasProperty("EnableMutualTls"))
            return nullptr;

        config.setPropertyValue("EnableMutualTls", true);
        config.setPropertyValue("CaCertificateFilePath", options.ca);
    }

    return config;
}

}

int main(int argc, const char* argv[])
{
    using namespace std::chrono_literals;

    Options options;
    if (!parseOptions(argc, argv, options))
    {
        printUsage(argv[0]);
        return 1;
    }

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

    ListPtr<IServer> servers;
    if (options.native == "plain")
    {
        servers = instance.addStandardServers();
    }
    else
    {
        const auto nativeConfig = createNativeServerConfig(instance, options);
        if (!nativeConfig.assigned())
        {
            std::cerr << "Mutual TLS is not available in this build of the Native server" << std::endl;
            return 1;
        }

        servers = List<IServer>();
        servers.pushBack(instance.addServer("OpenDAQNativeStreaming", nativeConfig));
        servers.pushBack(instance.addServer("OpenDAQOPCUA", nullptr));

        std::cout << "Native server channels: " << (options.native == "both" ? "plaintext and TLS" : "TLS only")
                  << (options.ca.empty() ? "" : ", mutual TLS") << std::endl;
    }

    for (const auto& server : servers)
        server.enableDiscovery();

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
