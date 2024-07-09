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
#include <coretypes/common.h>
#include <coreobjects/property_object.h>
#include "tags.h"

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [decorated]
 * [templated(defaultAliasName: NodePtr)]
 * [propertyClass(implTemplated: true)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/generic_property_object_ptr.h>")]
 * [interfaceSmartPtr(INode, GenericNodePtr)]
 * [addProperty(NodesMap, Dictionary, nullptr)]
 * [addProperty(Active, Bool, True)]
 * [libraryInterfaces(CoreTypes, IIterable)]
 */
DECLARE_OPENDAQ_INTERFACE(INode, IPropertyObject)
{
    // [property(staticConfAction: Skip)]
    virtual ErrCode INTERFACE_FUNC getNodeId(IString** value) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getUniqueId(IString** value) = 0;

    // [templateType(parent, INode)]
    virtual ErrCode INTERFACE_FUNC getParent(INode** parent) = 0;

    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    // [templateType(node, "")]
    virtual ErrCode INTERFACE_FUNC addNode(INode* node) = 0;

    // [property(None), elementType(value, INode)]
    virtual ErrCode INTERFACE_FUNC getNodes(IIterable** nodes) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getNodeCount(SizeT* nodeCount) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC hasNode(IString* nodeId, Bool* hasKey) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC getNode(IString* nodeId, INode** node) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC removeNode(IString* nodeId) = 0;

    // [property(Proc)]
    virtual ErrCode INTERFACE_FUNC clearNodes() = 0;

    virtual ErrCode INTERFACE_FUNC getTags(ITags** tags) = 0;

    // [elementType(nodes, INode)]
    virtual ErrCode INTERFACE_FUNC findNodesByTags(IString* tagEval, IList** nodes) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, Node, INode, IString*, nodeId)

END_NAMESPACE_OPENDAQ
