#include "opcuatms_server/objects/tms_server_function_block.h"
#include "opcuatms_server/objects/tms_server_channel.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/daqdevice_nodeids.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerChannel::TmsServerChannel(const ChannelPtr& object, const OpcUaServerPtr& server, const ContextPtr& context)
    : Super(object, server, context)
{
}

OpcUaNodeId TmsServerChannel::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_CHANNELTYPE);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
