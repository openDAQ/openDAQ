#include <opendaq/input_port_factory.h>
#include <open62541/daqbsp_nodeids.h>
#include <open62541/daqbt_nodeids.h>
#include <opendaq/range_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_ptr.h>
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_server/objects/tms_server_input_port.h"
#include "opcuatms_server/objects/tms_server_signal.h"
#include "tms_object_test.h"
#include <opendaq/context_factory.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsSignalTest : public TmsObjectTest
{
public:
    SignalPtr createSignal(const ContextPtr& context, const StringPtr& localId = "sig")
    {
        SignalPtr signal = Signal(context, nullptr, localId);
        signal->setActive(false);
        return signal;
    }
};

TEST_F(TmsSignalTest, Create)
{
    SignalPtr signal = Signal(NullContext(), nullptr, "sig");
    auto tmsSignal = TmsServerSignal(signal, this->getServer(), NullContext());
}

TEST_F(TmsSignalTest, Register)
{
    SignalPtr signal = Signal(NullContext(), nullptr, "sig");
    auto serverSignal = TmsServerSignal(signal, this->getServer(), NullContext());
    auto nodeId = serverSignal.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
}

TEST_F(TmsSignalTest, DomainSignalReference)
{
    SignalPtr signal = createSignal(NullContext(), "signal");
    SignalPtr domainSignal = createSignal(NullContext(), "time signal");
    signal.asPtr<ISignalConfig>(true).setDomainSignal(domainSignal);

    auto serverSignal = TmsServerSignal(signal, this->getServer(), NullContext());
    auto signalNodeId = serverSignal.registerOpcUaNode();

    auto serverDomainSignal = TmsServerSignal(domainSignal, this->getServer(), NullContext());
    auto domainSignalNodeId = serverDomainSignal.registerOpcUaNode();

    ASSERT_NO_THROW(serverSignal.createNonhierarchicalReferences());

    ASSERT_TRUE(this->getClient()->nodeExists(signalNodeId));
    ASSERT_TRUE(this->getClient()->nodeExists(domainSignalNodeId));

    OpcUaServerNode signalNode(*this->getServer(), signalNodeId);
    auto hasDomainNodes = signalNode.browse(OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL));
    ASSERT_EQ(hasDomainNodes.size(), 1u);
    ASSERT_EQ(hasDomainNodes[0]->getNodeId(), domainSignalNodeId);
}
