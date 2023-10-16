#include <opendaq/range_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/input_port_ptr.h>
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms/exceptions.h"
#include "opcuatms_client/objects/tms_client_input_port_factory.h"
#include "opcuatms_client/objects/tms_client_signal_factory.h"
#include "opcuatms_server/objects/tms_server_input_port.h"
#include "opcuatms_server/objects/tms_server_signal.h"
#include "tms_object_integration_test.h"
#include <opendaq/signal_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/context_factory.h>
#include <open62541/tmsbsp_nodeids.h>
#include <test_input_port_notifications.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsInputPortTest : public TmsObjectIntegrationTest
{
public:
    InputPortPtr createInputPort(std::string name, Bool requiresSignal)
    {
        auto ip = InputPort(NullContext(), nullptr, name);
        ip.setRequiresSignal(requiresSignal);
        ip.getTags().add("port");
        return ip;
    }
};

TEST_F(TmsInputPortTest, Create)
{
    InputPortPtr inputPort = createInputPort("The Name", false);
    auto tmsInputPort = TmsServerInputPort(inputPort, this->getServer(), NullContext());
}

TEST_F(TmsInputPortTest, Register)
{
    const std::string name{"Some Name"};
    InputPortPtr daqServerInputPort = createInputPort(name, false);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);
    ASSERT_TRUE(clientInputPort.assigned());
}

TEST_F(TmsInputPortTest, BrowseName)
{
    const std::string name{"Some Name"};
    InputPortPtr daqServerInputPort = createInputPort(name, false);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    auto browseName = client->readBrowseName(nodeId);
    ASSERT_EQ(browseName, name);
}

TEST_F(TmsInputPortTest, AttrName)
{
    const std::string name{"Test Name"};
    InputPortPtr daqServerInputPort = createInputPort(name, false);
    ASSERT_FALSE(daqServerInputPort.getRequiresSignal());
    ASSERT_EQ(daqServerInputPort.getLocalId(), name);

    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    auto browseName = clientContext->getClient()->readBrowseName(nodeId);
    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, browseName, clientContext, nodeId);

    ASSERT_EQ(daqServerInputPort.getLocalId(), clientInputPort.getLocalId());
    ASSERT_EQ(clientInputPort.getLocalId(), name);
}

TEST_F(TmsInputPortTest, AttrRequiresSignalFalse)
{
    InputPortPtr daqServerInputPort = createInputPort("The Name", false);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);

    ASSERT_FALSE(IsTrue(daqServerInputPort.getRequiresSignal()));
    ASSERT_FALSE(IsTrue(clientInputPort.getRequiresSignal()));
}

TEST_F(TmsInputPortTest, AttrRequiresSignalTrue)
{
    InputPortPtr daqServerInputPort = createInputPort("The Name", true);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);

    ASSERT_TRUE(IsTrue(daqServerInputPort.getRequiresSignal()));
    ASSERT_TRUE(IsTrue(clientInputPort.getRequiresSignal()));
}

TEST_F(TmsInputPortTest, MethodAcceptsSignal)
{
    SignalPtr signal = Signal(NullContext(), nullptr, "sig");
    InputPortPtr daqServerInputPort = createInputPort("The Name", true);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);

    //TODO: More testing when the server in fact really checks the signal if the signal is ok
    EXPECT_THROW(clientInputPort.acceptsSignal(signal), daq::opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(TmsInputPortTest, MethodConnectSignal)
{
    SignalPtr signal = Signal(NullContext(), nullptr, "sig");

    InputPortPtr daqServerInputPort = createInputPort("The Name", true);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);

    //TODO: More testing when the server in fact really can connect the signal
    EXPECT_THROW(clientInputPort.connect(signal), daq::opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(TmsInputPortTest, MethodDisconnectSignal)
{
    InputPortPtr daqServerInputPort = createInputPort("The Name", true);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);

    //TODO: More testing when the server in fact really can disconnect the signal
    EXPECT_THROW(clientInputPort.disconnect(), daq::opcua::OpcUaClientCallNotAvailableException);
    //auto status = clientInputPort->disconnect();
    //ASSERT_EQ(status, OPENDAQ_SUCCESS); //TODO: Should fail when implemented on the server as nothing is connected

    //SignalPtr signal = Signal(nullptr);
    //status = clientInputPort->connect(signal);
    //ASSERT_EQ(status, OPENDAQ_SUCCESS);

    //status = clientInputPort->disconnect();
    //ASSERT_EQ(status, OPENDAQ_SUCCESS);

    //status = clientInputPort->disconnect();
    //ASSERT_EQ(status, OPENDAQ_SUCCESS); //TODO: Should fail when implemented on the server as nothing is connected
}

TEST_F(TmsInputPortTest, ConnectedToReference)
{
    const auto logger = Logger();
    const auto scheduler = Scheduler(logger);
    const auto context = Context(scheduler, logger, nullptr, nullptr);

    SignalPtr signal = Signal(context, nullptr, "sig");

    auto serverSignal = TmsServerSignal(signal, this->getServer(), NullContext());
    auto signalNodeId = serverSignal.registerOpcUaNode();

    InputPortNotificationsPtr inputPortNotification = TestInputPortNotifications();

    InputPortConfigPtr inputPort = InputPort(NullContext(), nullptr, "inputPort");
    inputPort.setListener(inputPortNotification);
    inputPort.connect(signal);

    auto serverInputPort = TmsServerInputPort(inputPort, this->getServer(), NullContext());
    auto inputPortNodeId = serverInputPort.registerOpcUaNode();

    ASSERT_NO_THROW(serverInputPort.createNonhierarchicalReferences());

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, inputPortNodeId);
    SignalPtr clientSignal = TmsClientSignal(context, nullptr, "sig", clientContext, signalNodeId);

    auto connectedSignal = clientInputPort.getSignal();
    ASSERT_TRUE(connectedSignal.assigned());
    ASSERT_EQ(connectedSignal, clientSignal);
}

TEST_F(TmsInputPortTest, ComponentMethods)
{
    const std::string name{"inputPort"};
    InputPortPtr daqServerInputPort = createInputPort(name, false);
    auto serverInputPort = TmsServerInputPort(daqServerInputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    InputPortPtr clientInputPort = TmsClientInputPort(NullContext(), nullptr, "inputPort", clientContext, nodeId);

    ASSERT_EQ(daqServerInputPort.getName(), clientInputPort.getName());

    auto tags = daqServerInputPort.getTags();
    auto clientTags = clientInputPort.getTags();

    ASSERT_TRUE(tags.query("port"));
    ASSERT_TRUE(clientTags.query("port"));

    clientInputPort.setActive(false);
    ASSERT_EQ(daqServerInputPort.getActive(), clientInputPort.getActive());

    clientInputPort.setActive(true);
    ASSERT_EQ(daqServerInputPort.getActive(), clientInputPort.getActive());
}
