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
#include <opendaq/scaling_ptr.h>
#include <opendaq/scaling_builder_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scaling
 * @addtogroup opendaq_scaling_factories Factories
 * @{
 */

/*!
 * @brief Creates a Scaling with a Linear scaling type configuration.
 *
 * @param scale Coefficient by which the input data is to be multiplied.
 * @param offset Constant that is added to the <em>scale * value</em> multiplication result.
 * @param inputDataType The scaling's input data type.
 * @param outputDataType The scaling's output data type.
 */
inline ScalingPtr LinearScaling(const NumberPtr& scale,
                                const NumberPtr& offset,
                                SampleType inputDataType = SampleType::Float64,
                                ScaledSampleType outputDataType = ScaledSampleType::Float64)
{
    ScalingPtr obj(LinearScaling_Create(scale, offset, inputDataType, outputDataType));
    return obj;
}

/*!
 * @brief Creates a Scaling object with given input/output types, Scaling type and parameters.
 *
 * @param type The type of the Dimension rule.
 * @param parameters Tha parameters of the Dimension rule.
 * @param outputDataType The data type output by the rule after calculation.
 * @param inputDataType The type of input data expected by the rule.
 */
inline ScalingPtr Scaling(SampleType inputDataType, ScaledSampleType outputDataType, ScalingType scalingType, const DictPtr<IString, IBaseObject>& params)
{
    ScalingPtr obj(Scaling_Create(inputDataType, outputDataType, scalingType, params));
    return obj;
}

/*!
 * @brief Creates a Scaling object from Builder
 *
 * @param builder Scaling Builder
 */
inline ScalingPtr ScalingFromBuilder(const ScalingBuilderPtr& builder)
{
    ScalingPtr obj(ScalingFromBuilder_Create(builder));
    return obj;
}

/*!
 * @brief Creates a Scaling builder object with no parameters configured.
 */
inline ScalingBuilderPtr ScalingBuilder()
{
    ScalingBuilderPtr obj(ScalingBuilder_Create());
    return obj;
}

/*!
 * @brief Scaling builder copy factory that creates a configurable Scaling object from a
 * non-configurable one.
 * @param scaling The scaling of which configuration should be copied.
 */
inline ScalingBuilderPtr ScalingBuilderCopy(const ScalingPtr& scaling)
{
    ScalingBuilderPtr obj(ScalingBuilderFromExisting_Create(scaling));
    return obj;
}

inline StructTypePtr ScalingStructType()
{
    return StructType("Scaling",
                      List<IString>("OutputDataType", "InputDataType", "RuleType", "Parameters"),
                      List<IBaseObject>(0, 0, 0, Dict<IString, IBaseObject>()),
                      List<IType>(SimpleType(ctInt), SimpleType(ctInt), SimpleType(ctInt), SimpleType(ctDict)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
