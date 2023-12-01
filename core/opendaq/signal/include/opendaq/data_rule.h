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
#include <coretypes/dictobject.h>
#include <coretypes/number.h>

BEGIN_NAMESPACE_OPENDAQ

struct IDataRuleBuilder;

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_data_rule Data rule
 * @{
 */

/*!
 * @brief Enumeration of available Data rule types
 */
enum class DataRuleType
{
    Other = 0, ///< The rule is unknown to openDAQ and cannot be handled automatically.
    Linear,    ///< The parameters contain a `delta` and `start` parameters member. The value is calculated as: <em>inputValue * delta + start</em> .
    Constant,  ///< The value is a constant, as defined in the `constant` parameter field.
    Explicit   ///< The value is explicitly defined and is part of the signal's packet buffers.
};


/*!
 * @brief Rule that defines how a signal value is calculated from an implicit initialization
 * value when the rule type is not `Explicit`.
 *
 * Data rule objects implement the Struct methods internally and are Core type `ctStruct`.
 *
 * @subsection data_rule_explicit Explicit rule
 * When the rule type of the Data rule is set to `Explicit`, the values passed through the signal path, described by
 * the Value descriptor are stored in packet buffers.
 *
 * The Explicit rule can have 2 optional parameters:
 *
 * - `minExpectedDelta`: Specifies the minimum difference in value between two subsequent samples
 * - `maxExpectedDelta`: Specifies the maximum difference in value between two subsequent samples
 *
 * These are mostly used for domain signals to specify the expected rate of a signal, or the expected timeout of a signal.
 * The delta parameters should be configured to match the deltas in terms of the raw signal values (before scaling/resolution
 * are applied).
 *
 * An explicit rule must have either both or none of these parameters. To use only one, the other must be set to 0.
 *
 * @subsection data_rule_implicit Implicit rule
 * When the rule type of the Data rule is not `Explicit`, the buffers of packets are empty. The values must instead be
 * calculated via the Implicit value found in the packet buffers in conjunction with the parameters of the rule. Each
 * implicit rule type specifies its own required set of parameters.
 *
 * @subsubsection data_rule_linear Linear rule
 * The parameters include a `delta` and `start` integer members. The values are calculated according to the following
 * equation: <em>packetOffset + sampleIndex * delta + start</em>.
 *
 * @subsubsection data_rule_linear Constant rule
 * The parameters contain a `constant` number member. The value described by the constant rule is always equal to the
 * constant.
 */
DECLARE_OPENDAQ_INTERFACE(IDataRule, IBaseObject)
{
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
};
/*!@}*/

/*!
 * @ingroup opendaq_data_rule
 * @addtogroup opendaq_data_rule_factories Factories
 * @{
 */

/*!
 * @brief Creates a DataRule with a Linear rule type configuration.
 *
 * @param delta Coefficient by which the input data is to be multiplied.
 * @param start Constant that is added to the <em>scale * value</em> multiplication result.
 *
 * The scale and offset are stored within the `parameters` member of the Rule object
 * with the scale being at the first position of the list, and the offset at the second.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, LinearDataRule, IDataRule,
    INumber*, delta,
    INumber*, start
)

/*!
 * @brief Creates a DataRule with a Constant rule type configuration.
 * @param constant Constant value to be used in the rule.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ConstantDataRule, IDataRule,
    INumber*, constant
)

/*!
 * @brief Creates a DataRule with an Explicit rule type configuration and no parameters.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ExplicitDataRule, IDataRule,
    OPENDAQ_FACTORY_PARAMS()
)

/*!
 * @brief Creates a DataRule with an Explicit rule type configuration two optional parameters.
 * @param minExpectedDelta The lowest expected distance between two samples.
 * @param maxExpectedDelta The highest expected distance between two samples.
 *
 * Most often used for domain signals to specify estimates on how close together/far apart two
 * subsequent samples might be.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ExplicitDomainDataRule, IDataRule,
    INumber*, minExpectedDelta,
    INumber*, maxExpectedDelta
)

/*!
 * @brief Creates a DataRule with an Explicit rule type configuration and parameters.
 * @param ruleType .
 * @param parameters .
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DataRule, IDataRule,
    DataRuleType, ruleType,
    IDict*, parameters
)

/*!
 * @brief Creates a DataRulePtr from Builder.
 * @param builder DataRule Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DataRuleFromBuilder, IDataRule,
    IDataRuleBuilder*, builder
)

/*!@}*/

END_NAMESPACE_OPENDAQ
