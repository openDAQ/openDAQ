#include <testutils/testutils.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/logger_factory.h>
#include "mock/mock_module.h"

#include <chrono>
#include <thread>

#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
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

// Linked streaming types: checked when a module is added

class LinkTestModule : public MockModuleImpl
{
public:
    explicit LinkTestModule(const StringPtr& id = "link_test")
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

    void addDeviceType(const StringPtr& id, const PropertyObjectPtr& config)
    {
        auto builder = DeviceTypeBuilder().setId(id).setName(id).setConnectionStringPrefix(id);
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
    bool failDeviceTypes = false;
};

class ModuleManagerLinkTest : public ModuleManagerTest
{
protected:
    ModuleManagerLinkTest()
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

    struct LinkModule
    {
        ModulePtr module;
        LinkTestModule* impl;
    };

    static LinkModule makeModule(const std::string& id = "link_test")
    {
        auto* impl = new LinkTestModule(id);
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

TEST_F(ModuleManagerLinkTest, LinkToUnknownDeviceTypeRejected)
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

TEST_F(ModuleManagerLinkTest, LinkToDeviceTypeOfAnotherModuleRejected)
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

TEST_F(ModuleManagerLinkTest, DeviceTypesNotListedRejected)
{
    auto lib = makeModule();
    auto* impl = lib.impl;
    impl->addStreamingType("str", "dev", streamingConfig());
    impl->failDeviceTypes = true;

    expectContains(tryAdd(lib.module), "device types cannot be listed");
}

TEST_F(ModuleManagerLinkTest, UnlinkedStreamingTypesUnaffected)
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

TEST_F(ModuleManagerLinkTest, StreamingTypesNotImplementedUnaffected)
{
    // MockModuleImpl::getAvailableStreamingTypes fails with OPENDAQ_ERR_NOTIMPLEMENTED
    const auto module = createWithImplementation<IModule, MockModuleImpl>(String("not_implemented"));
    ASSERT_EQ(manager->addModule(module), OPENDAQ_SUCCESS);

    ObjectPtr<IErrorInfo> errorInfo;
    daqGetErrorInfo(&errorInfo);
    ASSERT_FALSE(errorInfo.assigned());
    ASSERT_TRUE(isAdded(module));
}

TEST_F(ModuleManagerLinkTest, OneBadLinkRejectsModule)
{
    auto lib = makeModule();
    auto* impl = lib.impl;
    impl->addDeviceType("dev", deviceConfig());
    impl->addStreamingType("good", "dev", streamingConfig());
    impl->addStreamingType("bad", "other", streamingConfig());

    expectContains(tryAdd(lib.module), R"(streaming type "bad")");
}
