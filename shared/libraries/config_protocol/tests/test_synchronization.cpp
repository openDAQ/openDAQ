// ReSharper disable CppClangTidyModernizeAvoidBind
#include <testutils/testutils.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_impl.h>
#include <opendaq/synchronization.h>
#include <opendaq/synchronization_impl.h>
#include <opendaq/synchronization_internal_ptr.h>
#include <opendaq/sync_interface_base_impl.h>
#include <opendaq/sync_interface_ptr.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/user_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>

using namespace daq;
using namespace daq::config_protocol;
using namespace testing;
using namespace std::placeholders;

// Simple test device with Synchronization
class TestDeviceWithSync2Impl : public Device
{
public:
    using Super = Device;

    TestDeviceWithSync2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
        SynchronizationPtr sync;
        checkErrorInfo(this->getSynchronization(&sync));
    }

    class TestSyncInterface : public SyncInterfaceBaseImpl<>
    {
    public:
        using Super = SyncInterfaceBaseImpl<>;

        TestSyncInterface(const StringPtr& name, const std::vector<SyncMode> & availableModes = {})
            : Super(name, availableModes)
        {
        }
    };

    SynchronizationPtr onGetSynchronization() override
    {
        auto sync = Synchronization();
        const auto syncInterface = createWithImplementation<ISyncInterface, TestSyncInterface>("TestInterface");
        sync.asPtr<ISynchronizationInternal>(true).addInterface(syncInterface);
        return sync;
    }
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TestDeviceWithSync2Impl)

class ConfigSynchronizationTest : public Test
{
public:
    void SetUp() override
    {
        auto anonymousUser = User("", "");

        serverContext = NullContext();
        serverDevice = createWithImplementation<IDevice, TestDeviceWithSync2Impl>(serverContext, nullptr, "dev");
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        server = std::make_unique<ConfigProtocolServer>(
            serverDevice,
            std::bind(&ConfigSynchronizationTest::serverNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(serverDevice.getContext()));

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext,
            std::bind(&ConfigSynchronizationTest::sendRequestAndGetReply, this, std::placeholders::_1),
            std::bind(&ConfigSynchronizationTest::sendNoReplyRequest, this, std::placeholders::_1),
            nullptr,
            nullptr,
            nullptr);

        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    PacketBuffer sendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        return server->processRequestAndGetReply(requestPacket);
    }

    void sendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        assert(false);
        server->processNoReplyRequest(requestPacket);
    }

    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        client->triggerNotificationPacket(notificationPacket);
    }

    SynchronizationPtr getServerSyncComponent() const
    {
        return serverDevice.getSynchronization();
    }

    SynchronizationPtr getClientSyncComponent() const
    {
        return clientDevice.getSynchronization();
    }

protected:
    ContextPtr serverContext;
    ContextPtr clientContext;
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
};

TEST_F(ConfigSynchronizationTest, Connect)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    ASSERT_TRUE(serverSync.assigned());
    ASSERT_TRUE(clientSync.assigned());
}

TEST_F(ConfigSynchronizationTest, getSyncInterfaces)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverInterfaces = serverSync.getSyncInterfaces();
    auto clientInterfaces = clientSync.getSyncInterfaces();

    ASSERT_EQ(serverInterfaces.getCount(), clientInterfaces.getCount());
    ASSERT_EQ(serverInterfaces.getKeyList(), clientInterfaces.getKeyList());
}

TEST_F(ConfigSynchronizationTest, getSource)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverSource = serverSync.getSource();
    auto clientSource = clientSync.getSource();

    ASSERT_EQ(serverSource.getName(), clientSource.getName());
}

TEST_F(ConfigSynchronizationTest, setSourceFromClient)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    // Set selected source from client
    clientSync.getSyncInterfaces().get("TestInterface").asPtr<IPropertyObject>(true).setPropertyValue("Mode", "Input");

    // Verify server has the new selected source
    ASSERT_EQ(clientSync.getSource().getName(), "TestInterface");
    ASSERT_EQ(serverSync.getSource().getName(), "TestInterface");
}

TEST_F(ConfigSynchronizationTest, GetSourceReferenceDomainId)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    daq::ListPtr<IString> serverDomainId, clientDomainId;
    ASSERT_ERROR_CODE_EQ(serverSync->getReferenceDomainIds(&serverDomainId), OPENDAQ_SUCCESS);
    ASSERT_ERROR_CODE_EQ(clientSync->getReferenceDomainIds(&clientDomainId), OPENDAQ_SUCCESS);

    ASSERT_EQ(serverDomainId, clientDomainId);
}

TEST_F(ConfigSynchronizationTest, SyncInterfaceGetName)
{
    auto clientSync = getClientSyncComponent();
    auto clientSource = clientSync.getSource();

    ASSERT_EQ(clientSource.getName(), "ClockSyncInterface");
}

TEST_F(ConfigSynchronizationTest, SyncInterfaceGetReferenceDomainId)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverSource = serverSync.getSource();
    auto clientSource = clientSync.getSource();

    ASSERT_EQ(serverSource.getReferenceDomainId(), clientSource.getReferenceDomainId());
}

TEST_F(ConfigSynchronizationTest, SyncInterfacePropertyAccess)
{
    auto clientSync = getClientSyncComponent();
    auto clientSource = clientSync.getSource();
    auto propObj = clientSource.asPtr<IPropertyObject>(true);

    // Test reading properties via property object interface
    ASSERT_EQ(propObj.getPropertyValue("Name"), "ClockSyncInterface");
    ASSERT_NO_THROW(propObj.getPropertyValue("Mode"));
    ASSERT_NO_THROW(propObj.getPropertyValue("Status.Synchronized"));
    ASSERT_NO_THROW(propObj.getPropertyValue("Status.ReferenceDomainId"));
}

TEST_F(ConfigSynchronizationTest, SetSyncInterfaceModeViaProperty)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();
    clientSync.getSyncInterfaces().get("TestInterface").asPtr<IPropertyObject>(true).setPropertyValue("Mode", "Input");

    PropertyObjectPtr serverSource = serverSync.getSource();
    PropertyObjectPtr clientSource = clientSync.getSource();

    // Get initial mode
    auto initialMode = serverSource.getPropertyValue("Mode");
    ASSERT_EQ(initialMode, clientSource.getPropertyValue("Mode"));

    // Set mode on client
    clientSource.setPropertyValue("Mode", "Off");
    ASSERT_EQ(serverSource.getPropertyValue("Mode"), "Off");
    ASSERT_EQ(clientSource.getPropertyValue("Mode"), "Off");

    // Set mode on client
    clientSource.setPropertyValue("Mode", "Input");
    ASSERT_EQ(serverSource.getPropertyValue("Mode"), "Input");
    ASSERT_EQ(clientSource.getPropertyValue("Mode"), "Input");
}
