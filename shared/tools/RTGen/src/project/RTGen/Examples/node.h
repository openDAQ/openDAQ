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
#include <coretypes/common.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

enum class NodeType : EnumType
{
    Unknown,
    Data,
    Module,
    ChannelNode
};

/*#
 * [decorated]
 * [templated]
 * [propertyClass(implTemplated: true)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/generic_property_object_ptr.h>")]
 */
DECLARE_RT_INTERFACE(INode, IPropertyObject)
{
    // [property(visible: false)]
    virtual ErrCode INTERFACE_FUNC getNodeType(NodeType* kind) = 0;

    // [property(staticConfAction: Skip)]
    virtual ErrCode INTERFACE_FUNC getNodeId(IString** value) = 0;

    // [property(Proc)]    
    virtual ErrCode INTERFACE_FUNC getUniqueId(IString** value) = 0;

    // [templateType(parent, INode)]
    virtual ErrCode INTERFACE_FUNC getParent(INode** parent) = 0;
};

END_NAMESPACE_DEWESOFT_RT_CORE
