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
#include <opendaq/data_rule.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_data_rule Data rule
 * @{
 */

/*!
 * @brief Configuration component of Data rule objects. Contains setter methods that allow for Data rule
 * parameter configuration, and a `build` method that builds the Data rule.
 */
DECLARE_OPENDAQ_INTERFACE(IDataRuleBuilder, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Sets the type of the data rule.
     * @param type The type of the data rule.
     */
    virtual ErrCode INTERFACE_FUNC setType(DataRuleType type) = 0;

    // [templateType(parameters, IString, IBaseObject), returnSelf]
    /*!
     * @brief Sets a dictionary of string-object key-value pairs representing the parameters used to evaluate the rule.
     * @param parameters The dictionary containing the rule parameter members.
     */
    virtual ErrCode INTERFACE_FUNC setParameters(IDict* parameters) = 0;

    /*!
     * @brief Gets the type of the data rule.
     * @param[out] type The type of the data rule.
     */
    virtual ErrCode INTERFACE_FUNC getType(DataRuleType* type) = 0;

    // [templateType(parameters, IString, IBaseObject)]
    /*!
     * @brief Gets a dictionary of string-object key-value pairs representing the parameters used to evaluate the rule.
     * @param[out] parameters The dictionary containing the rule parameter members.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IDict** parameters) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a string-object pair parameter to the Dictionary of Data rule parameters.
     * @param name The string-type name of the parameter.
     * @param parameter The object-type parameter.
     */
    virtual ErrCode INTERFACE_FUNC addParameter(IString* name, IBaseObject* parameter) = 0;

    // [returnSelf]
    /*!
     * @brief Removes the parameter with the given name from the Dictionary of Data rule parameters.
     */
    virtual ErrCode INTERFACE_FUNC removeParameter(IString* name) = 0;

    /*!
     * @brief Builds and returns a Data rule object using the currently set values of the Builder.
     * @param[out] dataRule The built Data rule.
     */
    virtual ErrCode INTERFACE_FUNC build(IDataRule** dataRule) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_data_rule
 * @addtogroup opendaq_data_rule_factories Factories
 * @{
 */

/*!
 * @brief Creates a Data rule builder with no parameters.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DataRuleBuilder, IDataRuleBuilder)

/*!
 * @brief Data rule copy factory that creates a configurable Data rule builder object from a
 * possibly non-configurable Data rule.
 * @param ruleToCopy The rule of which configuration should be copied.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY,
    DataRuleBuilderFromExisting, IDataRuleBuilder,
    IDataRule*, ruleToCopy
)

/*!@}*/

END_NAMESPACE_OPENDAQ
