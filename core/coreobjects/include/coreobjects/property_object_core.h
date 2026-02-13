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
#include <coreobjects/property_object_internal.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

enum class PropObjectCoreVariableId : EnumType
{
    LockOwner = 0,
    Mutex = 10,
    LockingStrategy = 20,
};

DECLARE_OPENDAQ_INTERFACE(IPropertyObjectCore, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getRecursiveLockGuard(ILockGuard** lockGuard) = 0;
    virtual ErrCode INTERFACE_FUNC getLockGuard(ILockGuard** lockGuard) = 0;
    virtual ErrCode INTERFACE_FUNC setInternalVariable(PropObjectCoreVariableId varId, IBaseObject* value) = 0;
    virtual ErrCode INTERFACE_FUNC setInternalVariables(IDict* varIdValueMap) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObjectCore)

END_NAMESPACE_OPENDAQ
