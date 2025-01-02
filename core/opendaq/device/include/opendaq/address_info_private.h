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
#include <opendaq/address_info.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IAddressInfoPrivate, IBaseObject)
{
    /*!
     * @brief Sets the reachability status of the address, ignoring the "Frozen" status of the address.
     * @param addressReachability The reachability status of the address.
     *
     * This status is set to "Unknown" by default. For IPv4 address types, the module manager checks
     * reachability when querying for available devices.
     */
    virtual ErrCode INTERFACE_FUNC setReachabilityStatusPrivate(AddressReachabilityStatus addressReachability) = 0;
};

END_NAMESPACE_OPENDAQ
