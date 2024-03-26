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
#include <coretypes/baseobject.h>
#include <coretypes/type_manager.h>
#include <opendaq/component.h>

BEGIN_NAMESPACE_OPENDAQ


/*#
 * [interfaceLibrary(ITypeManager, "coretypes")]
 */

DECLARE_OPENDAQ_INTERFACE(IComponentDeserializeContext, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getParent(IComponent** parent) = 0;
    virtual ErrCode INTERFACE_FUNC getRoot(IComponent** root) = 0;
    virtual ErrCode INTERFACE_FUNC getLocalId(IString** localId) = 0;
    virtual ErrCode INTERFACE_FUNC getContext(IContext** context) = 0;
    virtual ErrCode INTERFACE_FUNC getIntfID(IntfID* intfID) = 0;
    // [arrayArg(newIntfID, 1)]
    virtual ErrCode INTERFACE_FUNC clone(
        IComponent * newParent,
        IString* newLocalId,
        IComponentDeserializeContext** newComponentDeserializeContext,
        IntfID* newIntfID) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, ComponentDeserializeContext, IContext*, context, IComponent*, root, IComponent*, parent, IString*, localId, IntfID*, intfID);

END_NAMESPACE_OPENDAQ
