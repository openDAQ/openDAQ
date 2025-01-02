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
#include <opendaq/sample_type.h>
#include <coretypes/dictobject.h>
#include <coretypes/number.h>

BEGIN_NAMESPACE_OPENDAQ

struct IScalingBuilder;

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_scaling Scaling
 * @{
 */

/*!
 * @brief Enumeration of available scaling types
 */
enum class ScalingType
{
    Other = 0, 
    Linear ///< The parameters contain a `scale` and `offset`. Calculated as: <em>inputValue * scale + offset</em> .
};

/*#
 * [interfaceLibrary(INumber, CoreTypes)]
 * [include(ISampleType)]
 */

/*!
 * @brief Signal descriptor field that defines a scaling transformation, which should be
 * applied to data carried by the signal's packets when read.
 *
 * Each scaling specifies its `scalingType` and parses the parameters accordingly. The parameters
 * are to be interpreted and used as specified by each specific scaling type as detailed below.
 *
 * Additionally, each Scaling object states is input and output data types. The inputDataType
 * describes the raw data type of the signal value (that data is input into the scaling scaling),
 * while the outputDataType should match the sample type of the signal's value descriptor.
 * 
 * Scaling objects implement the Struct methods internally and are Core type `ctStruct`.
 *
 * @subsection scaling_types Scaling types
 * @subsubsection scaling_types_linear Linear scaling
 * Linear scaling parameters must have two entries:
 *   - Scale: coefficient by which the input data is to be multiplied
 *   - Offset: a constant that is added to the <em>scale * value</em> multiplication result
 *
 * The linear scaling output is calculated as follows: <em>inputValue * scale + offset</em>
 */
DECLARE_OPENDAQ_INTERFACE(IScaling, IBaseObject)
{
    /*!
     * @brief Gets the scaling's input data type.
     * @param[out] type The input data type
     *
     * The input data type corresponds to the raw values passed through the signal path in
     * data packets.
     */
    virtual ErrCode INTERFACE_FUNC getInputSampleType(SampleType* type) = 0;

    /*!
     * @brief Gets the scaling's output data type.
     * @param[out] type The output data type
     *
     * The output data type corresponds to the sample type specified in the value descriptor of
     * a signal, and is the type in which said signal's data should be read in after having
     * the scaling applied to it.
     */
    virtual ErrCode INTERFACE_FUNC getOutputSampleType(ScaledSampleType* type) = 0;

    /*!
     * @brief Gets the type of the scaling that determines how the scaling parameters should be interpreted
     * and how the scaling should be calculated.
     * @param[out] type The type of the scaling.
     */
    virtual ErrCode INTERFACE_FUNC getType(ScalingType* type) = 0;

    // [templateType(parameters, IString, IBaseObject)]
    /*!
     * @brief Gets the dictionary of parameters that are used to calculate the scaling in conjunction with the input data.
     * @param[out] parameters The dictionary of parameters.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IDict** parameters) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_scaling
 * @addtogroup opendaq_scaling_factories Factories
 * @{
 */

/*!
 * @brief Creates a Scaling with a Linear scaling type configuration.
 * The returned Scaling object is already frozen.
 *
 * @param scale Coefficient by which the input data is to be multiplied.
 * @param offset Constant that is added to the <em>scale * value</em> multiplication result.
 * @param inputDataType The scaling's input data type.
 * @param outputDataType The scaling's output data type.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, LinearScaling, IScaling,
    INumber*, scale,
    INumber*, offset,
    SampleType, inputDataType,
    ScaledSampleType, outputDataType
)

/*!
 * @brief Creates a Scaling object with given input/output types, Scaling type and parameters.
 *
 * @param inputDataType The type of input data expected by the rule.
 * @param outputDataType The data type output by the rule after calculation.
 * @param scalingType The type of the scaling.
 * @param parameters Tha parameters of the Dimension rule.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Scaling, IScaling,
    SampleType, inputDataType,
    ScaledSampleType, outputDataType,
    ScalingType, scalingType,
    IDict*, parameters
)

/*!
 * @brief Creates a Scaling object from Builder
 *
 * @param builder Scaling Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ScalingFromBuilder, IScaling,
    IScalingBuilder*, builder
)

/*!@}*/

END_NAMESPACE_OPENDAQ
