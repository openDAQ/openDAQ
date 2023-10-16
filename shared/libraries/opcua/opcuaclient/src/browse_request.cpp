#include "opcuaclient/browse_request.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

BrowseRequest::BrowseRequest(size_t nodesToBrowseSize)
    : OpcUaObject<UA_BrowseRequest>()
{
    resizeNodesToBrowse(nodesToBrowseSize);
}

BrowseRequest::BrowseRequest(const OpcUaNodeId& nodeId, OpcUaNodeClass nodeClassMask, const OpcUaNodeId& referenceTypeId, const OpcUaBrowseDirection browseDirection)
    : BrowseRequest(1u)
{
    value.nodesToBrowse[0].nodeId = nodeId.copyAndGetDetachedValue();
    value.nodesToBrowse[0].browseDirection = (UA_BrowseDirection) browseDirection;
    value.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    value.nodesToBrowse[0].referenceTypeId = referenceTypeId.copyAndGetDetachedValue();

    value.nodesToBrowse[0].nodeClassMask = (UA_UInt32) nodeClassMask;
    value.nodesToBrowse[0].includeSubtypes = UA_TRUE;
}

void BrowseRequest::resizeNodesToBrowse(size_t newNodesToBrowseSize)
{
    CheckStatusCodeException(UA_Array_resize(
        (void**) &value.nodesToBrowse, &value.nodesToBrowseSize, newNodesToBrowseSize, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION]));
}

END_NAMESPACE_OPENDAQ_OPCUA
