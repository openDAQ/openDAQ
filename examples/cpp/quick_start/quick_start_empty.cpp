/**
 * Part of the openDAQ stand-alone application quick start guide. The full
 * example can be found in app_quick_start_full.cpp
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <cassert>
#include <opendaq/opendaq.h>

using namespace daq;

static InstancePtr CreateCustomServerInstance(AuthenticationProviderPtr authenticationProvider)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "serverLocal");

    const auto statistics = instance.addFunctionBlock("RefFBModuleStatistics");
    const auto refDevice = instance.addDevice("daqref://device0");
    refDevice.addProperty(IntProperty("CustomProp", 0));
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);
    statistics.getInputPorts()[0].connect(Signal(context, nullptr, "foo"));

    const auto statusType = EnumerationType("StatusType", List<IString>("Off", "On"));
    typeManager.addType(statusType);
    const auto statusValue = Enumeration("StatusType", "On", typeManager);

    instance.getStatusContainer().asPtr<IComponentStatusContainerPrivate>().addStatus("TestStatus", statusValue);

    return instance;
}

static InstancePtr CreateDefaultServerInstance()
{
    auto authenticationProvider = AuthenticationProvider();
    return CreateCustomServerInstance(authenticationProvider);
}

static InstancePtr CreateUpdatedServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "serverLocal");

    const auto statistics = instance.addFunctionBlock("RefFBModuleScaling");
    const auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("NumberOfChannels", 3);

    const auto testType = EnumerationType("TestEnumType", List<IString>("TestValue1", "TestValue2"));
    instance.getContext().getTypeManager().addType(testType);

    const auto statusType = EnumerationType("StatusType", List<IString>("Off", "On"));
    typeManager.addType(statusType);
    const auto statusValue = Enumeration("StatusType", "Off", typeManager);

    instance.getStatusContainer().asPtr<IComponentStatusContainerPrivate>().addStatusWithMessage("TestStatus", statusValue, "MsgOff");


    return instance;
}

static InstancePtr CreateServerInstance(InstancePtr instance = CreateDefaultServerInstance())
{
    instance.addServer("OpenDAQNativeStreaming", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance(uint16_t nativeConfigProtocolVersion = std::numeric_limits<uint16_t>::max(),
                                        Bool restoreClientConfigOnReconnect = False)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    // const ModulePtr deviceModule(MockDeviceModule_Create(context));
    // moduleManager.addModule(deviceModule);

    auto instance = InstanceCustom(context, "clientLocal");

    auto config = instance.createDefaultAddDeviceConfig();

    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("OpenDAQNativeConfiguration");
    if (nativeConfigProtocolVersion != std::numeric_limits<uint16_t>::max())
        nativeDeviceConfig.setPropertyValue("ProtocolVersion", nativeConfigProtocolVersion);

    nativeDeviceConfig.setPropertyValue("RestoreClientConfigOnReconnect", restoreClientConfigOnReconnect);
    nativeDeviceConfig.addProperty(IntProperty("HeartbeatPeriod", 100000000));

    PropertyObjectPtr general = config.getPropertyValue("General");
    general.setPropertyValue("PrioritizedStreamingProtocols", List<IString>("OpenDAQNativeStreaming"));

    auto refDevice = instance.addDevice("daq.nd://127.0.0.1", config);
    return instance;
}

static void checkDeviceOperationMode(const daq::DevicePtr& device, OperationModeType expected, bool isServer = false)
{
    assert(device.getOperationMode() == expected);
    bool active = expected != OperationModeType::Idle;
    std::string messagePrefix = isServer ? "Server: " : "Client: ";

    for (const auto& ch: device.getChannels())
    {
        assert(ch.getActive() == active); // Removed message for simplicity
        for (const auto& signal: ch.getSignals())
        {
            assert(signal.getActive() == active); // Removed message for simplicity
        }
    }
}

int main(int /*argc*/, const char* /*argv*/[])
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    checkDeviceOperationMode(server, daq::OperationModeType::Operation);
    checkDeviceOperationMode(server.getDevices()[0], daq::OperationModeType::Operation);
    checkDeviceOperationMode(client.getRootDevice(), daq::OperationModeType::Operation);

    assert(server.getAvailableOperationModes() == client.getDevices()[0].getAvailableOperationModes());
    assert(server.getDevices()[0].getAvailableOperationModes() == client.getDevices()[0].getDevices()[0].getAvailableOperationModes());

    // setting the operation mode for server root device
    server.setOperationModeRecursive(daq::OperationModeType::Idle);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    checkDeviceOperationMode(server.getRootDevice(), daq::OperationModeType::Idle, true);
    checkDeviceOperationMode(server.getDevices()[0], daq::OperationModeType::Idle, true);

    checkDeviceOperationMode(client.getRootDevice(), daq::OperationModeType::Operation);
    checkDeviceOperationMode(client.getDevices()[0], daq::OperationModeType::Idle);
    checkDeviceOperationMode(client.getDevices()[0].getDevices()[0], daq::OperationModeType::Idle);

    // setting the operation mode for server sub device
    server.getDevices()[0].setOperationModeRecursive(daq::OperationModeType::SafeOperation);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    checkDeviceOperationMode(server.getRootDevice(), daq::OperationModeType::Idle, true);
    checkDeviceOperationMode(server.getDevices()[0], daq::OperationModeType::SafeOperation, true);

    checkDeviceOperationMode(client.getRootDevice(), daq::OperationModeType::Operation);
    checkDeviceOperationMode(client.getDevices()[0], daq::OperationModeType::Idle);
    checkDeviceOperationMode(client.getDevices()[0].getDevices()[0], daq::OperationModeType::SafeOperation);

    // setting the operation mode for client sub device
    client.getDevices()[0].getDevices()[0].setOperationModeRecursive(daq::OperationModeType::Operation);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    checkDeviceOperationMode(server.getRootDevice(), daq::OperationModeType::Idle, true);
    checkDeviceOperationMode(server.getDevices()[0], daq::OperationModeType::Operation, true);

    checkDeviceOperationMode(client.getRootDevice(), daq::OperationModeType::Operation);
    checkDeviceOperationMode(client.getDevices()[0], daq::OperationModeType::Idle);
    checkDeviceOperationMode(client.getDevices()[0].getDevices()[0], daq::OperationModeType::Operation);

    // setting the operation mode for client device not recursively
    client.getDevices()[0].setOperationMode(daq::OperationModeType::SafeOperation);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    checkDeviceOperationMode(server.getRootDevice(), daq::OperationModeType::SafeOperation, true);
    checkDeviceOperationMode(server.getDevices()[0], daq::OperationModeType::Operation, true);

    checkDeviceOperationMode(client.getRootDevice(), daq::OperationModeType::Operation);
    checkDeviceOperationMode(client.getDevices()[0], daq::OperationModeType::SafeOperation);
    checkDeviceOperationMode(client.getDevices()[0].getDevices()[0], daq::OperationModeType::Operation);

    // setting the operation mode for client device
    client.setOperationModeRecursive(daq::OperationModeType::Idle);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    checkDeviceOperationMode(server.getRootDevice(), daq::OperationModeType::Idle, true);
    checkDeviceOperationMode(server.getDevices()[0], daq::OperationModeType::Idle, true);

    checkDeviceOperationMode(client.getRootDevice(), daq::OperationModeType::Idle);
    checkDeviceOperationMode(client.getDevices()[0], daq::OperationModeType::Idle);
    checkDeviceOperationMode(client.getDevices()[0].getDevices()[0], daq::OperationModeType::Idle);
}