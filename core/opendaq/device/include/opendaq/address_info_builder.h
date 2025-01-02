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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <opendaq/address_info.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_server_capability
 * @addtogroup opendaq_address_info Address info
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IAddressInfoBuilder, IBaseObject)
{
    /*!
     * @brief Builds the address.
     * @param[out] address The address.
     */
    virtual ErrCode INTERFACE_FUNC build(IAddressInfo** address) = 0;
    
    /*!
     * @brief Gets the server address as a string.
     * @param[out] address The server address as a string.
     */
    virtual ErrCode INTERFACE_FUNC getAddress(IString** address) = 0;
    
    // [returnSelf()]
    /*!
     * @brief Sets the server address as a string.
     * @param address The server address as a string.
     */
    virtual ErrCode INTERFACE_FUNC setAddress(IString* address) = 0;
    
    /*!
     * @brief Gets the connection string corresponding to the address.
     * @param[out] connectionString The connection string.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;
    
    // [returnSelf()]
    /*!
     * @brief Sets the connection string corresponding to the address.
     * @param connectionString The connection string.
     */
    virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

    /*!
     * @brief Gets the type of the address.
     * @param[out] type The type the address.
     *
     * Currently available address types in the main openDAQ modules are: IPv4 and IPv6.
     */
    virtual ErrCode INTERFACE_FUNC getType(IString** type) = 0;
    
    // [returnSelf()]
    /*!
     * @brief Sets the type of the address.
     * @param type The type the address.
     *
     * Currently available address types in the main openDAQ modules are: IPv4 and IPv6.
     */
    virtual ErrCode INTERFACE_FUNC setType(IString* type) = 0;

    /*!
     * @brief Gets the reachability status of the address.
     * @param addressReachability The reachability status of the address.
     *
     * This status is set to "Unknown" by default. For IPv4 address types, the module manager checks
     * reachability when querying for available devices.
     */
    virtual ErrCode INTERFACE_FUNC getReachabilityStatus(AddressReachabilityStatus* addressReachability) = 0;
    
    // [returnSelf()]
    /*!
     * @brief Sets the reachability status of the address.
     * @param addressReachability The reachability status of the address.
     *
     * This status is set to "Unknown" by default. For IPv4 address types, the module manager checks
     * reachability when querying for available devices.
     */
    virtual ErrCode INTERFACE_FUNC setReachabilityStatus(AddressReachabilityStatus addressReachability) = 0;
};

/*!@}*/

/*!
 * @ingroup opendaq_address_info
 * @addtogroup opendaq_address_info_factories Factories
 * @{
 */

/*!
 * @brief Creates an Address builder with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AddressInfoBuilder, IAddressInfoBuilder)

/*!@}*/

END_NAMESPACE_OPENDAQ
