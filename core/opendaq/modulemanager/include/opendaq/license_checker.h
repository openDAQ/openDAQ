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
#include <opendaq/context.h>
#include <opendaq/device.h>
#include <opendaq/function_block.h>
#include <opendaq/server.h>
#include <opendaq/streaming.h>
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>
#include <opendaq/server_capability_config.h>
#include <opendaq/module_info.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 *  @brief Licensed modules implement tokens as a means of keeping track of how many function blocks are
 *         available to the user given their license. This interface should be implemented to keep track of them.
 */
DECLARE_OPENDAQ_INTERFACE(ILicenseChecker, IBaseObject)
{
    // [elementType(componentTypes, IString)]
    virtual ErrCode INTERFACE_FUNC getComponentTypes(IList** componentTypes) = 0;
    virtual ErrCode INTERFACE_FUNC getNumberOfAvailableTokens(IString * componentId, Int * availableTokens) = 0;

    /// @brief Tries to check out a number of tokens for a specific license feature, so that the feature can be used.
    /// @param feature The feature name to check out.
    /// @param count The number of tokens to check out.
    /// @return ErrCode Returns an error code indicating the success or failure of the operation.
    virtual ErrCode INTERFACE_FUNC checkOut(IString * feature, SizeT count) = 0;

    /**
     * @brief Checks in a specified feature with a given count.
     *
     * This function is used to release or return a previously checked-out feature
     * back to the license system. It ensures that the feature is no longer in use
     * and updates the license count accordingly.
     *
     * @param feature The feature name to check in.
     * @param count The number of licenses to check in (so that they become available again).
     * @return ErrCode Returns an error code indicating the success or failure of the operation.
     */
    virtual ErrCode INTERFACE_FUNC checkIn(IString * feature, SizeT count) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
