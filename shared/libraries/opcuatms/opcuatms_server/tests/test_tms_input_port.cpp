#include <opendaq/range_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/input_port_ptr.h>
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_server/objects/tms_server_input_port.h"
#include "opcuatms_server/objects/tms_server_signal.h"
#include "tms_object_test.h"
#include <opendaq/scheduler_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_factory.h>
#include <open62541/daqbsp_nodeids.h>
#include <test_input_port_notifications.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsInputPortTest : public TmsObjectTest
{
public:
    InputPortPtr createInputPort()
    {
        auto port = InputPort(NullContext(), nullptr, "port");
        port.getTags().add("port");
        return port;
    }
};

TEST_F(TmsInputPortTest, Create)
{
    InputPortPtr inputPort = createInputPort();
    auto tmsInputPort = TmsServerInputPort(inputPort, this->getServer(), NullContext());
}

TEST_F(TmsInputPortTest, Register)
{
    InputPortPtr inputPort = createInputPort();
    auto serverInputPort = TmsServerInputPort(inputPort, this->getServer(), NullContext());
    auto nodeId = serverInputPort.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
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

    InputPortConfigPtr inputPort = InputPort(context, nullptr, "TestInputPort");
    inputPort.setListener(inputPortNotification);
    inputPort.connect(signal);

    auto serverInputPort = TmsServerInputPort(inputPort, this->getServer(), NullContext());
    auto inputPortNodeId = serverInputPort.registerOpcUaNode();

    ASSERT_NO_THROW(serverInputPort.createNonhierarchicalReferences());

    ASSERT_TRUE(this->getClient()->nodeExists(signalNodeId));
    ASSERT_TRUE(this->getClient()->nodeExists(inputPortNodeId));

    OpcUaServerNode inputPortNode(*this->getServer(), inputPortNodeId);
    auto connectedToNodes = inputPortNode.browse(OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_CONNECTEDTOSIGNAL));
    ASSERT_EQ(connectedToNodes.size(), 1u);
    ASSERT_EQ(connectedToNodes[0]->getNodeId(), signalNodeId);
}
