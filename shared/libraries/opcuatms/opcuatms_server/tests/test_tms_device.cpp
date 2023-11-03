#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/mock_physical_device.h>
#include <open62541/tmsdevice_nodeids.h>
#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/unit_factory.h>
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_server/objects/tms_server_device.h"
#include "test_helpers.h"
#include "tms_object_test.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

using TmsDeviceTest = TmsObjectTest;

TEST_F(TmsDeviceTest, Create)
{
    DevicePtr device = test_helpers::SetupInstance();
    auto tmsDevice = TmsServerDevice(device, this->getServer(), NullContext());
}

TEST_F(TmsDeviceTest, Register)
{
    DevicePtr device = test_helpers::SetupInstance();
    auto tmsDevice = TmsServerDevice(device, this->getServer(), NullContext());
    auto nodeId = tmsDevice.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
}

TEST_F(TmsDeviceTest, SubDevices)
{
    DevicePtr device = test_helpers::SetupInstance();
    auto tmsDevice = TmsServerDevice(device, this->getServer(), NullContext());
    auto nodeId = tmsDevice.registerOpcUaNode();

    ASSERT_EQ(test_helpers::BrowseSubDevices(client, nodeId).size(), 2u);
}

TEST_F(TmsDeviceTest, FunctionBlock)
{
    DevicePtr device = test_helpers::SetupInstance();
    auto tmsDevice = TmsServerDevice(device, this->getServer(), NullContext());
    auto nodeId = tmsDevice.registerOpcUaNode();

    auto functionBlockNodeId = getChildNodeId(nodeId, "FB");
    ASSERT_EQ(test_helpers::BrowseFunctionBlocks(client, functionBlockNodeId).size(), 1u);
}

TEST_F(TmsDeviceTest, Property)
{
    DevicePtr device = test_helpers::SetupInstance();

    const auto sampleRateProp =
        FloatPropertyBuilder("SampleRate", 100.0).setUnit(Unit("Hz")).setMinValue(1.0).setMaxValue(1000000.0).build();

    device.addProperty(sampleRateProp);

    auto tmsDevice = TmsServerDevice(device, this->getServer(), NullContext());
    auto nodeId = tmsDevice.registerOpcUaNode();

    auto sampleRateNodeId = this->getChildNodeId(nodeId, "SampleRate");
    ASSERT_FALSE(sampleRateNodeId.isNull());

    auto srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 100.0);

    device.setPropertyValue("SampleRate", 14.0);

    srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 14.0);

    this->getServer()->writeValue(sampleRateNodeId, OpcUaVariant(22.2));

    srValue = this->getServer()->readValue(sampleRateNodeId);
    ASSERT_TRUE(srValue.hasScalarType<UA_Double>());
    ASSERT_DOUBLE_EQ(srValue.readScalar<UA_Double>(), 22.2);
}

TEST_F(TmsDeviceTest, Components)
{
    DevicePtr device = test_helpers::SetupInstance();
    auto tmsDevice = TmsServerDevice(device, this->getServer(), NullContext());
    auto nodeId = tmsDevice.registerOpcUaNode();

    auto devices = test_helpers::BrowseSubDevices(client, nodeId);
    auto componentA = getChildNodeId(devices[1], "componentA");
    ASSERT_FALSE(componentA.isNull());
    auto componentA1 = getChildNodeId(componentA, "componentA1");
    ASSERT_FALSE(componentA1.isNull());
    auto componentB = getChildNodeId(devices[1], "componentB");
    ASSERT_FALSE(componentB.isNull());
}
