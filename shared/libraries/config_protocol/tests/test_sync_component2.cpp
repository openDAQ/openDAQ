// ReSharper disable CppClangTidyModernizeAvoidBind
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_impl.h>
#include <opendaq/sync_component2.h>
#include <opendaq/sync_component2_impl.h>
#include <opendaq/sync_component2_internal_ptr.h>
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

// Simple test device with SyncComponent2
class TestDeviceWithSync2Impl : public Device
{
public:
    using Super = Device;

    TestDeviceWithSync2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
    {
        auto syncComponent = SyncComponent2(ctx, this->thisPtr<ComponentPtr>(), "sync");
        syncComponent.asPtr<IPropertyObjectInternal>().setLockingStrategy(LockingStrategy::ForwardOwnerLockOwn);
        this->addExistingComponent(syncComponent.detach());
    }

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
    {
        OPENDAQ_PARAM_NOT_NULL(obj);
        return daqTry([&obj, &serialized, &context, &factoryCallback]
        {
            *obj = Super::DeserializeComponent(
                serialized,
                context,
                factoryCallback,
                [](const SerializedObjectPtr&, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr&)
                {
                    return createWithImplementation<IDevice, TestDeviceWithSync2Impl>(
                        deserializeContext.getContext(),
                        deserializeContext.getParent(),
                        deserializeContext.getLocalId());
                }).detach();
        });
    }
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TestDeviceWithSync2Impl)

class ConfigSyncComponent2Test : public Test
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
            std::bind(&ConfigSyncComponent2Test::serverNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(serverDevice.getContext()));

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext,
            std::bind(&ConfigSyncComponent2Test::sendRequestAndGetReply, this, std::placeholders::_1),
            std::bind(&ConfigSyncComponent2Test::sendNoReplyRequest, this, std::placeholders::_1),
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

    SyncComponent2Ptr getServerSyncComponent() const
    {
        return serverDevice.findComponent("sync");
    }

    SyncComponent2Ptr getClientSyncComponent() const
    {
        return clientDevice.findComponent("sync");
    }

protected:
    ContextPtr serverContext;
    ContextPtr clientContext;
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
};

TEST_F(ConfigSyncComponent2Test, Connect)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    ASSERT_TRUE(serverSync.assigned());
    ASSERT_TRUE(clientSync.assigned());
}

TEST_F(ConfigSyncComponent2Test, GetInterfaces)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverInterfaces = serverSync.getInterfaces();
    auto clientInterfaces = clientSync.getInterfaces();

    ASSERT_EQ(serverInterfaces.getCount(), clientInterfaces.getCount());
    ASSERT_EQ(serverInterfaces.getKeyList(), clientInterfaces.getKeyList());
}

TEST_F(ConfigSyncComponent2Test, GetSelectedSource)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverSource = serverSync.getSelectedSource();
    auto clientSource = clientSync.getSelectedSource();

    ASSERT_EQ(serverSource.getName(), clientSource.getName());
}

TEST_F(ConfigSyncComponent2Test, SetSelectedSourceFromClient)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();
    auto serverSyncInternal = serverSync.asPtr<ISyncComponent2Internal>(true);

    // Add another interface on server
    auto newInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("TestInterface");
    serverSyncInternal.addInterface(newInterface);

    // Refresh client
    client.reset();
    client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
        clientContext,
        std::bind(&ConfigSyncComponent2Test::sendRequestAndGetReply, this, std::placeholders::_1),
        std::bind(&ConfigSyncComponent2Test::sendNoReplyRequest, this, std::placeholders::_1),
        nullptr,
        nullptr,
        nullptr);
    clientDevice = client->connect();
    clientSync = getClientSyncComponent();

    // Set selected source from client
    clientSync.setSelectedSource("TestInterface");

    // Verify server has the new selected source
    ASSERT_EQ(serverSync.getSelectedSource().getName(), "TestInterface");
}

TEST_F(ConfigSyncComponent2Test, GetSourceSynced)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    daq::Bool serverSynced, clientSynced;
    serverSync->getSourceSynced(&serverSynced);
    clientSync->getSourceSynced(&clientSynced);

    ASSERT_EQ(serverSynced, clientSynced);
}

TEST_F(ConfigSyncComponent2Test, GetSourceReferenceDomainId)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    daq::StringPtr serverDomainId, clientDomainId;
    serverSync->getSourceReferenceDomainId(&serverDomainId);
    clientSync->getSourceReferenceDomainId(&clientDomainId);

    ASSERT_EQ(serverDomainId, clientDomainId);
}

TEST_F(ConfigSyncComponent2Test, SyncInterfaceGetName)
{
    auto clientSync = getClientSyncComponent();
    auto clientSource = clientSync.getSelectedSource();

    ASSERT_EQ(clientSource.getName(), "InterfaceClockSync");
}

TEST_F(ConfigSyncComponent2Test, SyncInterfaceGetSynced)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverSource = serverSync.getSelectedSource();
    auto clientSource = clientSync.getSelectedSource();

    ASSERT_EQ(serverSource.getSynced(), clientSource.getSynced());
}

TEST_F(ConfigSyncComponent2Test, SyncInterfaceGetReferenceDomainId)
{
    auto serverSync = getServerSyncComponent();
    auto clientSync = getClientSyncComponent();

    auto serverSource = serverSync.getSelectedSource();
    auto clientSource = clientSync.getSelectedSource();

    ASSERT_EQ(serverSource.getReferenceDomainId(), clientSource.getReferenceDomainId());
}

TEST_F(ConfigSyncComponent2Test, SyncInterfacePropertyAccess)
{
    auto clientSync = getClientSyncComponent();
    auto clientSource = clientSync.getSelectedSource();
    auto propObj = clientSource.asPtr<IPropertyObject>(true);

    // Test reading properties via property object interface
    ASSERT_EQ(propObj.getPropertyValue("Name"), "InterfaceClockSync");
    ASSERT_NO_THROW(propObj.getPropertyValue("Mode"));
    ASSERT_NO_THROW(propObj.getPropertyValue("Status.Synchronized"));
    ASSERT_NO_THROW(propObj.getPropertyValue("Status.ReferenceDomainId"));
}

TEST_F(ConfigSyncComponent2Test, SetSelectedSourceNotFound)
{
    auto clientSync = getClientSyncComponent();

    ASSERT_THROW(clientSync.setSelectedSource("NonExistent"), NotFoundException);
}
