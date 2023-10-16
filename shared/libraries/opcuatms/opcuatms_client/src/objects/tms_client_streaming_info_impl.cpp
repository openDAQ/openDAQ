#include "opcuatms_client/objects/tms_client_streaming_info_impl.h"
#include <opcuatms_client/objects/tms_client_property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

TmsClientStreamingInfoImpl::TmsClientStreamingInfoImpl(const ContextPtr& daqContext,
                                                       const StringPtr& protocolId,
                                                       const TmsClientContextPtr& clientContext,
                                                       const opcua::OpcUaNodeId& nodeId)
    : TmsClientPropertyObjectBaseImpl(daqContext, protocolId, clientContext, nodeId)
{
}
END_NAMESPACE_OPENDAQ_OPCUA_TMS
