/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <corestructure/channel_node.h>
#include <coretypes/listobject.h>
#include <coretypes/iterable.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

enum class NodeOrder : EnumType
{
    Unsorted,
    Name,
    Index
};

/*#
 * [decorated]
 * [templated(IChannelNode)]
 * [propertyClass(ChannelNodeImpl, implTemplated: true)]
 * [propertyClassCtorArgs(const StringPtr& className, const StringPtr& nodeId)]
 * [interfaceSmartPtr(IChannelNode, ChannelNodePtr)]
 * [addProperty(NodesMap, Dictionary, nullptr)]
 */
DECLARE_OPENDAQ_INTERFACE(IChannelFolder, IChannelNode)
{
    // [templateType(node, "")]
    virtual ErrCode INTERFACE_FUNC addNode(IChannelNode* node) = 0;

    // [property(None), templateType(value, IChannelNode)]
    virtual ErrCode INTERFACE_FUNC getNodes(NodeOrder nodeOrder, IIterable** value) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getNodeCount(SizeT* nodeCount) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC hasNode(IString* nodeId, Bool* hasKey) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getNode(IString* nodeId, IChannelNode** node) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC removeNode(IString* nodeId) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC clearNodes() = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ChannelFolder, IString*, nodeId)

END_NAMESPACE_DEWESOFT_RT_CORE
