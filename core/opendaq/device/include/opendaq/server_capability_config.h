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
#include <coreobjects/property.h>
#include <coreobjects/property_object.h>
#include <opendaq/server_capability.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IProperty, "coreobjects")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IServerCapabilityConfig, IServerCapability)
{
    // [returnSelf]
    /*!
     * @brief Sets the connection string of device with current protocol
     * @param connectionString The connection string of device
     */
    virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the name of protocol
     * @param protocolName The name of protocol
     */
    virtual ErrCode INTERFACE_FUNC setProtocolName(IString* protocolName) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the type of protocol
     * @param type The type of protocol
     */
    virtual ErrCode INTERFACE_FUNC setProtocolType(IString* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the type of connection
     * @param type The type of connection
     */
    virtual ErrCode INTERFACE_FUNC setConnectionType(IString* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the client update method
     * @param type The client update method
     */
    virtual ErrCode INTERFACE_FUNC setCoreEventsEnabled(Bool enabled) = 0;

    // [returnSelf]
    /*!
     * @brief Adds the property to the Property object.
     * @param property The property to be added.
     * @retval OPENDAQ_ERR_INVALIDVALUE if the property has no name.
     * @retval OPENDAQ_ERR_ALREADYEXISTS if a property with the same name is already part of the Property object.
     * @retval OPENDAQ_ERR_FROZEN if the Property object is frozen.
     *
     * The Property is frozen once added to the Property object, making it immutable. The same Property cannot
     * be added to multiple different Property objects.
     */
    virtual ErrCode INTERFACE_FUNC addProperty(IProperty* property) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ServerCapability, IServerCapabilityConfig,
    IContext*, context,
    IString*, protocolName,
    IString*, protocolType
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ServerStreamingCapability, IServerCapabilityConfig,
    IContext*, context,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
