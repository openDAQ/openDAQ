#include "test_helpers/test_helpers.h"
#include <opendaq/device_info_internal_ptr.h>
#include <coreobjects/authentication_provider_factory.h>
#include <chrono>
#include <thread>
#include <iostream>

using namespace daq;

class GatewayPropertySyncTest : public testing::Test
{
public:
    InstancePtr createOpcUAServer()
    {
        auto instance = InstanceBuilder()
                        .setDefaultRootDeviceLocalId("serverLocal")
                        .setRootDevice("daqref://device1")
                        .build();
        
        instance.addProperty(IntProperty("TestProperty", 100));

        instance.addServer("OpenDAQOPCUA", nullptr);

        return instance;
    }

    InstancePtr createGateway()
    {
        auto instance = InstanceBuilder()
                             .setDefaultRootDeviceLocalId("gatewayLocal")
                             .build();

        auto gatewayDevice = instance.addDevice("daq.opcua://127.0.0.1");
        instance.addServer("OpenDAQNativeStreaming", nullptr);
        return instance;
    }

    InstancePtr createClient()
    {
        auto instance = InstanceBuilder()
                           .setDefaultRootDeviceLocalId("clientLocal")
                           .build();

        auto clientDevice = instance.addDevice("daq.nd://127.0.0.1");
        return instance;
    }

    void SetUp() override
    {       
        serverInstance = createOpcUAServer();
        gatewayInstance = createGateway();
        clientInstance = createClient();
    }


protected:
    InstancePtr serverInstance;
    InstancePtr gatewayInstance;
    InstancePtr clientInstance;
};

TEST_F(GatewayPropertySyncTest, PropertyChangePropagatesFromServerToClient)
{
    // Get the ref device on server
    auto serverRefDevice = serverInstance.getRootDevice();
    ASSERT_TRUE(serverRefDevice.assigned());
    
    // Get the mirrored device on client (through gateway)
    auto clientMirroredDevice = clientInstance.getDevices()[0].getDevices()[0];
    ASSERT_TRUE(clientMirroredDevice.assigned());

    // Verify initial property value on server
    ASSERT_EQ(serverRefDevice.getPropertyValue("TestProperty"), 100);
    
    // Verify initial property value on client
    ASSERT_EQ(clientMirroredDevice.getPropertyValue("TestProperty"), 100);

    // Change property value on server
    serverRefDevice.setPropertyValue("TestProperty", 1);

    // Verify property value changed on server
    ASSERT_EQ(serverRefDevice.getPropertyValue("TestProperty"), 1);
    
    // Verify property value changed on client
    ASSERT_EQ(clientMirroredDevice.getPropertyValue("TestProperty"), 1);
}

TEST_F(GatewayPropertySyncTest, MultiplePropertyChangesPropagate)
{
    // Get the ref device on server
    auto serverRefDevice = serverInstance.getRootDevice();
    ASSERT_TRUE(serverRefDevice.assigned());
    
    // Get the mirrored device on client
    auto clientMirroredDevice = clientInstance.getDevices()[0].getDevices()[0];
    ASSERT_TRUE(clientMirroredDevice.assigned());

    // Change property multiple times
    for (int i = 200; i <= 250; i += 10)
    {
        serverRefDevice.setPropertyValue("TestProperty", i);
        
        ASSERT_EQ(serverRefDevice.getPropertyValue("TestProperty"), i);
        ASSERT_EQ(clientMirroredDevice.getPropertyValue("TestProperty"), i);
    }
}

