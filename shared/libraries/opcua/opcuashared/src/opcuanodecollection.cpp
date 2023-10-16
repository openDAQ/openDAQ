#include "opcuashared/opcuanodecollection.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/* OpcUaNodeCollection */

OpcUaNodeCollection OpcUaNodeCollection::selectNodes(OpcUaNodeClass nodeClassMask)
{
    OpcUaNodeCollection rtn;

    for (const auto& item : *this)
    {
        auto nc = static_cast<UA_UInt32>(item->getNodeClass());
        if ((nc & (UA_UInt32)nodeClassMask) > 0)
            rtn.push_back(item);
    }

    return rtn;
}

OpcUaNodePtr OpcUaNodeCollection::locateNode(const OpcUaNodeId& nodeId) const
{
    auto it = std::find_if(cbegin(), cend(), [&nodeId](const OpcUaNodePtr& node) { return node->getNodeId() == nodeId; });
    if (it != cend())
        return *it;
    return nullptr;
}

END_NAMESPACE_OPENDAQ_OPCUA
