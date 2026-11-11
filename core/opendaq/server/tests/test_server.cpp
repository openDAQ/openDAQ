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
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_type_factory.h>

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

class TypedServer final : public daq::Server
{
public:
    explicit TypedServer(const daq::ServerTypePtr& type)
        : daq::Server(type, nullptr, nullptr, daq::NullContext())
    {
    }
};

class NestingFb final : public daq::FunctionBlock
{
public:
    NestingFb(const daq::ContextPtr& ctx, const daq::ComponentPtr& parent, const daq::StringPtr& localId)
        : daq::FunctionBlock(daq::FunctionBlockType("fb", "fb", ""), ctx, parent, localId)
    {
    }

    daq::DictPtr<daq::IString, daq::IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        return daq::Dict<daq::IString, daq::IFunctionBlockType>({{"fb", daq::FunctionBlockType("fb", "fb", "")}});
    }

    daq::FunctionBlockPtr onAddFunctionBlock(const daq::StringPtr& /*typeId*/, const daq::PropertyObjectPtr& /*config*/) override
    {
        const auto fb = daq::createWithImplementation<daq::IFunctionBlock, NestingFb>(this->context, this->functionBlocks, "nested");
        addNestedFunctionBlock(fb);
        return fb;
    }
};

class FbServer final : public daq::Server
{
public:
    FbServer()
        : daq::Server("FbServer", nullptr, nullptr, daq::NullContext())
    {
    }

    daq::DictPtr<daq::IString, daq::IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        return daq::Dict<daq::IString, daq::IFunctionBlockType>({{"fb", daq::FunctionBlockType("fb", "fb", "")}});
    }

    daq::FunctionBlockPtr onAddFunctionBlock(const daq::StringPtr& /*typeId*/, const daq::PropertyObjectPtr& /*config*/) override
    {
        const auto fb = daq::createWithImplementation<daq::IFunctionBlock, NestingFb>(this->context, this->functionBlocks, "fb");
        addNestedFunctionBlock(fb);
        return fb;
    }
};

TEST_F(ServerTest, ServerTypeFromId)
{
    auto server = daq::createWithImplementation<daq::IServer, TestServer>("TestServerId");

    const auto type = server.getServerType();
    ASSERT_EQ(type.getId(), "TestServerId");
    ASSERT_EQ(type.getServerKind(), daq::ServerKind::Unspecified);
}

TEST_F(ServerTest, ServerTypeFromType)
{
    const auto type = daq::ServerTypeBuilder().setId("TypedServer").setServerKind(daq::ServerKind::Mirror).build().asPtr<daq::IServerType>();
    auto server = daq::createWithImplementation<daq::IServer, TypedServer>(type);

    ASSERT_EQ(server.getId(), "TypedServer");
    ASSERT_EQ(server.getLocalId(), "TypedServer");
    ASSERT_EQ(server.getServerType(), type);
}

TEST_F(ServerTest, SerializeAndDeserializeServerKind)
{
    const auto type = daq::ServerTypeBuilder().setId("TypedServer").setServerKind(daq::ServerKind::Streaming).build().asPtr<daq::IServerType>();
    const auto srv = daq::createWithImplementation<daq::IServer, TypedServer>(type);

    const auto serializer = daq::JsonSerializer(daq::True);
    srv.serialize(serializer);

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "TypedServer");
    const daq::ServerPtr newSrv = daq::JsonDeserializer().deserialize(serializer.getOutput(), deserializeContext, nullptr);

    ASSERT_EQ(newSrv.getServerType().getId(), "TypedServer");
    ASSERT_EQ(newSrv.getServerType().getServerKind(), daq::ServerKind::Streaming);
}

TEST_F(ServerTest, DeserializeWithoutComponentType)
{
    const auto type = daq::ServerTypeBuilder().setId("TypedServer").setServerKind(daq::ServerKind::Streaming).build().asPtr<daq::IServerType>();
    const auto srv = daq::createWithImplementation<daq::IServer, TypedServer>(type);

    const auto serializer = daq::JsonSerializer();
    srv.serialize(serializer);
    std::string json = serializer.getOutput();
    const auto keyPos = json.find("\"componentType\"");
    ASSERT_NE(keyPos, std::string::npos);
    json.replace(keyPos, std::string("\"componentType\"").size(), "\"unknownKey\"");

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "TypedServer");
    const daq::ServerPtr newSrv = daq::JsonDeserializer().deserialize(json, deserializeContext, nullptr);
    ASSERT_EQ(newSrv.getServerType().getId(), "TypedServer");
    ASSERT_EQ(newSrv.getServerType().getServerKind(), daq::ServerKind::Unspecified);
}

TEST_F(ServerTest, NoFunctionBlocksByDefault)
{
    auto server = daq::createWithImplementation<daq::IServer, TestServer>("TestServerId");

    ASSERT_EQ(server.getAvailableFunctionBlockTypes().getCount(), 0u);
    ASSERT_EQ(server.getFunctionBlocks().getCount(), 0u);
    ASSERT_THROW(server.addFunctionBlock("fb"), daq::NotSupportedException);
}

TEST_F(ServerTest, AddAndRemoveFunctionBlocks)
{
    auto server = daq::createWithImplementation<daq::IServer, FbServer>();
    ASSERT_TRUE(server.getAvailableFunctionBlockTypes().hasKey("fb"));

    const auto fb = server.addFunctionBlock("fb");
    ASSERT_EQ(fb.getParent(), server.getItem("FB"));
    ASSERT_EQ(server.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(server.getFunctionBlocks()[0], fb);

    const auto nested = fb.addFunctionBlock("fb");
    ASSERT_EQ(server.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(server.getFunctionBlocks(daq::search::Recursive(daq::search::Any())).getCount(), 2u);

    server.removeFunctionBlock(fb);
    ASSERT_TRUE(fb.isRemoved());
    ASSERT_TRUE(nested.isRemoved());
    ASSERT_EQ(server.getFunctionBlocks().getCount(), 0u);
}

TEST_F(ServerTest, RemovingServerRemovesFunctionBlocks)
{
    auto server = daq::createWithImplementation<daq::IServer, FbServer>();
    const auto fb = server.addFunctionBlock("fb");

    server.remove();

    ASSERT_TRUE(fb.isRemoved());
    ASSERT_THROW(server.getFunctionBlocks(), daq::ComponentRemovedException);
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

