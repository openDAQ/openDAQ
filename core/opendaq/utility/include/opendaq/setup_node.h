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
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_setup_node SetupNode
 * @{
 */
    
enum class NodeType : EnumType
{
    Device = 0,
    FunctionBlock = 1,
    Signal,
    InputPort,
    Server,
    IOFolder,
    Channel,
    SyncComponent,
    Component,
    Folder,
    Instance,
    Unknown = 99
};

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */
DECLARE_OPENDAQ_INTERFACE(ISetupNode, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getLocalId(IString** id) = 0;
    virtual ErrCode INTERFACE_FUNC getType(NodeType* type) = 0;
    virtual ErrCode INTERFACE_FUNC getOptions(IPropertyObject** options) = 0;
    // [elementType(nodes, ISetupNode)]
    virtual ErrCode INTERFACE_FUNC getChildNodes(IList** nodes) = 0;
    // [elementType(nodes, ISetupNode)]
    virtual ErrCode INTERFACE_FUNC getChildNodesWithType(IList** nodes, NodeType type, Bool recursive) = 0;
    virtual ErrCode INTERFACE_FUNC getParent(ISetupNode** parent) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, SetupNode, IString*, setupString);

END_NAMESPACE_OPENDAQ
