#include <gtest/gtest.h>
#include <opendaq/instance_factory.h>
#include <opendaq/mock/mock_device_module.h>
#include <coreobjects/user_factory.h>
#include <opendaq/search_filter_factory.h>
#include <coreobjects/authentication_provider_factory.h>
#include <opendaq/device_private_ptr.h>

using namespace daq;

class TestDeviceLocking : public testing::Test
{
public:
    const UserPtr UserTomaz = User("tomaz", "tomaz");
    const UserPtr UserJure = User("jure", "jure");

    InstancePtr createServerInstance()
    {
        const auto moduleManager = ModuleManager("");
        auto logger = Logger();

        auto users = List<IUser>();
        users.pushBack(UserTomaz);
        users.pushBack(UserJure);
        auto authenticationProvider = StaticAuthenticationProvider(false, users);

        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        auto instance = InstanceCustom(context, "clientInstance");
        instance.addDevice("daqmock://phys_device");
        instance.addStandardServers();

        return instance;
    }

    InstancePtr connectClientInstance(const std::string& username, const std::string& password)
    {
        auto instance = Instance();

        const ModulePtr deviceModule(MockDeviceModule_Create(instance.getContext()));
        instance.getModuleManager().addModule(deviceModule);

        auto config = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr generalConfig = config.getPropertyValue("General");

        generalConfig.setPropertyValue("Username", username);
        generalConfig.setPropertyValue("Password", password);

        instance.addDevice("daq.nd://127.0.0.1", config);
        instance.addDevice("daqmock://phys_device");
        instance.addDevice("daqmock://phys_device");
        return instance;
    }
};


TEST_F(TestDeviceLocking, LockSingleClientDevice)
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

TEST_F(TestDeviceLocking, LockBottomUp)
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

TEST_F(TestDeviceLocking, LockTopDown)
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

TEST_F(TestDeviceLocking, UnlockBottomUp)
{
    auto serverInstance = createServerInstance();
    auto clientInstance = connectClientInstance("tomaz", "tomaz");

    auto devices = clientInstance.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(devices.getCount(), 4u);

    clientInstance.lock();

    devices[3].unlock();
    devices[2].unlock();
    devices[1].unlock();
    devices[0].unlock();
    clientInstance.unlock();

    ASSERT_FALSE(clientInstance.isLocked());
    ASSERT_FALSE(devices[0].isLocked());
    ASSERT_FALSE(devices[1].isLocked());
    ASSERT_FALSE(devices[2].isLocked());
    ASSERT_FALSE(devices[3].isLocked());
}

TEST_F(TestDeviceLocking, UnlockTopDown)
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

TEST_F(TestDeviceLocking, LockUnlockRoot)
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

TEST_F(TestDeviceLocking, LockUnlockRootWithUser)
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

TEST_F(TestDeviceLocking, AlreadyLockedSameUser)
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

TEST_F(TestDeviceLocking, AlreadyLockedDifferentUser)
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

TEST_F(TestDeviceLocking, LockUnlockSpecific)
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

TEST_F(TestDeviceLocking, UnlockUnlocked)
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

TEST_F(TestDeviceLocking, LockRevert)
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

TEST_F(TestDeviceLocking, UnlockRevert)
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

TEST_F(TestDeviceLocking, LockedWithAnonymousUser)
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
