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
#include <opendaq/ptp_sync_interface_impl.h>
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

    class TestSyncInterface : public SyncInterfaceBaseImpl
    {
    public:
        using Super = SyncInterfaceBaseImpl;
        using Super::setSyncSourceStatus;
        using Super::setSyncRoleStatus;

        TestSyncInterface(const TypeManagerPtr& manager,
                          const StringPtr& name,
                          const std::vector<SyncMode> & availableModes = {SyncMode::Off, SyncMode::Input, SyncMode::Output, SyncMode::Auto})
            : Super(manager, name, availableModes)
        {
        }
    };

    class TestPtpSyncInterface : public PtpSyncInterfaceBaseImpl
    {
    public:
        using Super = PtpSyncInterfaceBaseImpl;

        explicit TestPtpSyncInterface(const TypeManagerPtr& manager)
            : Super(manager)
        {
        }

        using Super::createPortProporties;
    };

    SynchronizationPtr onGetSynchronization() override
    {
        const auto manager = this->context.getTypeManager();
        auto sync = Synchronization(manager);
        const auto syncInterface = createWithImplementation<ISyncInterface, TestSyncInterface>(manager, "TestInterface");

        // Simulate a driver that already knows its sync status before the device is added
        // to the component tree, so the client's initial connect state can be verified too.
        auto* impl = dynamic_cast<TestSyncInterface*>(syncInterface.getObject());
        impl->setSyncSourceStatus(SyncSourceStatus::Synced, "boot");

        sync.asPtr<ISynchronizationInternal>(true).addInterface(syncInterface);

        const auto ptpInterface = createWithImplementation<ISyncInterface, TestPtpSyncInterface>(manager);
        auto* ptpImpl = dynamic_cast<TestPtpSyncInterface*>(ptpInterface.getObject());
        ptpImpl->createPortProporties("eth0");
        sync.asPtr<ISynchronizationInternal>(true).addInterface(ptpInterface);

        return sync;
    }
};

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
    clientSync.setSource("TestInterface");

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
    ASSERT_NO_THROW(propObj.getPropertyValue("Configuration.Mode"));
    ASSERT_NO_THROW(propObj.getPropertyValue("Status.SynchronizationSourceStatus"));
    ASSERT_NO_THROW(propObj.getPropertyValue("Status.ReferenceDomainId"));
}

TEST_F(ConfigSynchronizationTest, SetSyncInterfaceModeViaProperty)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();
    clientSync.setSource("TestInterface");

    const auto serverSource = serverSync.getSource();
    const auto clientSource = clientSync.getSource();

    ASSERT_EQ(serverSource.getName(), clientSource.getName());

    // Get initial mode
    ASSERT_EQ(serverSource.getMode(), SyncMode::Auto);
    ASSERT_EQ(serverSource.getMode(), clientSource.getMode());

    // Set mode on client
    clientSource.setMode(SyncMode::Input);
    ASSERT_EQ(serverSource.getMode(), SyncMode::Input);
    ASSERT_EQ(serverSource.getMode(), clientSource.getMode());
}

TEST_F(ConfigSynchronizationTest, InitialStatusMatchesOnConnect)
{
    // TestInterface's status is set to "Synced" before the device is even created (simulating
    // a driver that already knows its status at boot), so a freshly connecting client should
    // see that status right away, not just on a later change.
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    const SyncInterfacePtr serverInterface = serverSync.getSyncInterfaces().get("TestInterface");
    const SyncInterfacePtr clientInterface = clientSync.getSyncInterfaces().get("TestInterface");

    const auto serverStatus = serverInterface.getStatusContainer().getStatus("SynchronizationSourceStatus");
    const auto clientStatus = clientInterface.getStatusContainer().getStatus("SynchronizationSourceStatus");

    ASSERT_EQ(serverStatus.getValue(), "Synced");
    ASSERT_EQ(clientStatus.getValue(), "Synced");
}

TEST_F(ConfigSynchronizationTest, StatusChangedPropagatesToClient)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    const SyncInterfacePtr serverInterface = serverSync.getSyncInterfaces().get("TestInterface");
    const SyncInterfacePtr clientInterface = clientSync.getSyncInterfaces().get("TestInterface");

    auto* impl = dynamic_cast<TestDeviceWithSync2Impl::TestSyncInterface*>(serverInterface.getObject());
    ASSERT_NE(impl, nullptr);

    impl->setSyncSourceStatus(SyncSourceStatus::Error, "cable unplugged");

    const auto serverStatusContainer = serverInterface.getStatusContainer();
    const auto clientStatusContainer = clientInterface.getStatusContainer();

    ASSERT_EQ(serverStatusContainer.getStatus("SynchronizationSourceStatus").getValue(), "Error");
    ASSERT_EQ(clientStatusContainer.getStatus("SynchronizationSourceStatus").getValue(), "Error");

    ASSERT_EQ(serverStatusContainer.getStatusMessage("SynchronizationSourceStatus"), "cable unplugged");
    ASSERT_EQ(clientStatusContainer.getStatusMessage("SynchronizationSourceStatus"), "cable unplugged");
}

TEST_F(ConfigSynchronizationTest, SaveLoadFromClient)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();
    const auto clientSyncUpdatable = clientSync.asPtr<IUpdatable>(true);

    ASSERT_EQ(clientSync.getSource().getName(), "ClockSyncInterface");
    ASSERT_EQ(serverSync.getSource().getName(), "ClockSyncInterface");

    auto serializer = JsonSerializer();
    ASSERT_ERROR_CODE_EQ(clientSyncUpdatable->serializeForUpdate(serializer), OPENDAQ_SUCCESS);

    // Change the source from the client, verify the server followed
    clientSync.setSource("TestInterface");
    ASSERT_EQ(clientSync.getSource().getName(), "TestInterface");
    ASSERT_EQ(serverSync.getSource().getName(), "TestInterface");

    // Restore from the client - the server should be driven back to its saved state too
    const auto deserializer = JsonDeserializer();
    deserializer.update(clientSyncUpdatable, serializer.getOutput(), nullptr);

    ASSERT_EQ(clientSync.getSource().getName(), "ClockSyncInterface");
    ASSERT_EQ(serverSync.getSource().getName(), "ClockSyncInterface");
}

TEST_F(ConfigSynchronizationTest, RoleStatusChangedPropagatesToClient)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    const SyncInterfacePtr serverInterface = serverSync.getSyncInterfaces().get("TestInterface");
    const SyncInterfacePtr clientInterface = clientSync.getSyncInterfaces().get("TestInterface");

    auto* impl = dynamic_cast<TestDeviceWithSync2Impl::TestSyncInterface*>(serverInterface.getObject());
    ASSERT_NE(impl, nullptr);

    impl->setSyncRoleStatus(SyncRoleStatus::Input, "locked as input");

    const auto serverStatusContainer = serverInterface.getStatusContainer();
    const auto clientStatusContainer = clientInterface.getStatusContainer();

    ASSERT_EQ(serverStatusContainer.getStatus("SynchronizationRoleStatus").getValue(), "Input");
    ASSERT_EQ(clientStatusContainer.getStatus("SynchronizationRoleStatus").getValue(), "Input");

    ASSERT_EQ(serverStatusContainer.getStatusMessage("SynchronizationRoleStatus"), "locked as input");
    ASSERT_EQ(clientStatusContainer.getStatusMessage("SynchronizationRoleStatus"), "locked as input");
}

TEST_F(ConfigSynchronizationTest, PtpInterfaceNestedPropertiesVisibleOnClient)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    const SyncInterfacePtr serverInterface = serverSync.getSyncInterfaces().get("PtpSyncInterface");
    const SyncInterfacePtr clientInterface = clientSync.getSyncInterfaces().get("PtpSyncInterface");

    const auto serverConfig = serverInterface.getConfiguration();
    const auto clientConfig = clientInterface.getConfiguration();

    ASSERT_EQ(serverConfig.getPropertyValue("PtpConfiguration.TransportProtocol"), clientConfig.getPropertyValue("PtpConfiguration.TransportProtocol"));
    ASSERT_EQ(clientConfig.getPropertyValue("PtpConfiguration.TransportProtocol"), "IEEE802_3");

    ASSERT_EQ(serverConfig.getPropertyValue("PortConfiguration.eth0.DelayMechanism"), clientConfig.getPropertyValue("PortConfiguration.eth0.DelayMechanism"));
    ASSERT_EQ(clientConfig.getPropertyValue("PortConfiguration.eth0.DelayMechanism"), "E2E");
}

TEST_F(ConfigSynchronizationTest, PtpInterfaceNestedPropertyChangeFromClientPropagatesToServer)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    const SyncInterfacePtr serverInterface = serverSync.getSyncInterfaces().get("PtpSyncInterface");
    const SyncInterfacePtr clientInterface = clientSync.getSyncInterfaces().get("PtpSyncInterface");

    const auto serverConfig = serverInterface.getConfiguration();
    const auto clientConfig = clientInterface.getConfiguration();

    clientConfig.setPropertyValue("PortConfiguration.eth0.DelayMechanism", "P2P");
    clientConfig.setPropertyValue("PtpConfiguration.TransportProtocol", "UDP_IPV4");

    ASSERT_EQ(serverConfig.getPropertyValue("PortConfiguration.eth0.DelayMechanism"), "P2P");
    ASSERT_EQ(serverConfig.getPropertyValue("PtpConfiguration.TransportProtocol"), "UDP_IPV4");
}
