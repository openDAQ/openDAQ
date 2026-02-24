/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/setup_node.h>
#include <rapidjson/document.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/setup_node_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class SetupNodeImpl : public ImplementationOf<ISetupNode>
{
public:
    SetupNodeImpl() = default;
    SetupNodeImpl(const StringPtr& setupString);

    ErrCode INTERFACE_FUNC getLocalId(IString** id) override;
    ErrCode INTERFACE_FUNC getType(NodeType* type) override;
    ErrCode INTERFACE_FUNC getOptions(IPropertyObject** options) override;
    ErrCode INTERFACE_FUNC getChildNodes(IList** childNodes) override;
    ErrCode INTERFACE_FUNC getChildNodesWithType(IList** childNodes, NodeType type, Bool recursive) override;
    ErrCode INTERFACE_FUNC getParent(ISetupNode** parent) override;

    static PropertyObjectPtr createOptions(const rapidjson::Value& value, NodeType nt);
    static NodeType getNodeType(const rapidjson::Value& value);
    bool read(const StringPtr& localId, const rapidjson::Value& document, const SetupNodePtr& parent, NodeType nodeType);

private:
    NodeType nodeType;
    PropertyObjectPtr options;
    ListPtr<ISetupNode> children;
    SetupNodePtr parent;
    StringPtr localId;

};


END_NAMESPACE_OPENDAQ
