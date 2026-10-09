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
#include <opendaq/component_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
 * [interfaceLibrary(IComponentType, "opendaq")]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_function_block_type Function block type
 * @{
 */

/*!
 * @brief Provides information about the function block.
 */
DECLARE_OPENDAQ_INTERFACE(IFunctionBlockType, IComponentType)
{
    /*!
     * @brief Gets the alwaysEmptyInput flag value.
     * @param[out] alwaysEmpty The flag value.
     *
     * When alwaysEmptyInput flag is true, the function blocks of that type guarantee that they
     * will always have an empty input port available for further connections. Defaults to false
     * (i.e. when not set by the FB type provider).
     */
    virtual ErrCode INTERFACE_FUNC getAlwaysEmptyInput(Bool* alwaysEmpty) = 0;

    /*!
     * @brief Gets the singleton flag value.
     * @param[out] singleton The flag value.
     *
     * When singleton flag is true, the function block type guarantees that at most one instance
     * of the function block type can exist under a single parent and adding a second function
     * block of the same type will fail. Defaults to false (i.e. when not set by the FB type
     * provider).
     */
    virtual ErrCode INTERFACE_FUNC getSingleton(Bool* singleton) = 0;

    /*!
     * @brief Gets the commonSettingsTypeId - the Id for which this FB type acts as a common
     * settings folder.
     * @param[out] typeId The Id of the function block type.
     *
     * When assigned, the function block type declares that it acts as a common settings holder
     * for nested function blocks of the type specified by commonSettingsTypeId (provided via
     * the typeId output parameter). When unassigned, the function block does not declare such
     * a relationship.
     */
    virtual ErrCode INTERFACE_FUNC getCommonSettingsTypeId(IString** typeId) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_function_block_type
 * @addtogroup opendaq_function_block_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a Function block type object, with the id, name, description and optional defaultConfig.
 * @param id The unique type ID of the function block.
 * @param name The name of the function block. Eg. FFT.
 * @param description A short description of the function block and its behaviour.
 * @param defaultConfig The property object, to be cloned and returned, each time user creates default
 * configuration object. This way each instance of the function block has its own configuration object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, FunctionBlockType,
    IString*, id,
    IString*, name,
    IString*, description,
    IPropertyObject*, defaultConfig
)

/*!@}*/

END_NAMESPACE_OPENDAQ
