#include "test_helpers/test_helpers.h"
#include <opendaq/instance_factory.h>
#include <opendaq/mock/mock_device_module.h>
#include <coreobjects/user_factory.h>
#include <opendaq/search_filter_factory.h>
#include <coreobjects/authentication_provider_factory.h>
#include <native_streaming_client_module/module_dll.h>
#include <opendaq/device_private_ptr.h>
#include <native_streaming_server_module/module_dll.h>
#include <atomic>
#include <thread>

using namespace daq;

// The param - whether the legacy add-device config is used to supply the credentials (see test_helpers::configModeName)
class DeviceLockingTest : public testing::TestWithParam<bool>
{
public:
    const UserPtr UserTomaz = User("tomaz", "tomaz");
    const UserPtr UserJure = User("jure", "jure");

    InstancePtr createServerInstance()
    {
        const auto moduleManager = ModuleManager("[[none]]");
        auto logger = Logger();

        auto users = List<IUser>();
        users.pushBack(UserTomaz);
        users.pushBack(UserJure);
        auto authenticationProvider = StaticAuthenticationProvider(false, users);

        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager, authenticationProvider);
        {
            const ModulePtr mockModule(MockDeviceModule_Create(context));
            
            ModulePtr nativeServerModule;
            createNativeStreamingServerModule(&nativeServerModule, context);

            moduleManager.addModule(mockModule);
            moduleManager.addModule(nativeServerModule);
        }

        auto instance = InstanceCustom(context, "clientInstance");
        instance.addDevice("daqmock://phys_device");
        test_helpers::addServer(instance, "OpenDAQNativeStreaming");

        return instance;
    }

    InstancePtr connectClientInstance(const std::string& username, const std::string& password)
    {
        auto instance = test_helpers::createInstance("[[none]]");
        {
            auto context = instance.getContext();

            ModulePtr nativeClientModule;
            createNativeStreamingClientModule(&nativeClientModule, context);            

            const ModulePtr deviceModule(MockDeviceModule_Create(context));
            auto mm = instance.getModuleManager();
            
            mm.addModule(deviceModule);
            mm.addModule(nativeClientModule);
        }

        const auto connectionString = test_helpers::withPort("daq.nd://127.0.0.1");
        auto config = test_helpers::createAddDeviceConfig(instance, connectionString, GetParam());
        test_helpers::setCredentials(config, username, password, GetParam());

        instance.addDevice(connectionString, config);
        instance.addDevice("daqmock://phys_device");
        instance.addDevice("daqmock://phys_device");
        return instance;
    }
};


TEST_P(DeviceLockingTest, LockSingleClientDevice)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(devices.getCount(), 4u);

    devices[3].lock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
    ASSERT_TRUE(devices[3].isLocked());
}

TEST_P(DeviceLockingTest, LockBottomUp)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(devices.getCount(), 4u);

    devices[3].lock();
    devices[2].lock();
    devices[1].lock();
    devices[0].lock();
    clientInstance.lock();

    ASSERT_TRUE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_TRUE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());
    ASSERT_TRUE(devices[3].isLocked());
}

TEST_P(DeviceLockingTest, LockTopDown)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(devices.getCount(), 4u);

    devices[0].lock();
    devices[1].lock();
    devices[2].lock();
    devices[3].lock();
    clientInstance.lock();

    ASSERT_TRUE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_TRUE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());
    ASSERT_TRUE(devices[3].isLocked());
}

TEST_P(DeviceLockingTest, UnlockTopDown)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(devices.getCount(), 4u);

    clientInstance.lock();

    clientInstance.unlock();
    devices[0].unlock();
    devices[1].unlock();
    devices[2].unlock();
    devices[3].unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
    ASSERT_FALSE(devices[3].isLocked());
}

TEST_P(DeviceLockingTest, LockUnlockRoot)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    clientInstance.lock();

    ASSERT_TRUE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_TRUE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());

    clientInstance.unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, LockUnlockRootWithUser)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    clientInstance.getRootDevice().asPtr<IDevicePrivate>().lock(UserJure);

    ASSERT_TRUE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_TRUE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());

    ASSERT_THROW(clientInstance.getRootDevice().asPtr<IDevicePrivate>().unlock(UserTomaz), AccessDeniedException);
    clientInstance.getRootDevice().asPtr<IDevicePrivate>().unlock(UserJure);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, AlreadyLockedSameUser)
{
    auto serverInstance = createServerInstance();
    serverInstance.getRootDevice().asPtr<IDevicePrivate>().lock(UserTomaz);

    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    clientInstance.unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, AlreadyLockedDifferentUser)
{
    auto serverInstance = createServerInstance();
    serverInstance.getRootDevice().asPtr<IDevicePrivate>().lock(UserJure);

    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    ASSERT_THROW(clientInstance.lock(), DeviceLockedException);
    ASSERT_THROW(clientInstance.unlock(), AccessDeniedException);
}

TEST_P(DeviceLockingTest, LockUnlockSpecific)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    devices[0].lock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    devices[0].unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, UnlockUnlocked)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    devices[0].lock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    clientInstance.unlock();
    clientInstance.unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, LockRevert)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    devices[0].lock();
    devices[2].asPtr<IDevicePrivate>().lock(UserJure);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());

    ASSERT_THROW(clientInstance.lock(), DeviceLockedException);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, UnlockRevert)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    devices[0].lock();
    devices[2].asPtr<IDevicePrivate>().lock(UserJure);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());

    ASSERT_THROW(clientInstance.unlock(), AccessDeniedException);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, LockedWithAnonymousUser)
{
    const auto anonymousUser = User("", "");

    auto serverInstance = createServerInstance();
    serverInstance.getRootDevice().asPtr<IDevicePrivate>().lock(anonymousUser);

    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());

    clientInstance.unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, UnlockChild)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);
    ASSERT_EQ(devices[0].getDevices().getCount(), 1u);

    clientInstance.lock();

    ASSERT_TRUE(clientInstance.isLocked());
    ASSERT_TRUE(devices[0].isLocked());
    ASSERT_TRUE(devices[0].getDevices()[0].isLocked());
    ASSERT_TRUE(devices[1].isLocked());
    ASSERT_TRUE(devices[2].isLocked());

    ASSERT_THROW(devices[0].getDevices()[0].unlock(), DeviceLockedException);
    ASSERT_THROW(devices[1].unlock(), DeviceLockedException);
    ASSERT_THROW(devices[2].unlock(), DeviceLockedException);

    clientInstance.unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[0].getDevices()[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
}

TEST_P(DeviceLockingTest, ForceUnlock)
{
    auto serverInstance = createServerInstance();
    serverInstance.getRootDevice().asPtr<IDevicePrivate>().lock(UserJure);

    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices();
    ASSERT_EQ(devices.getCount(), 3u);
    ASSERT_TRUE(devices[0].isLocked());

    ASSERT_THROW(clientInstance.unlock(), AccessDeniedException);

    clientInstance.getRootDevice().asPtr<IDevicePrivate>().forceUnlock();
    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
}

// An event of the device that arrives while a lock or unlock call waits for its reply must not hold up that reply
TEST_P(DeviceLockingTest, LockUnlockWhileServerChangesDevice)
{
    auto serverInstance = createServerInstance();
    const auto serverDevice = serverInstance.getRootDevice();
    serverDevice.addProperty(IntProperty("Counter", 0));

    auto clientInstance = connectClientInstance("tomaz", "tomaz");
    const auto device = clientInstance.getDevices()[0];
    ASSERT_TRUE(device.hasProperty("Counter"));

    std::atomic<bool> stop{false};
    std::thread writer(
        [&serverDevice, &stop]
        {
            for (Int value = 1; !stop; ++value)
            {
                serverDevice.setPropertyValue("Counter", value);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

    // Locks and unlocks until enough events of the device have arrived for some to have met a call in progress
    const Int eventCount = 20;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    std::string error;
    try
    {
        while (static_cast<Int>(device.getPropertyValue("Counter")) < eventCount && std::chrono::steady_clock::now() < deadline)
        {
            device.lock();
            device.unlock();
            device.lock();
            device.asPtr<IDevicePrivate>().forceUnlock();
        }
    }
    catch (const std::exception& e)
    {
        error = e.what();
    }

    stop = true;
    writer.join();

    ASSERT_TRUE(error.empty()) << error;
    ASSERT_GE(static_cast<Int>(device.getPropertyValue("Counter")), eventCount);
    ASSERT_FALSE(device.isLocked());
}

INSTANTIATE_TEST_SUITE_P(ConfigModes, DeviceLockingTest, testing::Bool(), test_helpers::configModeName);
