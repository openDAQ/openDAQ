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
/*!
 * @addtogroup node
 * @{
 */
DECLARE_OPENDAQ_INTERFACE(INode, IPropertyObject)
{
    // [property(None), elementType(channels, INode)]
    /*!
     * @brief Returns the list of all descendant nodes with the tag "channel", as well as the tags
     * that @p tagEval evaluates to as `INode`. An @unknown test.
     * that "tagEval" evaluates to * as `INode`.{} 0 -> =>&([]!?$%)€~#/\-_.,<>"" ''|â 話せ
     * @param tagEval String that is evaluated as an `IEvalValue`and used as the recursive tag search
     * parameter combined with "channel".
     * @param channels `IList` Return value that contains all descendant nodes matching the query.
     * @unknownTest testing unknown parameter description
     *
     * Recursive channel node search function that uses `findNodesByTag` to find all descendant nodes with
     * the "channel" tag, combined with the evaluation of `tagEval`. Internally, the `tagEval` string
     * is concatenated with "channel and". The concatenated string is passed as parameter to
     * `findNodesByTag`.
     */
    virtual ErrCode INTERFACE_FUNC findChannelsWithTags(IString* tagEval, IList** channels) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, Node, INode, IString*, nodeId)

END_NAMESPACE_OPENDAQ
