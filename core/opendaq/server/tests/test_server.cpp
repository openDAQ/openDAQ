#include <opendaq/server_impl.h>
#include <opendaq/server_ptr.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/component_exceptions.h>
#include <opendaq/discovery_server_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_manager_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>

using ServerTest = testing::Test;

class TestServer : public daq::Server
{
public:
    TestServer(const daq::StringPtr& id)
        : daq::Server(id, nullptr, nullptr, daq::NullContext())
    {
    }
};

TEST_F(ServerTest, Id)
{
    auto server = daq::createWithImplementation<daq::IServer, TestServer>("TestServerId");

    ASSERT_EQ(server.getId(), "TestServerId");
}

TEST_F(ServerTest, Signals)
{
    auto server = daq::createWithImplementation<daq::IServer, TestServer>("TestServerId");

    ASSERT_EQ(server.getSignals().getElementInterfaceId(), daq::ISignal::Id);
}

TEST_F(ServerTest, Remove)
{
    auto server = daq::createWithImplementation<daq::IServer, TestServer>("TestServerId");

    ASSERT_NO_THROW(server.remove());
    ASSERT_TRUE(server.isRemoved());

    ASSERT_THROW(server.getSignals(), daq::ComponentRemovedException);
}

class MockServer final : public daq::Server
{
public:
    MockServer(const daq::StringPtr& id, const daq::ContextPtr& ctx)
        : daq::Server(id, nullptr, nullptr, ctx)
    {
        createAndAddSignal("sig_server");
    }
};

TEST_F(ServerTest, SerializeAndDeserialize)
{
    const auto srv = daq::createWithImplementation<daq::IServer, MockServer>("MockServerId", daq::NullContext());

    const auto serializer = daq::JsonSerializer(daq::True);
    srv.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "srv");

    const daq::ServerPtr newSrv = deserializer.deserialize(str1, deserializeContext, nullptr);

    const auto serializer2 = daq::JsonSerializer(daq::True);
    newSrv.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);

    ASSERT_EQ(newSrv.getSignals().getElementInterfaceId(), daq::ISignal::Id);
}

TEST_F(ServerTest, BeginUpdateEndUpdate)
{
    const auto srv = daq::createWithImplementation<daq::IServer, MockServer>("MockServerId", daq::NullContext());
    srv.addProperty(daq::StringPropertyBuilder("SrvProp", "-").build());

    const auto sig = srv.getSignals()[0];
    sig.addProperty(daq::StringPropertyBuilder("SigProp", "-").build());

    srv.beginUpdate();

    srv.setPropertyValue("SrvProp", "s");
    ASSERT_EQ(srv.getPropertyValue("SrvProp"), "-");

    sig.setPropertyValue("SigProp", "cs");
    ASSERT_EQ(sig.getPropertyValue("SigProp"), "-");

    srv.endUpdate();

    ASSERT_EQ(srv.getPropertyValue("SrvProp"), "s");
    ASSERT_EQ(sig.getPropertyValue("SigProp"), "cs");
}

class RecordingDiscoveryServer final : public daq::ImplementationOf<daq::IDiscoveryServer>
{
public:
    daq::ErrCode INTERFACE_FUNC registerService(daq::IString* id, daq::IPropertyObject* config, daq::IDeviceInfo* deviceInfo) override
    {
        registeredIds.emplace_back(daq::StringPtr::Borrow(id).toStdString());
        registeredConfigs.emplace_back(daq::PropertyObjectPtr(config));
        registeredDeviceInfoAssigned.push_back(deviceInfo != nullptr);
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC unregisterService(daq::IString* id) override
    {
        unregisteredIds.emplace_back(daq::StringPtr::Borrow(id).toStdString());
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC setRootDevice(daq::IDevice* /*device*/) override
    {
        return OPENDAQ_SUCCESS;
    }

    std::vector<std::string> registeredIds;
    std::vector<daq::PropertyObjectPtr> registeredConfigs;
    std::vector<bool> registeredDeviceInfoAssigned;
    std::vector<std::string> unregisteredIds;
};

// Server advertising two discovery services under a single server id.
class MultiServiceServer final : public daq::Server
{
public:
    MultiServiceServer(const daq::StringPtr& id, const daq::DevicePtr& rootDevice, const daq::ContextPtr& ctx)
        : daq::Server(id, nullptr, rootDevice, ctx)
    {
    }

protected:
    daq::ListPtr<daq::IPropertyObject> getDiscoveryConfigs() override
    {
        return daq::List<daq::IPropertyObject>(makeConfig("_first._tcp.local.", 1234), makeConfig("_second._tcp.local.", 5678));
    }

private:
    static daq::PropertyObjectPtr makeConfig(const daq::StringPtr& serviceName, daq::Int port)
    {
        auto config = daq::PropertyObject();
        config.addProperty(daq::StringProperty("ServiceName", serviceName));
        config.addProperty(daq::IntProperty("Port", port));
        return config;
    }
};

class SingleServiceServer final : public daq::Server
{
public:
    SingleServiceServer(const daq::StringPtr& id, const daq::DevicePtr& rootDevice, const daq::ContextPtr& ctx)
        : daq::Server(id, nullptr, rootDevice, ctx)
    {
    }

protected:
    daq::PropertyObjectPtr getDiscoveryConfig() override
    {
        auto config = daq::PropertyObject();
        config.addProperty(daq::StringProperty("ServiceName", "_only._tcp.local."));
        return config;
    }
};

class ServerDiscoveryTest : public testing::Test
{
protected:
    daq::ContextPtr createContext(const daq::DictPtr<daq::IString, daq::IDiscoveryServer>& discoveryServers)
    {
        return daq::Context(nullptr,
                            daq::Logger(),
                            daq::TypeManager(),
                            daq::ModuleManager("[[none]]"),
                            nullptr,
                            daq::Dict<daq::IString, daq::IBaseObject>(),
                            discoveryServers);
    }
};

TEST_F(ServerDiscoveryTest, EnableDiscoveryRegistersEveryConfig)
{
    auto* fakeImpl = new RecordingDiscoveryServer();
    const daq::DiscoveryServerPtr fake = fakeImpl;

    const auto ctx = createContext(daq::Dict<daq::IString, daq::IDiscoveryServer>({{"fake", fake}}));
    const auto instance = daq::InstanceBuilder().setContext(ctx).build();

    const auto srv = daq::createWithImplementation<daq::IServer, MultiServiceServer>("MultiServiceServerId",
                                                                                    instance.getRootDevice(),
                                                                                    ctx);
    ASSERT_NO_THROW(srv.enableDiscovery());

    ASSERT_EQ(fakeImpl->registeredIds.size(), 2u);
    ASSERT_EQ(fakeImpl->registeredIds[0], "MultiServiceServerId");
    ASSERT_EQ(fakeImpl->registeredIds[1], "MultiServiceServerId");

    ASSERT_EQ(fakeImpl->registeredConfigs[0].getPropertyValue("ServiceName"), "_first._tcp.local.");
    ASSERT_EQ(fakeImpl->registeredConfigs[1].getPropertyValue("ServiceName"), "_second._tcp.local.");
    ASSERT_EQ(fakeImpl->registeredConfigs[0].getPropertyValue("Port"), 1234);
    ASSERT_EQ(fakeImpl->registeredConfigs[1].getPropertyValue("Port"), 5678);

    // the root device info must be forwarded to the discovery server
    ASSERT_TRUE(fakeImpl->registeredDeviceInfoAssigned[0]);
    ASSERT_TRUE(fakeImpl->registeredDeviceInfoAssigned[1]);
}

TEST_F(ServerDiscoveryTest, EnableDiscoverySingleConfigBackCompat)
{
    auto* fakeImpl = new RecordingDiscoveryServer();
    const daq::DiscoveryServerPtr fake = fakeImpl;

    const auto ctx = createContext(daq::Dict<daq::IString, daq::IDiscoveryServer>({{"fake", fake}}));
    const auto instance = daq::InstanceBuilder().setContext(ctx).build();

    const auto srv = daq::createWithImplementation<daq::IServer, SingleServiceServer>("SingleServiceServerId",
                                                                                     instance.getRootDevice(),
                                                                                     ctx);
    ASSERT_NO_THROW(srv.enableDiscovery());

    ASSERT_EQ(fakeImpl->registeredIds.size(), 1u);
    ASSERT_EQ(fakeImpl->registeredIds[0], "SingleServiceServerId");
    ASSERT_EQ(fakeImpl->registeredConfigs[0].getPropertyValue("ServiceName"), "_only._tcp.local.");
}

TEST_F(ServerDiscoveryTest, EnableDiscoveryAcrossMultipleDiscoveryServers)
{
    auto* firstImpl = new RecordingDiscoveryServer();
    auto* secondImpl = new RecordingDiscoveryServer();
    const daq::DiscoveryServerPtr first = firstImpl;
    const daq::DiscoveryServerPtr second = secondImpl;

    const auto ctx = createContext(daq::Dict<daq::IString, daq::IDiscoveryServer>({{"first", first}, {"second", second}}));
    const auto instance = daq::InstanceBuilder().setContext(ctx).build();

    const auto srv = daq::createWithImplementation<daq::IServer, MultiServiceServer>("MultiServiceServerId",
                                                                                    instance.getRootDevice(),
                                                                                    ctx);
    ASSERT_NO_THROW(srv.enableDiscovery());

    ASSERT_EQ(firstImpl->registeredIds.size(), 2u);
    ASSERT_EQ(secondImpl->registeredIds.size(), 2u);
}

TEST_F(ServerDiscoveryTest, DisableDiscoveryUnregistersOnceById)
{
    auto* fakeImpl = new RecordingDiscoveryServer();
    const daq::DiscoveryServerPtr fake = fakeImpl;

    const auto ctx = createContext(daq::Dict<daq::IString, daq::IDiscoveryServer>({{"fake", fake}}));
    const auto instance = daq::InstanceBuilder().setContext(ctx).build();

    const auto srv = daq::createWithImplementation<daq::IServer, MultiServiceServer>("MultiServiceServerId",
                                                                                    instance.getRootDevice(),
                                                                                    ctx);
    srv.enableDiscovery();
    ASSERT_EQ(fakeImpl->registeredIds.size(), 2u);

    ASSERT_NO_THROW(srv.disableDiscovery());

    ASSERT_EQ(fakeImpl->unregisteredIds.size(), 1u);
    ASSERT_EQ(fakeImpl->unregisteredIds[0], "MultiServiceServerId");
}

TEST_F(ServerDiscoveryTest, StopUnregistersDiscovery)
{
    auto* fakeImpl = new RecordingDiscoveryServer();
    const daq::DiscoveryServerPtr fake = fakeImpl;

    const auto ctx = createContext(daq::Dict<daq::IString, daq::IDiscoveryServer>({{"fake", fake}}));
    const auto instance = daq::InstanceBuilder().setContext(ctx).build();

    const auto srv = daq::createWithImplementation<daq::IServer, MultiServiceServer>("MultiServiceServerId",
                                                                                    instance.getRootDevice(),
                                                                                    ctx);
    srv.enableDiscovery();
    ASSERT_NO_THROW(srv.stop());

    ASSERT_EQ(fakeImpl->unregisteredIds.size(), 1u);
    ASSERT_EQ(fakeImpl->unregisteredIds[0], "MultiServiceServerId");
}

