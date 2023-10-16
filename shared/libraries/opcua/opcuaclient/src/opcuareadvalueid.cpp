#include "opcuaclient/opcuareadvalueid.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaReadValueIdWithCallback::OpcUaReadValueIdWithCallback(const OpcUaNodeId& nodeId,
                                                           const ProcessFunctionType& processFunction,
                                                           AttributeIdType attributeId)
    : processFunction(processFunction)
{    
    getValue().nodeId = nodeId.copyAndGetDetachedValue();
    getValue().attributeId = attributeId;
}

END_NAMESPACE_OPENDAQ_OPCUA
