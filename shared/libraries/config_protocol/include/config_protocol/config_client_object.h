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
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IComponent, "opendaq")]
 * [interfaceLibrary(ICoreEventArgs, "coreobjects")]
 */
DECLARE_OPENDAQ_INTERFACE(IConfigClientObject, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getRemoteGlobalId(IString** remoteGlobalId) = 0;
    virtual ErrCode INTERFACE_FUNC setRemoteGlobalId(IString* remoteGlobalId) = 0;
    virtual ErrCode INTERFACE_FUNC handleRemoteCoreEvent(IComponent* sender, ICoreEventArgs* args) = 0;
    virtual ErrCode INTERFACE_FUNC remoteUpdate(ISerializedObject* serialized) = 0;
};

END_NAMESPACE_OPENDAQ
