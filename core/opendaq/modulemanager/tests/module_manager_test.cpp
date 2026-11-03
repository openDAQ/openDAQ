#include <testutils/testutils.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include "mock/mock_module.h"

#include <chrono>
#include <set>
#include <thread>

#include <opendaq/component_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/module_manager_errors.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>

using namespace daq;

class ModuleManagerTest : public testing::Test
{
protected:
    static const std::string SearchDir;

    ModuleManagerTest()
        : logger(Logger())
        , context(NullContext(logger))
    {
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);
    }

    LoggerPtr logger;
    ContextPtr context;
};

const std::string ModuleManagerTest::SearchDir{MODULE_TEST_DIR};

TEST_F(ModuleManagerTest, Create)
{
    auto manager = ModuleManager("[[none]]");
    ASSERT_TRUE(manager.assigned());
}

// TODO: Fix memory leak
//
//TEST_F(ModuleManagerTest, DefaultToCurrentDirectory)
//{
//    ASSERT_NO_THROW(ModuleManager(""));
//}

TEST_F(ModuleManagerTest, CreateNullPath)
{
    ASSERT_THROW(ModuleManager(nullptr), InvalidParameterException);
}

TEST_F(ModuleManagerTest, CreateInvalidPath)
{
    auto mngr = ModuleManager("in\\/@|id");
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, CreateNonExistentPath)
{
    std::string workingDir = SearchDir + "/doesNotExist";
    auto mngr = ModuleManager(workingDir);
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, CreateNotAFolder)
{
    std::string workingDir = SearchDir + "/file";
    auto mngr = ModuleManager(workingDir);
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, AddModule)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>();

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            ASSERT_EQ(module.getModuleInfo().getId(), "mock");
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddModuleNullStringId)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>(nullptr);

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            ASSERT_EQ(module.getModuleInfo().getId(), nullptr);
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddModuleEmptyStringId)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>("");

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            ASSERT_EQ(module.getModuleInfo().getId(), "");
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddDuplicateModule)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>();

    ASSERT_NO_THROW(manager.addModule(mock));
    ASSERT_THROW(manager.addModule(mock), DuplicateItemException);
}

TEST_F(ModuleManagerTest, EnumDriver)
{
    auto moduleManager = ModuleManager(SearchDir);
    moduleManager.loadModules(NullContext());

    ListPtr<IModule> moduleDrivers = moduleManager.getModules();
    auto count = moduleDrivers.getCount();

    ASSERT_EQ(count, 3u);

    std::cout << "Module driver count: " << count << std::endl;

#ifdef __GNUC__
    for (const ObjectPtr<IModule>& module : moduleDrivers)
    {
        ModuleInfoPtr info;
        module->getModuleInfo(&info);

        std::cout << info.getName() << std::endl;
    }

#else
    for (const ModulePtr& module : moduleDrivers)
    {
        std::cout << module.getModuleInfo().getName() << std::endl;
    }
#endif
}

TEST_F(ModuleManagerTest, RegisterDaqTypes)
{
    auto typeManager = context.getTypeManager();

    ASSERT_TRUE(typeManager.hasType("PtpSyncInterface"));
    ASSERT_TRUE(typeManager.hasType("SyncInterfaceBase"));
    ASSERT_TRUE(typeManager.hasType("InterfaceClockSync"));
}

// Carries a manufacturer and serial number, so that discovery results can be matched to it
class ScanTestDevice : public Device
{
public:
    ScanTestDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
        , info(DeviceInfo("daqmock://dev"))
    {
        info.setManufacturer("openDAQ");
        info.setSerialNumber("scan_test");
    }

protected:
    DeviceInfoPtr onGetInfo() override
    {
        return info;
    }

private:
    DeviceInfoConfigPtr info;
};

class MockModuleInternal : public MockModuleImpl
{
public:
    MockModuleInternal();

    daq::ErrCode INTERFACE_FUNC getAvailableDevices(daq::IList** availableDevices) override;
    daq::ErrCode INTERFACE_FUNC getAvailableDeviceTypes(daq::IDict** deviceTypes) override;
    daq::ErrCode INTERFACE_FUNC createDevice(daq::IDevice** device, daq::IString* connectionString, daq::IComponent* parent, daq::IPropertyObject* config) override;

    DeviceInfoConfigPtr info;
    DeviceTypePtr type;
    int scanCount = 0;
};

MockModuleInternal::MockModuleInternal()
{
    info = DeviceInfo("daqmock://dev");
    type = DeviceTypeBuilder().setConnectionStringPrefix("daqmock").setId("daqmock").build();
}

daq::ErrCode MockModuleInternal::getAvailableDevices(daq::IList** availableDevices)
{
    scanCount++;
    *availableDevices = List<IDeviceInfo>(info).detach();
    return OPENDAQ_SUCCESS;
}

daq::ErrCode MockModuleInternal::getAvailableDeviceTypes(daq::IDict** deviceTypes)
{
    *deviceTypes = Dict<IString, IDeviceType>({{type.getId(), type}}).detach();
    return OPENDAQ_SUCCESS;
}

daq::ErrCode MockModuleInternal::createDevice(daq::IDevice** device,
                                              daq::IString* connectionString,
                                              daq::IComponent* /*parent*/,
                                              daq::IPropertyObject* /*config*/)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    StringPtr connectionStringPtr = StringPtr::Borrow(connectionString);
    if (connectionStringPtr == "daqmock://invalid_arg")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_ARGUMENT);
    else if (connectionStringPtr == "daqmock://not_found")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
    else if (connectionStringPtr == "daqmock://general_error")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "abc123");

    // A context of its own keeps the device out of the module manager's ownership cycle
    *device = createWithImplementation<IDevice, ScanTestDevice>(NullContext(), nullptr, String("dev")).detach();
    return OPENDAQ_SUCCESS;
}

TEST_F(ModuleManagerTest, TestRescanTimer1)
{
    auto manager = ModuleManager("[[none]]");

    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);
    
    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);
    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);
}

TEST_F(ModuleManagerTest, TestRescanTimer2)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 10}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);
    
    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 2);
}

TEST_F(ModuleManagerTest, ScanOnAddDisabled)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ScanOnAdd", False}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 0);

    // a discovery lookup does not scan either
    ASSERT_FALSE(utils.getDiscoveryInfo("openDAQ", "missing").assigned());
    ASSERT_EQ(impl->scanCount, 0);

    // a smart connection string cannot be resolved without a scan
    ASSERT_THROW(utils.createDevice("daq://openDAQ_missing", nullptr), NotFoundException);
    ASSERT_EQ(impl->scanCount, 0);
}

TEST_F(ModuleManagerTest, ScanOnAddConfigProperty)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ScanOnAdd", False}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    // the default config follows the option; the property overrides it per call
    auto config = utils.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    ASSERT_EQ(static_cast<bool>(general.getPropertyValue("ScanOnAdd")), false);

    utils.createDevice("daqmock", nullptr, config);
    ASSERT_EQ(impl->scanCount, 0);

    general.setPropertyValue("ScanOnAdd", True);
    utils.createDevice("daqmock", nullptr, config);
    ASSERT_EQ(impl->scanCount, 1);
}

TEST_F(ModuleManagerTest, ScanOnAddDefaultEnabled)
{
    auto manager = ModuleManager("[[none]]");
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    auto config = utils.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    ASSERT_EQ(static_cast<bool>(general.getPropertyValue("ScanOnAdd")), true);

    general.setPropertyValue("ScanOnAdd", False);
    utils.createDevice("daqmock", nullptr, config);
    ASSERT_EQ(impl->scanCount, 0);

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);
}

TEST_F(ModuleManagerTest, ParallelDeviceCreationSuccess)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    auto connectionArgs = Dict<IString, IPropertyObject>({{"daqmock://1", nullptr}, {"daqmock://2", nullptr}});
    DictPtr<IString, IDevice> devices;
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr));
    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_EQ(impl->scanCount, 1);

    devices = Dict<IString, IDevice>();
    ASSERT_EQ(utils->createDevices(&devices, connectionArgs, nullptr), OPENDAQ_SUCCESS);

    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr, errCodes, errorInfos));
    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_EQ(errCodes.getCount(), 2u);
    ASSERT_EQ(errorInfos.getCount(), 2u);

    ASSERT_EQ(errCodes.get("daqmock://1"), OPENDAQ_SUCCESS);
    ASSERT_EQ(errCodes.get("daqmock://2"), OPENDAQ_SUCCESS);
    ASSERT_FALSE(errorInfos.get("daqmock://1").assigned());
    ASSERT_FALSE(errorInfos.get("daqmock://2").assigned());
}

TEST_F(ModuleManagerTest, ParallelDeviceCreationPartialSuccess)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    auto connectionArgs = Dict<IString, IPropertyObject>({{"daqmock://1", nullptr},
                                                          {"daqmock://invalid_arg", nullptr},
                                                          {"daqmock://general_error", nullptr}});
    DictPtr<IString, IDevice> devices;
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr));
    ASSERT_EQ(devices.getCount(), 3u);
    ASSERT_EQ(impl->scanCount, 1);

    devices = Dict<IString, IDevice>();
    ASSERT_EQ(utils->createDevices(&devices, connectionArgs, nullptr), OPENDAQ_PARTIAL_SUCCESS);

    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr, errCodes, errorInfos));
    ASSERT_EQ(devices.getCount(), 3u);
    ASSERT_EQ(errCodes.getCount(), 3u);
    ASSERT_EQ(errorInfos.getCount(), 3u);

    ASSERT_EQ(errCodes.get("daqmock://1"), OPENDAQ_SUCCESS);
    ASSERT_EQ(errCodes.get("daqmock://invalid_arg"), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_EQ(errCodes.get("daqmock://general_error"), OPENDAQ_ERR_GENERALERROR);
    ASSERT_FALSE(errorInfos.get("daqmock://1").assigned());
    ASSERT_TRUE(errorInfos.get("daqmock://invalid_arg").assigned());
    ASSERT_TRUE(errorInfos.get("daqmock://general_error").assigned());
}

TEST_F(ModuleManagerTest, ParallelDeviceCreationFailure)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto utils = manager.asPtr<IModuleManagerUtils>();

    ASSERT_THROW_MSG(utils.createDevices(Dict<IString, IPropertyObject>(), nullptr), InvalidParameterException, "None connection arguments provided");

    auto connectionArgs = Dict<IString, IPropertyObject>({{"daqmock://general_error", nullptr}});
    DictPtr<IString, IDevice> devices;
    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();

    ASSERT_ERROR_CODE_EQ(utils->createDevices(&devices, connectionArgs, nullptr, errCodes, errorInfos), OPENDAQ_ERR_GENERALERROR);
    ASSERT_EQ(devices.getCount(), 1u);
    ASSERT_EQ(errCodes.getCount(), 1u);
    ASSERT_EQ(errorInfos.getCount(), 1u);
    ASSERT_TRUE(devices.hasKey("daqmock://general_error"));
    ASSERT_TRUE(errCodes.hasKey("daqmock://general_error"));
    ASSERT_TRUE(errorInfos.hasKey("daqmock://general_error"));

    ObjectPtr<IErrorInfo> errorInfo = errorInfos.get("daqmock://general_error");
    ASSERT_EQ(errCodes.get("daqmock://general_error"), OPENDAQ_ERR_GENERALERROR);
    ASSERT_TRUE(errorInfo.assigned());
    // TODO search for substring within the backtrace information
//    StringPtr message;
//    errorInfo->getMessage(&message);
//    ASSERT_TRUE(message.toStdString().find("abc123") != std::string::npos);
}

class TypesTestModule : public MockModuleImpl
{
public:
    explicit TypesTestModule(const StringPtr& id = "types_test")
        : MockModuleImpl(id)
    {
    }

    daq::ErrCode INTERFACE_FUNC getAvailableDeviceTypes(daq::IDict** types) override
    {
        if (failDeviceTypes)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "no device types");
        *types = deviceTypes.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC getAvailableStreamingTypes(daq::IDict** types) override
    {
        *types = streamingTypes.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC getAvailableDevices(daq::IList** devices) override
    {
        *devices = availableDevices.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC createDevice(daq::IDevice** /*device*/,
                                             daq::IString* connectionString,
                                             daq::IComponent* /*parent*/,
                                             daq::IPropertyObject* /*config*/) override
    {
        createdConnectionStrings.pushBack(connectionString);
        return OPENDAQ_SUCCESS;
    }

    void addDeviceType(const StringPtr& id, const PropertyObjectPtr& config, DeviceKind kind = DeviceKind::Adapter)
    {
        auto builder = DeviceTypeBuilder().setId(id).setName(id).setConnectionStringPrefix(id);
        if (kind != DeviceKind::Unspecified)
            builder.setDeviceKind(kind);
        if (config.assigned())
            builder.setDefaultConfig(config);
        deviceTypes.set(id, builder.build());
    }

    void addStreamingType(const StringPtr& id, const StringPtr& linkedId, const PropertyObjectPtr& config)
    {
        auto builder = StreamingTypeBuilder().setId(id).setName(id).setConnectionStringPrefix(id);
        if (linkedId.assigned() && linkedId.getLength() > 0)
        {
            auto linkedBuilder = DeviceTypeBuilder().setId(linkedId).setName(linkedId).setConnectionStringPrefix(linkedId);
            if (config.assigned())
                linkedBuilder.setDefaultConfig(config);
            builder.setLinkedDeviceType(linkedBuilder.build());
        }
        else if (config.assigned())
        {
            builder.setDefaultConfig(config);
        }
        streamingTypes.set(id, builder.build());
    }

    DictPtr<IString, IDeviceType> deviceTypes = Dict<IString, IDeviceType>();
    DictPtr<IString, IStreamingType> streamingTypes = Dict<IString, IStreamingType>();
    ListPtr<IString> createdConnectionStrings = List<IString>();
    ListPtr<IDeviceInfo> availableDevices = List<IDeviceInfo>();
    bool failDeviceTypes = false;
};

class ModuleManagerTypesTest : public ModuleManagerTest
{
protected:
    ModuleManagerTypesTest()
        : manager(ModuleManager("[[none]]"))
    {
        manager.loadModules(NullContext());
    }

    static PropertyObjectPtr deviceConfig()
    {
        auto nested = PropertyObject();
        nested.addProperty(IntProperty("A", 1));
        nested.addProperty(StringProperty("B", "b"));

        auto config = PropertyObject();
        config.addProperty(StringProperty("Host", "localhost"));
        config.addProperty(IntProperty("Port", 7420));
        config.addProperty(ObjectProperty("Nested", nested));
        return config;
    }

    static PropertyObjectPtr streamingConfig()
    {
        auto nested = PropertyObject();
        nested.addProperty(IntProperty("A", 1));

        auto config = PropertyObject();
        config.addProperty(IntProperty("Port", 7420));
        config.addProperty(ObjectProperty("Nested", nested));
        return config;
    }

    struct TypesModule
    {
        ModulePtr module;
        TypesTestModule* impl;
    };

    static TypesModule makeModule(const std::string& id = "types_test")
    {
        auto* impl = new TypesTestModule(id);
        IModule* intf = impl;
        return {ModulePtr(intf), impl};
    }

    // Adds the module; returns an empty string on success, else the error message
    std::string tryAdd(const ModulePtr& module)
    {
        const ErrCode errCode = manager->addModule(module);
        if (OPENDAQ_SUCCEEDED(errCode))
        {
            ObjectPtr<IErrorInfo> errorInfo;
            daqGetErrorInfo(&errorInfo);
            EXPECT_FALSE(errorInfo.assigned()) << "error info left set by a successful add";
            daqClearErrorInfo();
            EXPECT_TRUE(isAdded(module));
            return {};
        }

        EXPECT_EQ(errCode, OPENDAQ_ERR_MODULE_LOAD_FAILED);
        EXPECT_FALSE(isAdded(module));

        ObjectPtr<IErrorInfo> errorInfo;
        daqGetErrorInfo(&errorInfo);
        daqClearErrorInfo();
        StringPtr message;
        if (errorInfo.assigned())
            errorInfo->getMessage(&message);
        return message.assigned() ? message.toStdString() : std::string("<no message>");
    }

    bool isAdded(const ModulePtr& module)
    {
        for (const auto& m : manager.getModules())
            if (m == module)
                return true;
        return false;
    }

    static void expectContains(const std::string& message, const std::string& part)
    {
        EXPECT_NE(message.find(part), std::string::npos) << "\"" << part << "\" not in: " << message;
    }

    ModuleManagerPtr manager;
};

TEST_F(ModuleManagerTypesTest, LinkToUnknownDeviceTypeRejected)
{
    auto lib = makeModule("link_test_unknown");
    auto* impl = lib.impl;
    impl->addDeviceType("dev", deviceConfig());
    impl->addStreamingType("str", "other", streamingConfig());

    const auto message = tryAdd(lib.module);
    expectContains(message, R"(Module with id "link_test_unknown")");
    expectContains(message, R"(streaming type "str")");
    expectContains(message, R"(device type "other")");
    expectContains(message, "does not provide");
}

TEST_F(ModuleManagerTypesTest, LinkToDeviceTypeOfAnotherModuleRejected)
{
    auto deviceLib = makeModule("device_module");
    auto* deviceModule = deviceLib.impl;
    deviceModule->addDeviceType("dev", deviceConfig());
    ASSERT_EQ(tryAdd(deviceLib.module), "");

    auto streamingLib = makeModule("streaming_module");
    auto* streamingModule = streamingLib.impl;
    streamingModule->addStreamingType("str", "dev", streamingConfig());

    const auto message = tryAdd(streamingLib.module);
    expectContains(message, R"(Module with id "streaming_module")");
    expectContains(message, R"(device type "dev", which the module does not provide)");
}

TEST_F(ModuleManagerTypesTest, DeviceTypesNotListedRejected)
{
    auto lib = makeModule();
    auto* impl = lib.impl;
    impl->addStreamingType("str", "dev", streamingConfig());
    impl->failDeviceTypes = true;

    expectContains(tryAdd(lib.module), "device types cannot be listed");
}

TEST_F(ModuleManagerTypesTest, UnlinkedStreamingTypesUnaffected)
{
    auto streaming = PropertyObject();
    streaming.addProperty(StringProperty("Port", "7420"));
    streaming.addProperty(BoolProperty("OnlyHere", true));

    auto lib = makeModule();
    auto* impl = lib.impl;
    impl->addDeviceType("dev", deviceConfig());
    impl->addStreamingType("str", "", streaming);
    impl->addStreamingType("str2", nullptr, nullptr);
    impl->failDeviceTypes = true;

    ASSERT_EQ(tryAdd(lib.module), "");
}

TEST_F(ModuleManagerTypesTest, StreamingTypesNotImplementedUnaffected)
{
    // MockModuleImpl::getAvailableStreamingTypes fails with OPENDAQ_ERR_NOTIMPLEMENTED
    const auto module = createWithImplementation<IModule, MockModuleImpl>(String("not_implemented"));
    ASSERT_EQ(manager->addModule(module), OPENDAQ_SUCCESS);

    ObjectPtr<IErrorInfo> errorInfo;
    daqGetErrorInfo(&errorInfo);
    ASSERT_FALSE(errorInfo.assigned());
    ASSERT_TRUE(isAdded(module));
}

TEST_F(ModuleManagerTypesTest, OneBadLinkRejectsModule)
{
    auto lib = makeModule();
    auto* impl = lib.impl;
    impl->addDeviceType("dev", deviceConfig());
    impl->addStreamingType("good", "dev", streamingConfig());
    impl->addStreamingType("bad", "other", streamingConfig());

    expectContains(tryAdd(lib.module), R"(streaming type "bad")");
}

TEST_F(ModuleManagerTypesTest, UnspecifiedDeviceKindLogsInfo)
{
    const auto sink = LastMessageLoggerSink();
    sink.setLevel(LogLevel::Info);
    const LastMessageLoggerSinkPrivatePtr lastMessage = sink;
    manager = ModuleManager("[[none]]");
    manager.loadModules(Context(nullptr, LoggerWithSinks(List<ILoggerSink>(sink)), TypeManager(), nullptr, nullptr));

    auto lib = makeModule();
    lib.impl->addDeviceType("unspecdev", nullptr, DeviceKind::Unspecified);
    lib.impl->addDeviceType("extdev", nullptr, DeviceKind::Adapter);
    ASSERT_EQ(tryAdd(lib.module), "");

    bool logged = false;
    while (!logged && lastMessage.waitForMessage(1000))
        logged = lastMessage.getLastMessage().toStdString().find(R"(Device type "unspecdev")") != std::string::npos;
    ASSERT_TRUE(logged);
}

TEST_F(ModuleManagerTypesTest, UnlistableStreamingTypeLogsWarning)
{
    const auto sink = LastMessageLoggerSink();
    sink.setLevel(LogLevel::Warn);
    const LastMessageLoggerSinkPrivatePtr lastMessage = sink;
    manager = ModuleManager("[[none]]");
    manager.loadModules(Context(nullptr, LoggerWithSinks(List<ILoggerSink>(sink)), TypeManager(), nullptr, nullptr));

    auto lib = makeModule();
    lib.impl->addDeviceType("mockdev", nullptr, DeviceKind::Mirrored);
    lib.impl->addStreamingType("StreamingMode", "mockdev", nullptr);
    lib.impl->addStreamingType("mockstr", "mockdev", nullptr);
    ASSERT_EQ(tryAdd(lib.module), "");

    const auto config = manager.asPtr<IModuleManagerUtils>().createAddDeviceConfig(DeviceInfo("mockdev://"));
    const auto options = config.getStreamingOptions();
    ASSERT_EQ(options.getStreamingConfigs().getKeyList(), List<IString>("mockstr"));
    ASSERT_EQ(options.getStreamingConfig().getStreamingType().getId(), "mockstr");

    bool logged = false;
    while (!logged && lastMessage.waitForMessage(1000))
        logged = lastMessage.getLastMessage().toStdString().find(R"(Streaming type "StreamingMode")") != std::string::npos;
    ASSERT_TRUE(logged);
}

TEST_F(ModuleManagerTypesTest, DeviceKindChecksParent)
{
    auto lib = makeModule();
    lib.impl->addDeviceType("rootdev", nullptr, DeviceKind::Root);
    lib.impl->addDeviceType("extdev", nullptr, DeviceKind::Adapter);
    lib.impl->addDeviceType("mirdev", nullptr, DeviceKind::Mirrored);
    lib.impl->addDeviceType("unspecdev", nullptr, DeviceKind::Unspecified);
    ASSERT_EQ(tryAdd(lib.module), "");

    const ModuleManagerUtilsPtr utils = manager;
    const auto parent = Component(NullContext(), nullptr, "parent");
    const auto create = [&utils](const char* connectionString, const ComponentPtr& owner)
    {
        DevicePtr device;
        const ErrCode errCode = utils->createDevice(&device, String(connectionString), owner, nullptr);
        daqClearErrorInfo();
        return errCode;
    };

    ASSERT_EQ(create("rootdev://", parent), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(create("extdev://", nullptr), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(create("mirdev://", nullptr), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(lib.impl->createdConnectionStrings.getCount(), 0u);

    ASSERT_EQ(create("rootdev://", nullptr), OPENDAQ_SUCCESS);
    ASSERT_EQ(create("extdev://", parent), OPENDAQ_SUCCESS);
    ASSERT_EQ(create("mirdev://", parent), OPENDAQ_SUCCESS);
    ASSERT_EQ(create("unspecdev://", nullptr), OPENDAQ_SUCCESS);
    ASSERT_EQ(create("unspecdev://", parent), OPENDAQ_SUCCESS);
    ASSERT_EQ(lib.impl->createdConnectionStrings.getCount(), 5u);
}

TEST_F(ModuleManagerTypesTest, RootKindNotListed)
{
    auto lib = makeModule();
    lib.impl->addDeviceType("rootdev", nullptr, DeviceKind::Root);
    lib.impl->addDeviceType("extdev", nullptr, DeviceKind::Adapter);
    for (const auto& prefix : {"rootdev", "extdev"})
    {
        const auto info = DeviceInfo(std::string(prefix) + "://10.0.0.5");
        info.setDeviceType(lib.impl->deviceTypes.get(prefix));
        lib.impl->availableDevices.pushBack(info);
    }
    lib.impl->availableDevices.pushBack(DeviceInfo("rootdev://untyped"));
    ASSERT_EQ(tryAdd(lib.module), "");

    std::set<std::string> listed;
    for (const auto& info : manager.asPtr<IModuleManagerUtils>().getAvailableDevices())
        listed.insert(info.getConnectionString());

    const std::set<std::string> expected{"extdev://10.0.0.5", "rootdev://untyped"};
    ASSERT_EQ(listed, expected);
}

class ModuleManagerAddConfigTest : public ModuleManagerTypesTest
{
protected:
    ModuleManagerAddConfigTest()
    {
        auto lib = makeModule("add_config");
        module = lib.impl;
        module->addDeviceType("mockdev", deviceConfig(), DeviceKind::Mirrored);
        module->addDeviceType("extdev", nullptr, DeviceKind::Adapter);
        module->addDeviceType("unlinked", nullptr, DeviceKind::Mirrored);
        module->addDeviceType("rootdev", nullptr, DeviceKind::Root);
        module->addStreamingType("mockstr", "mockdev", deviceConfig());
        module->addStreamingType("free", nullptr, streamingConfig());
        manager->addModule(lib.module);
    }

    struct Cap
    {
        std::string prefix;
        ProtocolType type;
        std::string protocolId = "";
    };

    using Caps = std::vector<Cap>;
    using Strs = std::vector<std::string>;

    static constexpr auto cfg = ProtocolType::Configuration;
    static constexpr auto streaming = ProtocolType::Streaming;
    static constexpr auto both = ProtocolType::ConfigurationAndStreaming;

    static DeviceInfoConfigPtr deviceInfo(const std::string& connectionString, const Caps& capabilities = {})
    {
        auto info = DeviceInfo(connectionString);
        for (size_t i = 0; i < capabilities.size(); ++i)
        {
            const auto& cap = capabilities[i];
            const auto id = cap.protocolId.empty() ? cap.prefix + std::to_string(i) : cap.protocolId;
            info.asPtr<IDeviceInfoInternal>().addServerCapability(ServerCapability(id, id, cap.type).setPrefix(cap.prefix));
        }
        return info;
    }

    ErrCode create(const DeviceInfoPtr& info, const StringPtr& prefix, AddDeviceConfigPtr& config)
    {
        IAddDeviceConfig* raw = nullptr;
        const ErrCode errCode = manager.asPtr<IModuleManagerUtils>()->createAddDeviceConfig(&raw, info, prefix);
        config = AddDeviceConfigPtr::Adopt(raw);
        daqClearErrorInfo();
        return errCode;
    }

    AddDeviceConfigPtr create(const DeviceInfoPtr& info, const StringPtr& prefix = nullptr)
    {
        AddDeviceConfigPtr config;
        EXPECT_EQ(create(info, prefix, config), OPENDAQ_SUCCESS);
        return config;
    }

    static std::vector<std::string> listed(const AddDeviceConfigPtr& config)
    {
        std::vector<std::string> ids;
        for (const auto& [id, _] : config.getStreamingOptions().getStreamingConfigs())
            ids.push_back(id);
        return ids;
    }

    static std::string selected(const AddDeviceConfigPtr& config)
    {
        const auto streamingConfig = config.getStreamingOptions().getStreamingConfig();
        return streamingConfig.assigned() ? streamingConfig.getStreamingType().getId().toStdString() : "";
    }

    TypesTestModule* module = nullptr;
};

TEST_F(ModuleManagerAddConfigTest, DiscoveredDeviceType)
{
    struct Row
    {
        Caps caps;
        std::string expectedDevice;
    };

    const std::vector<Row> rows = {
        {{{"extdev", cfg}, {"mockdev", both}}, "mockdev"},
        {{{"mockdev", cfg}, {"extdev", both}}, "extdev"},
        {{{"mockdev", cfg}, {"extdev", cfg}}, "mockdev"},
        {{{"extdev", cfg}, {"mockdev", cfg}}, "extdev"},
        {{{"unloaded", both}, {"extdev", cfg}}, "extdev"},
        {{{"rootdev", both}, {"extdev", cfg}}, "extdev"},
        {{{"mockdev", both}, {"extdev", cfg, "OpenDAQNativeConfiguration"}}, "extdev"},
    };

    for (const auto& row : rows)
        ASSERT_EQ(create(deviceInfo("daq://x", row.caps)).getDeviceType().getId(), row.expectedDevice);

    AddDeviceConfigPtr config;
    ASSERT_EQ(create(deviceInfo("daq://x", {{"unloaded", both}}), nullptr, config), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(ModuleManagerAddConfigTest, DiscoveredExplicitPrefix)
{
    const auto info = deviceInfo("daq://x", {{"mockdev", cfg}, {"extdev", cfg}, {"unloaded", cfg}});

    ASSERT_EQ(create(info, "extdev").getDeviceType().getId(), "extdev");

    AddDeviceConfigPtr config;
    ASSERT_EQ(create(info, "free", config), OPENDAQ_ERR_NOTFOUND);
    ASSERT_EQ(create(info, "unloaded", config), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(ModuleManagerAddConfigTest, DiscoveredStreamingTypes)
{
    struct Row
    {
        Caps caps;
        Strs expectedListed;
        std::string expectedSelected;
    };

    const std::vector<Row> rows = {
        {{{"mockdev", cfg}, {"free", streaming}, {"mockstr", both}, {"unloaded", streaming}}, {"free", "mockstr"}, "mockstr"},
        {{{"mockdev", cfg}, {"free", streaming}}, {"free"}, "free"},
        {{{"mockdev", cfg}, {"free", streaming}, {"free", both}}, {"free"}, "free"},
        {{{"mockdev", cfg}}, {}, ""},
    };

    for (const auto& row : rows)
    {
        const auto config = create(deviceInfo("daq://x", row.caps), "mockdev");
        ASSERT_EQ(listed(config), row.expectedListed);
        ASSERT_EQ(selected(config), row.expectedSelected);
    }
}

TEST_F(ModuleManagerAddConfigTest, ConnectionStringOnly)
{
    const auto bare = create(deviceInfo("mockdev://"));
    ASSERT_EQ(bare.getDeviceType().getId(), "mockdev");
    ASSERT_EQ(listed(bare), (Strs{"mockstr", "free"}));
    ASSERT_EQ(selected(bare), "mockstr");

    const auto withHost = create(deviceInfo("mockdev://192.168.0.1"), "mockdev");
    ASSERT_EQ(withHost.getDeviceType().getId(), "mockdev");

    const auto unlinked = create(deviceInfo("unlinked://"));
    ASSERT_EQ(listed(unlinked), (Strs{"mockstr", "free"}));
    ASSERT_EQ(selected(unlinked), "");

    AddDeviceConfigPtr config;
    ASSERT_EQ(create(deviceInfo("mockdev://"), "extdev", config), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(create(deviceInfo("unloaded://"), nullptr, config), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(ModuleManagerAddConfigTest, ScanOnAddSeededFromModuleManager)
{
    ASSERT_TRUE(create(deviceInfo("mockdev://")).getScanOnAdd());

    auto disabled = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ScanOnAdd", False}})}});
    const auto context = Context(nullptr, Logger(), nullptr, disabled, nullptr, options);

    auto lib = makeModule();
    lib.impl->addDeviceType("mockdev", nullptr);
    ASSERT_EQ(disabled->addModule(lib.module), OPENDAQ_SUCCESS);

    ASSERT_FALSE(disabled.asPtr<IModuleManagerUtils>().createAddDeviceConfig(deviceInfo("mockdev://")).getScanOnAdd());
}

TEST_F(ModuleManagerAddConfigTest, RootKindConfigRejected)
{
    AddDeviceConfigPtr config;
    ASSERT_EQ(create(deviceInfo("rootdev://"), nullptr, config), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(create(deviceInfo("daq://x", {{"rootdev", cfg}, {"extdev", cfg}}), "rootdev", config), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(create(deviceInfo("daq://x", {{"rootdev", cfg}}), nullptr, config), OPENDAQ_ERR_INVALIDPARAMETER);
}
