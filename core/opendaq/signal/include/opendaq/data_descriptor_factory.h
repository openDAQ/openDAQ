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
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_descriptor_builder_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/range_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/scaling_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_data_descriptor_factories Factories
 * @{
 */

/*!
 * @brief Data descriptor builder factory that creates a builder object with no parameters configured.
 */
inline DataDescriptorBuilderPtr DataDescriptorBuilder()
{
    DataDescriptorBuilderPtr obj(DataDescriptorBuilder_Create());
    return obj;
}

/*!
 * @brief Data descriptor copy factory that creates a Data descriptor builder object from a
 * different Data descriptor, copying its parameters.
 * @param descriptorToCopy The Data descriptor of which configuration should be copied.
 */
inline DataDescriptorBuilderPtr DataDescriptorBuilderCopy(const DataDescriptorPtr& dataDescriptor)
{
    DataDescriptorBuilderPtr obj(DataDescriptorBuilderFromExisting_Create(dataDescriptor));
    return obj;
}

/*!
 * @brief Creates a Data descriptor object using the build parameters Dictionary.
 * @param buildParams Dictionary of build parameters such as the Name and Dimensions.
 */
inline DataDescriptorPtr DataDescriptor(const DictPtr<IString, IBaseObject>& descriptorParams)
{
    DataDescriptorPtr obj(DataDescriptor_Create(descriptorParams));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Data descriptor struct.
 */
inline StructTypePtr DataDescriptorStructType()
{
    return StructType("dataDescriptor",
                      List<IString>("dimensions",
                                    "name",
                                    "sampleType",
                                    "unit",
                                    "valueRange",
                                    "dataRule",
                                    "scaling",
                                    "origin",
                                    "tickResolution",
                                    "structFields",
                                    "metadata"),
                      List<IBaseObject>(List<IDimension>(),
                                        "",
                                        0,
                                        nullptr,
                                        nullptr,
                                        ExplicitDataRule(),
                                        nullptr,
                                        nullptr,
                                        nullptr,
                                        nullptr,
                                        Dict<IString, IBaseObject>()),
                      List<IType>(SimpleType(ctList),
                                  SimpleType(ctString),
                                  SimpleType(ctInt),
                                  UnitStructType(),
                                  RangeStructType(),
                                  DataRuleStructType(),
                                  ScalingStructType(),
                                  SimpleType(ctString),
                                  RatioStructType(),
                                  SimpleType(ctList),
                                  SimpleType(ctDict)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
