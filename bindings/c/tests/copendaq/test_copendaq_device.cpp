#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqDeviceTest : public testing::Test
{
    void SetUp() override
    {
        InstanceBuilder* builder = nullptr;
        InstanceBuilder_createInstanceBuilder(&builder);
        InstanceBuilder_setGlobalLogLevel(builder, LogLevel::LogLevelDebug);
        String* component = nullptr;
        String_createString(&component, "Instance");
        InstanceBuilder_setComponentLogLevel(builder, component, LogLevel::LogLevelError);
        BaseObject_releaseRef(component);
        LoggerSink* sink = nullptr;
        LoggerSink_createStdErrLoggerSink(&sink);
        InstanceBuilder_addLoggerSink(builder, sink);
        BaseObject_releaseRef(sink);
        InstanceBuilder_setSchedulerWorkerNum(builder, 1);
        String* modulePath = nullptr;
        String_createString(&modulePath, ".");
        InstanceBuilder_addModulePath(builder, modulePath);
        BaseObject_releaseRef(modulePath);

        InstanceBuilder_build(builder, &instance);
        BaseObject_releaseRef(builder);
        Instance_getRootDevice(instance, &dev);
    }

    void TearDown() override
    {
        BaseObject_releaseRef(instance);
        BaseObject_releaseRef(dev);
    }

protected:
    Instance* instance = nullptr;
    Device* dev = nullptr;
};

TEST_F(COpendaqDeviceTest, AddressInfo)
{
    AddressInfoBuilder* builder = nullptr;
    AddressInfoBuilder_createAddressInfoBuilder(&builder);

    String* connectionString = nullptr;
    String_createString(&connectionString, "daqref://device0");
    AddressInfoBuilder_setConnectionString(builder, connectionString);
    AddressInfoBuilder_setReachabilityStatus(builder, AddressReachabilityStatusUnknown);
    String* type = nullptr;
    String_createString(&type, "Type");
    AddressInfoBuilder_setType(builder, type);
    BaseObject_releaseRef(type);
    String* address = nullptr;
    String_createString(&address, "Address");
    AddressInfoBuilder_setAddress(builder, connectionString);
    AddressInfoBuilder_setAddress(builder, address);
    BaseObject_releaseRef(address);
    BaseObject_releaseRef(connectionString);

    AddressInfo* addressInfo = nullptr;
    AddressInfoBuilder_build(builder, &addressInfo);
    ASSERT_NE(addressInfo, nullptr);

    String* connectionStringOut = nullptr;
    AddressInfo_getConnectionString(addressInfo, &connectionStringOut);
    ASSERT_NE(connectionStringOut, nullptr);
    ConstCharPtr connectionStringStr = nullptr;
    String_getCharPtr(connectionStringOut, &connectionStringStr);
    printf("ConnectionString %s\n", connectionStringStr);
    BaseObject_releaseRef(connectionStringOut);
    BaseObject_releaseRef(addressInfo);
    BaseObject_releaseRef(builder);
}

TEST_F(COpendaqDeviceTest, DeviceInfo)
{
    DeviceInfo* deviceInfo = nullptr;
    Device_getInfo(dev, &deviceInfo);
    ASSERT_NE(deviceInfo, nullptr);
    String* connectionString = nullptr;
    DeviceInfo_getConnectionString(deviceInfo, &connectionString);
    ASSERT_NE(connectionString, nullptr);
    ConstCharPtr connectionStringStr = nullptr;
    String_getCharPtr(connectionString, &connectionStringStr);

    String* name = nullptr;
    DeviceInfo_getName(deviceInfo, &name);
    ConstCharPtr nameStr = nullptr;
    String_getCharPtr(name, &nameStr);

    printf("ConnectionString %s\n", connectionStringStr);
    printf("Name %s\n", nameStr);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(connectionString);
    BaseObject_releaseRef(deviceInfo);
}

TEST_F(COpendaqDeviceTest, Device)
{
    String* connectionString = nullptr;
    String_createString(&connectionString, "daqref://device0");

    Device* connectedDevice = nullptr;
    Device_addDevice(dev, &connectedDevice, connectionString, nullptr);
    ASSERT_NE(connectedDevice, nullptr);

    DeviceInfo* connectedDeviceInfo = nullptr;
    Device_getInfo(connectedDevice, &connectedDeviceInfo);
    ASSERT_NE(connectedDeviceInfo, nullptr);

    DeviceType* connectedDeviceType = nullptr;
    DeviceInfo_getDeviceType(connectedDeviceInfo, &connectedDeviceType);
    ASSERT_NE(connectedDeviceType, nullptr);

    BaseObject_releaseRef(connectedDeviceType);
    BaseObject_releaseRef(connectedDeviceInfo);
    BaseObject_releaseRef(connectedDevice);
    BaseObject_releaseRef(connectionString);
}

TEST_F(COpendaqDeviceTest, IoFolderConfig)
{
    Component* component = nullptr;
    BaseObject_borrowInterface(instance, COMPONENT_INTF_ID, reinterpret_cast<BaseObject**>(&component));
    ASSERT_NE(component, nullptr);

    FolderConfig* folderConfig = nullptr;
    String* localId = nullptr;
    String_createString(&localId, "IoFolder");
    Context* ctx = nullptr;
    Component_getContext(component, &ctx);
    ASSERT_NE(ctx, nullptr);
    FolderConfig_createIoFolder(&folderConfig, ctx, nullptr, localId);
    ASSERT_NE(folderConfig, nullptr);

    BaseObject_releaseRef(localId);
    BaseObject_releaseRef(folderConfig);
    BaseObject_releaseRef(ctx);
}