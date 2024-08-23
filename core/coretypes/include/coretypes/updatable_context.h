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
#include <coretypes/stringobject.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_utility
 * @defgroup types_updatable Updatable
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IUpdatableContext, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) = 0;
    virtual ErrCode INTERFACE_FUNC getInputPortConnection(IString* parentId, IDict** connections) = 0;
    virtual ErrCode INTERFACE_FUNC setNotMutedComponent(IString* componentId) = 0;
    virtual ErrCode INTERFACE_FUNC getComponentIsMuted(IString* componentId, Bool* muted) = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
