/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <vector>
#include <stack>
#include <map>

#include "opcuashared/node/opcuanodemethod.h"
#include "opcuashared/opcuanodecollection.h"
#include "opcuaclient/browser/opcuanodevisitor.h"
#include "opcuaclient/opcuanodefactory.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

struct OpcUaBrowserResults
{
    OpcUaNodeCollection nodes;
    void clear()
    {
        nodes.clear();
    }
};

class OpcUaNodeFactoryBrowser;
using OpcUaNodeFactoryBrowserPtr = std::shared_ptr<OpcUaNodeFactoryBrowser>;

class OpcUaNodeFactoryBrowser : public OpcUaNodeVisitor
{
public:
    OpcUaNodeFactoryBrowser(const IOpcUaNodeFactoryPtr& nodeFactory, const OpcUaClientPtr& client);
    ~OpcUaNodeFactoryBrowser();

    void browseTree();

    void browseTree(const OpcUaNodePtr& startNodeFolder);

    void browseTree(const OpcUaNodeId& startNodeId);

    void browse(const OpcUaNodePtr& startNodeFolder);

    void browse(const OpcUaNodeId& startNodeId);

    void browse(const OpcUaNodePtr& startNodeFolder, bool recursive, OpcUaNodeClass nodeClassMask);

    void browse(const OpcUaNodeId& startNodeId, bool recursive, OpcUaNodeClass nodeClassMask);

    void addToList(const OpcUaNodePtr& node);

    const OpcUaNodeCollection& getNodes() const;
    size_t getNodesSize();

    const IOpcUaNodeFactoryPtr& getNodeFactory();

protected:
    void traverse(const UA_ReferenceDescription& reference) override;
    void applyNode(const UA_ReferenceDescription& referenceDescription) override;

    OpcUaBrowserResults results;
    IOpcUaNodeFactoryPtr nodeFactory;

    OpcUaNodeId currentParent;
};

END_NAMESPACE_OPENDAQ_OPCUA
