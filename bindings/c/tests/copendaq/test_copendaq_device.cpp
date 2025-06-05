#include <copendaq.h>

#include <gtest/gtest.h>

#include "opendaq/context_factory.h"

class COpendaqDeviceTest : public testing::Test
{
    void SetUp() override
    {
        auto context = daq::NullContext();
        daqContext* ctx = (daqContext*) context.getObject();

        daqInstance_createInstance(&instance, ctx, nullptr);
        daqInstance_getRootDevice(instance, &dev);
    }

    void TearDown() override
    {
        daqBaseObject_releaseRef(instance);
        daqBaseObject_releaseRef(dev);
    }

protected:
    daqInstance* instance = nullptr;
    daqDevice* dev = nullptr;
};

TEST_F(COpendaqDeviceTest, AddressInfo)
{
    daqAddressInfoBuilder* builder = nullptr;
    daqAddressInfoBuilder_createAddressInfoBuilder(&builder);

    daqString* connectionString = nullptr;
    daqString_createString(&connectionString, "daqref://device0");
    daqAddressInfoBuilder_setConnectionString(builder, connectionString);
    daqAddressInfoBuilder_setReachabilityStatus(builder, daqAddressReachabilityStatusUnknown);
    daqString* type = nullptr;
    daqString_createString(&type, "Type");
    daqAddressInfoBuilder_setType(builder, type);
    daqBaseObject_releaseRef(type);
    daqString* address = nullptr;
    daqString_createString(&address, "Address");
    daqAddressInfoBuilder_setAddress(builder, connectionString);
    daqAddressInfoBuilder_setAddress(builder, address);
    daqBaseObject_releaseRef(address);
    daqBaseObject_releaseRef(connectionString);

    daqAddressInfo* addressInfo = nullptr;
    daqAddressInfoBuilder_build(builder, &addressInfo);
    ASSERT_NE(addressInfo, nullptr);

    daqString* connectionStringOut = nullptr;
    daqAddressInfo_getConnectionString(addressInfo, &connectionStringOut);
    ASSERT_NE(connectionStringOut, nullptr);

    daqBaseObject_releaseRef(connectionStringOut);
    daqBaseObject_releaseRef(addressInfo);
    daqBaseObject_releaseRef(builder);
}

TEST_F(COpendaqDeviceTest, DeviceInfo)
{
    daqDeviceInfo* deviceInfo = nullptr;
    daqDevice_getInfo(dev, &deviceInfo);
    ASSERT_NE(deviceInfo, nullptr);
    daqString* connectionString = nullptr;
    daqDeviceInfo_getConnectionString(deviceInfo, &connectionString);
    ASSERT_NE(connectionString, nullptr);
    daqConstCharPtr connectionStringStr = nullptr;
    daqString_getCharPtr(connectionString, &connectionStringStr);

    daqString* name = nullptr;
    daqDeviceInfo_getName(deviceInfo, &name);
    daqConstCharPtr nameStr = nullptr;
    daqString_getCharPtr(name, &nameStr);

    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(connectionString);
    daqBaseObject_releaseRef(deviceInfo);
}

TEST_F(COpendaqDeviceTest, IoFolderConfig)
{
    daqComponent* component = nullptr;
    daqBaseObject_borrowInterface(instance, DAQ_COMPONENT_INTF_ID, (daqBaseObject**) &component);
    ASSERT_NE(component, nullptr);

    daqFolderConfig* folderConfig = nullptr;
    daqString* localId = nullptr;
    daqString_createString(&localId, "IoFolder");
    daqContext* ctx = nullptr;
    daqComponent_getContext(component, &ctx);
    ASSERT_NE(ctx, nullptr);
    daqFolderConfig_createIoFolder(&folderConfig, ctx, nullptr, localId);
    ASSERT_NE(folderConfig, nullptr);

    daqBaseObject_releaseRef(localId);
    daqBaseObject_releaseRef(folderConfig);
    daqBaseObject_releaseRef(ctx);
}