#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"
#include <opendaq/context_internal_ptr.h>
#include <coreobjects/authentication_provider_factory.h>

using ModulesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, ModuleManager)
{
    daq::ModuleManagerPtr manager1 = daq::ModuleManager("");
    ASSERT_NO_THROW(manager1.loadModules(NullContext()));
    daq::ModuleManagerPtr manager2 = daq::ModuleManager(".");
    ASSERT_NO_THROW(manager2.loadModules(NullContext()));
    daq::ModuleManagerPtr manager3 = daq::ModuleManager("./dir1/dir2");
    ASSERT_NO_THROW(manager3.loadModules(NullContext()));
    daq::ModuleManagerPtr manager4 = daq::ModuleManager("/invalid_dir/dir1/dir2");
    ASSERT_NO_THROW(manager4.loadModules(NullContext()));
    daq::ModuleManagerPtr manager5 = daq::ModuleManager("[[none]]");
    ASSERT_NO_THROW(manager5.loadModules(NullContext()));
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, BBInstanceModules)
{
    ASSERT_NO_THROW(daq::InstancePtr instance1 = daq::Instance());
    ASSERT_NO_THROW(daq::InstancePtr instance2 = daq::Instance("."));
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, EnumerateModules)
{
    // Create the module manager and load modules in the executable directory
    daq::ModuleManagerPtr manager = daq::ModuleManager("");
    manager.loadModules(NullContext());
    ASSERT_GT(manager.getModules().getCount(), 0u);

    ModulePtr _module;
    for (auto mod : manager.getModules())
    {
        if (mod.getName() == "ReferenceFunctionBlockModule")
        {
            _module = mod;
            break;
        }
    }

    ASSERT_TRUE(_module.assigned());

    ASSERT_NO_THROW(const auto devices = _module.getAvailableDevices());
    ASSERT_NO_THROW(const auto functionBlockTypes = _module.getAvailableFunctionBlockTypes());
    ASSERT_NO_THROW(const auto serverTypes = _module.getAvailableServerTypes());
    ASSERT_NO_THROW(const auto deviceTypes = _module.getAvailableDeviceTypes());
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, CreateComponents)
{
    SKIP_TEST_MAC_CI;
    
    const auto context = Context(nullptr, Logger(), TypeManager(), ModuleManager(""), AuthenticationProvider(), Dict<IString, IBaseObject>());

    ModuleManagerPtr manager = context.asPtr<IContextInternal>().moveModuleManager();
    manager.loadModules(context);
    ASSERT_GT(manager.getModules().getCount(), 0u);

    ModulePtr fbModule;
    ModulePtr serverModule;
    ModulePtr nativeStreamingServerModule;
    ModulePtr websocketStreamingServerModule;
    ModulePtr devModule;
    for (auto mod : manager.getModules())
    {
        if (mod.getName() == "ReferenceFunctionBlockModule")
            fbModule = mod;
        else if (mod.getName() == "ReferenceDeviceModule")
            devModule = mod;
        else if (mod.getName() == "OpenDAQOPCUAServerModule")
            serverModule = mod;
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        else if (mod.getName() == "OpenDAQNativeStreamingServerModule")
            nativeStreamingServerModule = mod;
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        else if (mod.getName() == "OpenDAQWebsocketStreamingServerModule")
            websocketStreamingServerModule = mod;
#endif
    }

    ASSERT_TRUE(fbModule.assigned());
    ASSERT_TRUE(devModule.assigned());
    ASSERT_TRUE(serverModule.assigned());
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    ASSERT_TRUE(nativeStreamingServerModule.assigned());
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    ASSERT_TRUE(websocketStreamingServerModule.assigned());
#endif

    auto availableDevices = devModule.getAvailableDevices();
    auto it = std::find_if(availableDevices.begin(),
                           availableDevices.end(),
                           [](DeviceInfoPtr info) { return info.getConnectionString() == "daqref://device0"; });
    ASSERT_TRUE(it != availableDevices.end());

    daq::DictPtr<daq::IString, daq::IFunctionBlockType> functionBlockTypes = fbModule.getAvailableFunctionBlockTypes();
    daq::FunctionBlockTypePtr statisticsFbType = functionBlockTypes.get("RefFBModuleStatistics");
    
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    daq::DictPtr<daq::IString, daq::IServerType> nativeStreamingServerTypes =
        nativeStreamingServerModule.getAvailableServerTypes();
    daq::ServerTypePtr nativeStreamingServerType = nativeStreamingServerTypes.get("OpenDAQNativeStreaming");
    ASSERT_GT(nativeStreamingServerTypes.getCount(), 0u);
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    daq::DictPtr<daq::IString, daq::IServerType> websocketStreamingServerTypes =
        websocketStreamingServerModule.getAvailableServerTypes();
    daq::ServerTypePtr webSocketStreamingServerType = websocketStreamingServerTypes.get("OpenDAQLTStreaming");
    ASSERT_GT(websocketStreamingServerTypes.getCount(), 0u);
#endif
    daq::DictPtr<daq::IString, daq::IServerType> serverTypes = serverModule.getAvailableServerTypes();
    daq::ServerTypePtr opcUaServerType = serverTypes.get("OpenDAQOPCUA");

    ASSERT_GT(functionBlockTypes.getCount(), 0u);
    ASSERT_GT(availableDevices.getCount(), 0u);
    ASSERT_GT(serverTypes.getCount(), 0u);

    daq::FunctionBlockPtr functionBlock = fbModule.createFunctionBlock(statisticsFbType.getId(), nullptr, "fb");
    daq::DevicePtr device = devModule.createDevice("daqref://device0", nullptr);
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    daq::ServerPtr nativeStreamingServer = nativeStreamingServerModule.createServer(nativeStreamingServerType.getId(), device);
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    daq::ServerPtr webSocketStreamingServer = websocketStreamingServerModule.createServer(webSocketStreamingServerType.getId(), device);
#endif
    daq::ServerPtr opcUaServer = serverModule.createServer(opcUaServerType.getId(), device);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1500ms);
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, CreateServer)
{
    const auto context =
        Context(nullptr, Logger(), TypeManager(), ModuleManager(""), AuthenticationProvider(), Dict<IString, IBaseObject>());

    ModuleManagerPtr manager = context.asPtr<IContextInternal>().moveModuleManager();
    manager.loadModules(NullContext());
    ASSERT_GT(manager.getModules().getCount(), 0u);
    
    ModulePtr serverModule;
    ModulePtr nativeStreamingServerModule;
    ModulePtr websocketStreamingServerModule;
    ModulePtr devModule;
    for (auto mod : manager.getModules())
    {
        if (mod.getName() == "OpenDAQOPCUAServerModule")
            serverModule = mod;
        else if (mod.getName() == "ReferenceDeviceModule")
            devModule = mod;
#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
        else if (mod.getName() == "OpenDAQNativeStreamingServerModule")
            nativeStreamingServerModule = mod;
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        else if (mod.getName() == "OpenDAQWebsocketStreamingServerModule")
            websocketStreamingServerModule = mod;
#endif
    }
    ASSERT_TRUE(serverModule.assigned());
    daq::DictPtr<daq::IString, daq::IServerType> serverTypes = serverModule.getAvailableServerTypes();
    ASSERT_GT(serverTypes.getCount(), 0u);

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    ASSERT_TRUE(nativeStreamingServerModule.assigned());
    daq::DictPtr<daq::IString, daq::IServerType> nativeStreamingServerTypes =
        nativeStreamingServerModule.getAvailableServerTypes();
    ASSERT_GT(nativeStreamingServerTypes.getCount(), 0u);
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    ASSERT_TRUE(websocketStreamingServerModule.assigned());
    daq::DictPtr<daq::IString, daq::IServerType> websocketStreamingServerTypes =
        websocketStreamingServerModule.getAvailableServerTypes();
    ASSERT_GT(websocketStreamingServerTypes.getCount(), 0u);
#endif

    daq::DevicePtr device = devModule.createDevice("daqref://device0", nullptr);

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    daq::ServerTypePtr nativeStreamingServerType = nativeStreamingServerTypes.get("OpenDAQNativeStreaming");
    daq::PropertyObjectPtr nativeStreamingConfig = nativeStreamingServerType.createDefaultConfig();
    daq::ListPtr<IProperty> nativeStreamingConfigFields = nativeStreamingConfig.getVisibleProperties();
    ASSERT_NO_THROW(nativeStreamingConfigFields[0].getName());
    nativeStreamingConfig.setPropertyValue("NativeStreamingPort", 7420);
    ASSERT_NO_THROW(nativeStreamingServerModule.createServer(nativeStreamingServerType.getId(), device, nativeStreamingConfig));
#endif
#if defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    daq::ServerTypePtr webSocketServerType = websocketStreamingServerTypes.get("OpenDAQLTStreaming");
    daq::PropertyObjectPtr webSocketConfig = webSocketServerType.createDefaultConfig();
    daq::ListPtr<IProperty> webSocketConfigFields = webSocketConfig.getVisibleProperties();
    ASSERT_NO_THROW(webSocketConfigFields[0].getName());
    webSocketConfig.setPropertyValue("WebsocketStreamingPort", 7414);
    webSocketConfig.setPropertyValue("WebsocketControlPort", 7438);
    ASSERT_NO_THROW(websocketStreamingServerModule.createServer(webSocketServerType.getId(), device, webSocketConfig));
#endif

    daq::ServerTypePtr opcUaServerType = serverTypes.get("OpenDAQOPCUA");
    daq::PropertyObjectPtr opcUaConfig = opcUaServerType.createDefaultConfig();
    daq::ListPtr<IProperty> configFields = opcUaConfig.getVisibleProperties();
    ASSERT_NO_THROW(configFields[0].getName());
    opcUaConfig.setPropertyValue("Port", 4840);

    ASSERT_NO_THROW(serverModule.createServer(opcUaServerType.getId(), device, opcUaConfig));
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, InstanceModules)
{
    // Create the instance and load modules
    daq::InstancePtr instance = Instance();

    // List available function blocks
    daq::DictPtr<daq::IString, daq::IFunctionBlockType> functionBlockTypes = instance.getAvailableFunctionBlockTypes();

    // Add the statistics function block, if available
    if (functionBlockTypes.hasKey("RefFBModuleStatistics"))
        daq::FunctionBlockPtr functionBlock = instance.addFunctionBlock("RefFBModuleStatistics");
}

END_NAMESPACE_OPENDAQ
