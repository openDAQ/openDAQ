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
#include <coreobjects/coreobjects.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Extends IComponent interface with functionality needed for openDAQ client components.
 */
DECLARE_OPENDAQ_INTERFACE(ITmsClientComponent, IBaseObject)
{
    /*!
     * @brief Gets the global ID of the component as defined by openDAQ server.
     * @param[out] globalId The global ID of the server component.
     *
     * OpenDAQ client device can be connected to multiple openDAQ servers. This means that the global ID which was unique on the server,
     * is no longer unique on the client device. In that case a unique prefix is added to each client openDAQ device.
     * This means that global ID of a client component can be differnet that the one of the same component on the server side.
     * This method returs global ID as defined by openDAQ server.
     */
    virtual ErrCode INTERFACE_FUNC getRemoteGlobalId(IString * *globalId) = 0;
};

END_NAMESPACE_OPENDAQ
