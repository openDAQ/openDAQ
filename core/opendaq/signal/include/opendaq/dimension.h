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
#include <coretypes/listobject.h>
#include <coreobjects/unit.h>
#include <opendaq/dimension_rule.h>

BEGIN_NAMESPACE_OPENDAQ

struct IDimensionBuilder;
/*#
 * [interfaceLibrary(IUnit, CoreObjects)]
 */

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_dimension Dimension
 * @{
 */

/*!
 * @brief Describes a dimension of the signal's data. Eg. a column/row in a matrix.
 *
 * Dimension objects define the size and labels of a single data dimension. Labels, in concert
 * with the unit, provide information about the position of data in a structure. For example, for vector
 * rank data in a frequency domain, the unit would be Hz, and the labels would range from the minimum to the maximum
 * frequency of the spectrum.
 *
 * The number of dimensions a sample descriptor defines the rank of the signals data. When no dimensions are
 * present, one sample is a single value. When there's one dimension, a sample contains a vector of values,
 * when there are three a sample contains a matrix. Higher ranks of data can be represented by adding more
 * dimension objects to a sample descriptor.
 *
 * The labels can be defined with a Rule (in example a linear rule with a coefficient and offset), where the
 * the data index is the input to the rule. In example
 * A Linear rule with coefficient = 5, offset = 10, size = 5 provides the following list of labels:
 * [10, 15, 20, 25, 30]
 *
 * To specify the labels can explicitly, the List dimension rule can be used. The list rule allows for a list of
 * numbers, strings, or ranges to be used as the dimension labels.
 * 
 * Dimension objects implement the Struct methods internally and are Core type `ctStruct`.
 */
DECLARE_OPENDAQ_INTERFACE(IDimension, IBaseObject)
{
    /*!
     * @brief Gets the name of the dimension.
     * @param[out] name The name of the dimension.
     *
     * The name that best describes the dimension, in example "Frequency" for spectrum data.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the size of the dimension.
     * @param[out] size The size of the dimension.
     *
     * The size is obtained from the dimension rule parameters - either from the `size` parameter, or the
     * count of elements in the `list` parameter.
     */
    virtual ErrCode INTERFACE_FUNC getSize(SizeT* size) = 0;

    /*!
     * @brief Gets the unit of the dimension's labels.
     * @param[out] unit The unit of the dimension.
     */
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;

    // [elementType(labels, IBaseObject)]
    /*!
     * @brief Gets a list of labels that defines the dimension.
     * @param[out] labels The list of labels.
     *
     * The list is obtained from the dimension rule parameters by parsing and evaluating the parameters
     * in conjunction with the rule type.
     */
    virtual ErrCode INTERFACE_FUNC getLabels(IList** labels) = 0;

    /*!
     * @brief Gets the rule that defines the labels and size of the dimension.
     * @param[out] rule The dimension rule.
     *
     * The rule takes as input the index of data value in a sample and produces a label associated
     * with that index.
     */
    virtual ErrCode INTERFACE_FUNC getRule(IDimensionRule** rule) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_dimension
 * @addtogroup opendaq_dimension_factories Factories
 * @{
 */

/*!
 * @brief Creates a dimension object of which labels and size are defined via rule.
 * @param rule The rule via which labels are defined.
 * @param unit The unit of the dimension's labels.
 * @param name The name the dimension.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Dimension, IDimension,
    IDimensionRule*, rule,
    IUnit*, unit,
    IString*, name
)

/*!
 * @brief Creates a Dimension using Builder
 * @param builder Dimension Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DimensionFromBuilder,
    IDimension,
    IDimensionBuilder*, builder
)

/*!@}*/

END_NAMESPACE_OPENDAQ
