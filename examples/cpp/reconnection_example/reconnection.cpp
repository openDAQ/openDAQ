#include <opendaq/opendaq.h>
#include <coreobjects/core_event_args_ids.h>
#include <iostream>
#include <condition_variable>

using namespace daq;
using namespace std::chrono_literals;

/*
 * Exampling highlighting how to detect connection loss and reconnection when connected via the
 * openDAQ Native Protocol. This example works on openDAQ versions 3.11 and newer.
 */

InstancePtr createServerInstance()
{
    auto instance = InstanceBuilder().setGlobalLogLevel(LogLevel::Critical).build();
    instance.addServer("OpenDAQNativeStreaming", nullptr);
    return instance;
}

// Creates an `AddDeviceConfig` object with monitoring enabled. Monitoring enables a heartbeat mechanism
// that can detect connection loss when the device does not gracefully disconnect.
PropertyObjectPtr createConfigWithMonitoring(const InstancePtr& instance)
{
    auto config = instance.createDefaultAddDeviceConfig();
    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    PropertyObjectPtr transportLayerConfig = nativeDeviceConfig.getPropertyValue("TransportLayerConfig");
    transportLayerConfig.setPropertyValue("MonitoringEnabled", True);

    return config;
}

InstancePtr createClientInstance()
{
    auto instance = InstanceBuilder().setGlobalLogLevel(LogLevel::Critical).build();
    instance.addDevice("daq.nd://127.0.0.1", createConfigWithMonitoring(instance));
    return instance;
}

std::string protocolTypeToString(Int typeId)
{
    const ProtocolType type = static_cast<ProtocolType>(typeId);
    switch (type)
    {
        case ProtocolType::Unknown:
            return "Unknown";
        case ProtocolType::Configuration:
            return "Configuration";
        case ProtocolType::Streaming:
            return "Streaming";
        case ProtocolType::ConfigurationAndStreaming:
            return "ConfigurationAndStreaming";
    }

    return "";
}

int main()
{
    auto serverInstance = createServerInstance();
    auto clientInstance = createClientInstance();
    auto clientDevice = clientInstance.getDevices()[0];

    // Get the connection status of the configuration and streaming connections.
    auto connectionStatusContainer = clientDevice.getConnectionStatusContainer();
    auto configurationStatus = connectionStatusContainer.getStatus("ConfigurationStatus");
    auto streamingStatus = connectionStatusContainer.getStatus("StreamingStatus_OpenDAQNativeStreaming_1");

    std::cout << "=====================\n";
    std::cout << "Configuration connection status: " << configurationStatus.getValue() << "\n";
    std::cout << "Native streaming connection status: " << streamingStatus.getValue() << "\n";
    std::cout << "=====================\n\n";

    std::condition_variable cv;
    std::mutex m;
    int connectionChangedCount = 0;
    bool ready = false;

    // Listen for connection status changes when the state changes to "Connected" or "Reconnecting".
    clientDevice.getOnComponentCoreEvent() += [&] (ComponentPtr&, CoreEventArgsPtr& args)
    {
        if (args.getEventId() == static_cast<Int>(CoreEventId::ConnectionStatusChanged))
        {
            const EnumerationPtr value = args.getParameters().get("StatusValue");
            if (value != "Connected" && value != "Reconnecting")
                return;

            std::unique_lock lk(m);
            cv.wait(lk, [&ready] { return ready; });

            std::cout << "=====================\n";
            std::cout << "Connection status name: " << args.getParameters().get("StatusName") << "\n";
            std::cout << "Connection string: " << args.getParameters().get("ConnectionString") << "\n";
            std::cout << "Connection status value: " << args.getParameters().get("StatusValue") << "\n";
            std::cout << "Connection status Protocol Type: " << protocolTypeToString(args.getParameters().get("ProtocolType")) << "\n";
            std::cout << "=====================\n\n";

            ready = false;
            connectionChangedCount++;
            cv.notify_one();
        }
    };

    {
        // Simulate a connection loss.
        std::lock_guard lk(m);
        serverInstance.release();
        ready = true;
        cv.notify_one();
    }

    // Wait for both the configuration and streaming connection to start reconnection.
    for (int i = 0; i < 2; i++)
    {
        std::unique_lock lk(m);
        cv.wait(lk, [&connectionChangedCount, i] { return connectionChangedCount > i; });
        ready = true;
        cv.notify_one();
    }

    std::this_thread::sleep_for(1000ms);

    {
        // Simulate a reconnection.
        std::lock_guard lk(m);
        ready = false;
        serverInstance = createServerInstance();
        ready = true;
        cv.notify_one();
    }
    
    // Wait for both the configuration and streaming connection to be re-established.
    for (int i = 2; i < 4; i++)
    {
        std::unique_lock lk(m);
        cv.wait(lk, [&connectionChangedCount, i] { return connectionChangedCount > i; });
        ready = true;
        cv.notify_one();
    }

    std::cout << "Press \"enter\" to exit the application..." << "\n";
    std::cin.get();
    return 0;
}
