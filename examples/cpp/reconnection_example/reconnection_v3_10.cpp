#include <opendaq/opendaq.h>
#include <coreobjects/core_event_args_ids.h>
#include <iostream>
#include <condition_variable>

using namespace daq;
using namespace std::chrono_literals;

/*
 * Exampling highlighting how to detect connection loss and reconnection when connected via the
 * openDAQ Native Protocol. This example works on both the latest and the 3.10 version of
 * openDAQ. Newer versions provide a more streamlined way of handling disconnection/reconn
 * detection.
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

int main()
{
    auto serverInstance = createServerInstance();
    auto clientInstance = createClientInstance();
    auto clientDevice = clientInstance.getDevices()[0];

    // Get the connection status of the configuration connection
    auto statusContainer = clientDevice.getStatusContainer();
    auto connectionStatus = statusContainer.getStatus("ConnectionStatus");

    std::cout << "=====================\n";
    std::cout << "Connection status: " << connectionStatus.getValue() << "\n";
    std::cout << "=====================\n\n";
    
    std::condition_variable cv;
    std::mutex m;
    int connectionChangedCount = 0;
    bool ready = false;
    
    // Listen for connection status changes via the "StatusChanged" core event.
    clientDevice.getOnComponentCoreEvent() += [&] (ComponentPtr&, CoreEventArgsPtr& args)
    {
        if (args.getEventId() == static_cast<Int>(CoreEventId::StatusChanged) && args.getParameters().hasKey("ConnectionStatus"))
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&ready] { return ready; });

            std::cout << "=====================\n";
            std::cout << "Connection status changed: " << args.getParameters().get("ConnectionStatus") << "\n";
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

    {
        // Wait for the configuration connection to start reconnection.
        std::unique_lock lk(m);
        cv.wait(lk, [&connectionChangedCount] { return connectionChangedCount > 0; });
    }

    std::this_thread::sleep_for(1000ms);

    {
        // Simulate a reconnection.
        std::lock_guard lk(m);
        serverInstance = createServerInstance();
        ready = true;
        cv.notify_one();
    }

    {
        // Wait for the configuration connection to be re-established.
        std::unique_lock lk(m);
        cv.wait(lk, [&connectionChangedCount] { return connectionChangedCount > 1; });
    }

    std::cout << "Press \"enter\" to exit the application..." << "\n";
    std::cin.get();
    return 0;
}
