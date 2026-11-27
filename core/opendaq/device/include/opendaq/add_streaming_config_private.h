/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/add_streaming_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IAddStreamingConfig, AddStreamingConfigPtr, "<opendaq/add_streaming_config_ptr.h>")]
 */

/*!
 * @ingroup opendaq_add_device_config
 * @addtogroup opendaq_add_streaming_config_private Add streaming config private
 * @{
 */

/*!
 * @brief Provides access to private methods of the add streaming config.
 */
DECLARE_OPENDAQ_INTERFACE(IAddStreamingConfigPrivate, IBaseObject)
{
    // [templateType(streamingTypes, IString, IStreamingType)]
    /*!
     * @brief Creates a config bound by type ID to the given types, with the values of this config.
     * @param streamingTypes The available Streaming types, keyed by ID
     * @param[out] bound The bound config
     * @retval OPENDAQ_ERR_NOTFOUND when the Streaming type is not among the given types.
     * @retval OPENDAQ_ERR_INVALIDVALUE when the values of this config don't fit the given type.
     *
     * The bound config is created from the default configs of the given type, with the values of this config applied.
     * Unset values take the current defaults of the given type.
     * The supplied credential of the authentication config is not kept.
     * The deprecated `Username` and `Password` of the type config are reset to their defaults when the authentication config
     * has the `UserNamePassword` method, as the credential of such a type is supplied through the authentication config.
     * A type without that method keeps them: its module has no authentication config to get the credential from, so it
     * still reads them from the type config.
     */
    virtual ErrCode INTERFACE_FUNC bind(IDict* streamingTypes, IAddStreamingConfig** bound) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
