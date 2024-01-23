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
#include <opendaq/scaling.h>
#include <opendaq/rule_private.h>
#include <coretypes/struct_impl.h>
#include <coretypes/dictobject_factory.h>
#include <opendaq/scaling_builder_ptr.h>
#include <opendaq/scaling_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ScalingImpl : public GenericStructImpl<IScaling, IStruct, IRulePrivate>
{
public:
    explicit ScalingImpl(SampleType inputType,
                         ScaledSampleType outputType,
                         ScalingType ruleType,
                         DictPtr<IString, IBaseObject> params);

    explicit ScalingImpl(NumberPtr scale,
                         NumberPtr offset,
                         SampleType inputType,
                         ScaledSampleType outputType);

    explicit ScalingImpl(IScalingBuilder* scalingBuilder);

    ErrCode INTERFACE_FUNC getInputSampleType(SampleType* type) override;
    ErrCode INTERFACE_FUNC getOutputSampleType(ScaledSampleType* type) override;

    ErrCode INTERFACE_FUNC getType(ScalingType* type) override;
    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;

    ErrCode INTERFACE_FUNC verifyParameters() override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    ErrCode verifyParametersInternal() const;
    static DictPtr<IString, IBaseObject> PackBuilder(IScalingBuilder* scalingBuilder);

    ScaledSampleType outputDataType;
    SampleType inputDataType;
    ScalingType ruleType;
    DictPtr<IString, IBaseObject> params;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ScalingImpl)

END_NAMESPACE_OPENDAQ
