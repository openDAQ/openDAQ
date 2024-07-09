/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/dictobject_factory.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <opendaq/rule_private.h>
#include <opendaq/scaling_builder.h>
#include <opendaq/scaling_builder_ptr.h>
#include <opendaq/scaling_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ScalingBuilderImpl : public ImplementationOf<IScalingBuilder>
{
public:
    explicit ScalingBuilderImpl();
    explicit ScalingBuilderImpl(const ScalingPtr& scalingToCopy);

    explicit ScalingBuilderImpl(SampleType inputType,
                                ScaledSampleType outputType,
                                ScalingType ruleType,
                                DictPtr<IString, IBaseObject> params);

    explicit ScalingBuilderImpl(NumberPtr scale,
                                NumberPtr offset,
                                SampleType inputType,
                                ScaledSampleType outputType);

    ErrCode INTERFACE_FUNC build(IScaling** scaling) override;

    ErrCode INTERFACE_FUNC setInputDataType(SampleType type) override;
    ErrCode INTERFACE_FUNC getInputDataType(SampleType* type) override;

    ErrCode INTERFACE_FUNC setOutputDataType(ScaledSampleType type) override;
    ErrCode INTERFACE_FUNC getOutputDataType(ScaledSampleType* type) override;

    ErrCode INTERFACE_FUNC setScalingType(ScalingType type) override;
    ErrCode INTERFACE_FUNC getScalingType(ScalingType* type) override;

    ErrCode INTERFACE_FUNC setParameters(IDict* parameters) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;
    ErrCode INTERFACE_FUNC addParameter(IString* name, IBaseObject* parameter) override;
    ErrCode INTERFACE_FUNC removeParameter(IString* name) override;

private:
    ScaledSampleType outputDataType;
    SampleType inputDataType;
    ScalingType ruleType;
    DictPtr<IString, IBaseObject> params;
};

END_NAMESPACE_OPENDAQ
