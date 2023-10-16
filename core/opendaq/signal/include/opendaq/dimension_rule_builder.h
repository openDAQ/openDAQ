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
#include <opendaq/dimension_rule.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_dimension
 * @addtogroup opendaq_dimension_rule Dimension rule
 * @{
 */
    
/*!
 * @brief Configuration component of Dimension rule objects. Contains setter methods that allow for Dimension rule
 * parameter configuration, and a `build` method that builds the Dimension rule.
 */
DECLARE_OPENDAQ_INTERFACE(IDimensionRuleBuilder, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Sets the type of the dimension rule. Rule parameters must be configured to match the requirements of the rule type.
     * @param type The type of the dimension rule.
     *
     * The required rule parameters are as follows:
     *   - Linear: `delta`, `start`, and `size` number parameters. Calculated as: <em>index * delta + start</em> for `size` number of elements.
     *   - Logarithmic: `delta`, `start`, `base`, and `size` number parameters. Calculated as: <em>base ^ (index * delta + start)</em> for `size` number of elements.
     *   - List: `list` parameter. The list contains all dimension labels.
     */
    virtual ErrCode INTERFACE_FUNC setType(DimensionRuleType type) = 0;

    // [templateType(parameters, IString, IBaseObject), returnSelf]
    /*!
     * @brief Sets a dictionary of string-object key-value pairs representing the parameters used to evaluate the rule.
     * @param parameters The dictionary containing the rule parameter members.
     */
    virtual ErrCode INTERFACE_FUNC setParameters(IDict* parameters) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a string-object pair parameter to the Dictionary of Dimension rule parameters.
     * @param name The string-type name of the parameter.
     * @param parameter The object-type parameter.
     */
    virtual ErrCode INTERFACE_FUNC addParameter(IString* name, IBaseObject* parameter) = 0;

    // [returnSelf]
    /*!
     * @brief Removes the parameter with the given name from the Dictionary of Dimension rule parameters.
     */
    virtual ErrCode INTERFACE_FUNC removeParameter(IString* name) = 0;

    /*!
     * @brief Builds and returns a Dimension rule object using the currently set values of the Builder.
     * @param[out] dataRule The built Dimension rule.
     */
    virtual ErrCode INTERFACE_FUNC build(IDimensionRule** dimensionRule) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_dimension_rule
 * @addtogroup opendaq_dimension_rule_factories Factories
 * @{
 */

/*!
 * @brief Creates a DataRuleConfig with no parameters.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DimensionRuleBuilder, IDimensionRuleBuilder)

/*!
 * @brief Dimension rule copy factory that creates a builder Rule object from a possibly non-configurable Rule.
 * @param ruleToCopy The rule of which configuration should be copied.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DimensionRuleBuilderFromExisting, IDimensionRuleBuilder,
    IDimensionRule*, ruleToCopy
)

/*!@}*/

END_NAMESPACE_OPENDAQ
