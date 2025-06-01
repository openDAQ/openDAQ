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

    ASSERT_EQ(count, 0u);

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

    *device = DevicePtr();
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

TEST_F(ModuleManagerTest, ParallelDeviceCreationSuccess)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 10}})}});
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
}
