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
#include <coreobjects/component_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<coreobjects/component_type_ptr.h>")]
 * [interfaceLibrary(IComponentType, "coreobjects")]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_function_block_type Streaming type
 * @{
 */

/*!
 * @brief Provides information on what streaming type can be created by a given module. Can be used
 * to obtain the default configuration used when either adding/creating a new device, or establishing
 * a new streaming connection.
 */
DECLARE_OPENDAQ_INTERFACE(IStreamingType, IComponentType)
{
    /*
     * @brief Gets the prefix found in connection strings used to establish a streaming connection of
     * this type.
     * @param[out] prefix The connection string prefix.
     *
     * The prefix is always found at the start of the connection string, before the "://" delimiter.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionStringPrefix(IString** prefix) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_function_block_type
 * @addtogroup opendaq_function_block_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a Streaming type object, with the id, name, description and optional defaultConfig.
 * @param id The unique type ID of the Streaming.
 * @param name The name of the Streaming. Eg. FFT.
 * @param description A short description of the Streaming and its behaviour.
 * @param defaultConfig The property object, to be cloned and returned, each time user creates default
 * configuration object. This way each instance of the Streaming has its own configuration object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, StreamingType,
    IString*, id,
    IString*, name,
    IString*, description,
    IString*, prefix,
    IPropertyObject*, defaultConfig
)

/*!@}*/

END_NAMESPACE_OPENDAQ
