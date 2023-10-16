#include "opcuatms_client/objects/tms_client_channel_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsClientChannelImpl::TmsClientChannelImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const daq::opcua::tms::TmsClientContextPtr& clientContext,
    const OpcUaNodeId& nodeId
)
    : TmsClientFunctionBlockBaseImpl(context, parent, localId, clientContext, nodeId)
{

}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
