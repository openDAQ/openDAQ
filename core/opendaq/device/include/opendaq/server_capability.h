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
#include <coretypes/enumeration.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IEnumeration, EnumerationPtr, "<coretypes/enumeration_ptr.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IServerCapability, IBaseObject)
{
    /*!
     * @brief Gets the connection string of device with current protocol
     * @param[out] connectionString The connection string of device
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

    /*!
     * @brief Gets the name of protocol
     * @param[out] protocolName The name of protocol
     */
    virtual ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the supported type of protocol
     * @param type The supported type of protocol
     */
    virtual ErrCode INTERFACE_FUNC setSupportedProtocolType(IString* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the type of protocol
     * @param type The type of protocol
     */
    virtual ErrCode INTERFACE_FUNC setProtocolType(IString* type) = 0;

    /*!
     * @brief Gets the type of protocol
     * @param[out] type The type of protocol
     */
    virtual ErrCode INTERFACE_FUNC getProtocolType(IEnumeration** type) = 0;

    /*!
     * @brief Gets the type of connection
     * @param[out] type The type of connection
     */
    virtual ErrCode INTERFACE_FUNC getConnectionType(IString** type) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ServerCapability, IServerCapability,
    IString*, connectionString,
    IString*, protocolName,
    IString*, protocolType, 
    IString*, connectionType
    )

END_NAMESPACE_OPENDAQ
