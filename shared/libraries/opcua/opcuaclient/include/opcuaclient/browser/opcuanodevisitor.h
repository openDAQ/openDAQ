/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/*
    Builds tree of nodes of given server by using browse service
*/
#include <map>

#include <opcuaclient/browse_request.h>
#include "opcuaclient/browser/opcuabrowser.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuashared/node/opcuanodemethod.h"
#include "opcuashared/opcua.h"
#include "opcuashared/opcuanodeid.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaNodeVisitor
{
public:
    explicit OpcUaNodeVisitor(const OpcUaClientPtr& client, OpcUaNodeClass nodeClassMask = OpcUaNodeClass::All, bool recursive = true);
    virtual ~OpcUaNodeVisitor();

    OpcUaNodeClass getBrowseMask();
    void setBrowseMask(OpcUaNodeClass nodeClassMask);

    bool getRecursive();
    void setRecursive(bool recursive);
    void setRequestedMaxReferencesPerNode(uint32_t requestedMaxReferencesPerNode);

    const OpcUaClientPtr& getClient();
    virtual void traverse(const OpcUaNodeId& nodeId);

protected:
    virtual void traverse(const UA_ReferenceDescription& reference);
    void browseAndApplyNodes(const OpcUaNodeId& startNodeId, OpcUaBrowser& browser);
    virtual void applyNode(const UA_ReferenceDescription& referenceDescription);
    virtual void applyVariable(const UA_ReferenceDescription& referenceDescription);
    virtual void applyObject(const UA_ReferenceDescription& referenceDescription);
    virtual void applyMethod(const UA_ReferenceDescription& referenceDescription);

    OpcUaNodeId getOpcUaNodeId(const UA_ReferenceDescription& referenceDescription);

    OpcUaClientPtr client;
    uint32_t requestedMaxReferencesPerNode{0};

private:
    OpcUaNodeClass browseMask;
    bool recursive;
};

END_NAMESPACE_OPENDAQ_OPCUA
