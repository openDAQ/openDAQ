#include <opendaq/server_impl.h>
#include <opendaq/server_ptr.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/component_exceptions.h>

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

