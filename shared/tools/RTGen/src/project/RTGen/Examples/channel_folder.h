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
#include "coretypes/coretypes.h"
#include "channel_node.h"

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

static const IntfID ChannelFolderGuid = { 0x08E22C01, 0x35FE, 0x5335, { 0xA7, 0x68, 0x0C, 0x2D, 0xAC, 0xF8, 0x4A, 0x7A } };

/*#
 * [decorated]
 * [propertyClass]
 * [templated(IChannelNode)]
 * [interfaceSmartPtr(IChannelNode, ChannelNodePtr)]
 * [interfaceNamespace(IChannelFolder, "Dewesoft::RT::Core")]
 */
DECLARE_RT_INTERFACE(IChannelFolder, IChannelNode)
{
    DEFINE_INTFID(ChannelFolderGuid)

    // [templateType(node, "")]
    virtual ErrCode INTERFACE_FUNC addNode(IChannelNode* node = nullptr) = 0;

    // [elementType(value, IChannelNode)]
    virtual ErrCode INTERFACE_FUNC getNodes(IList** value) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ChannelFolder)

END_NAMESPACE_DEWESOFT_RT_CORE
