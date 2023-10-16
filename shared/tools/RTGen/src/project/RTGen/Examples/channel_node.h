/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/stringobject.h>
#include "node.h"

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

enum class ChannelNodeType : EnumType
{
    Unknown,
    Channel,
    Folder
};

/*#
 * [decorated]
 * [templated(INode, IChannelNode)]
 * [propertyClass(NodeImpl, implTemplated: true)]
 * [propertyClassCtorArgs(const StringPtr& className, const StringPtr& nodeId)]
 * [interfaceSmartPtr(INode, NodePtr)]
 */
DECLARE_RT_INTERFACE(IChannelNode, INode)
{
    virtual ErrCode INTERFACE_FUNC getName(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC setName(IString* value) = 0;

    // [property(visible: false)]
    virtual ErrCode INTERFACE_FUNC getChannelNodeType(ChannelNodeType* kind) = 0;
};

END_NAMESPACE_DEWESOFT_RT_CORE
