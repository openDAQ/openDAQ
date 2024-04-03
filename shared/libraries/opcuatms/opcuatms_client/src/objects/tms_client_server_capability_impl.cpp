#include "opcuatms_client/objects/tms_client_server_capability_impl.h"
#include <opcuatms_client/objects/tms_client_property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

TmsClientServerCapabilityImpl::TmsClientServerCapabilityImpl(const ContextPtr& daqContext,
                                                             const StringPtr& protocolName,
                                                             const StringPtr& protocolId,
                                                             const TmsClientContextPtr& clientContext,
                                                             const opcua::OpcUaNodeId& nodeId)
    : TmsClientPropertyObjectBaseImpl(daqContext, protocolId, protocolName, clientContext, nodeId)
{

}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
