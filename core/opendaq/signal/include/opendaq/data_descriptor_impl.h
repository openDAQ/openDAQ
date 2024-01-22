
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
#include <coretypes/listobject_factory.h>
#include <coretypes/struct_impl.h>
#include <opendaq/data_descriptor_builder_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_rule_calc.h>
#include <opendaq/data_rule_calc_private.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/scaling_calc.h>
#include <opendaq/scaling_calc_private.h>

BEGIN_NAMESPACE_OPENDAQ

class DataDescriptorImpl : public GenericStructImpl<IDataDescriptor, IStruct, IScalingCalcPrivate, IDataRuleCalcPrivate>
{
public:
    explicit DataDescriptorImpl(IDataDescriptorBuilder* dataDescriptorBuilder);

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDimensions(IList** dimensions) override;
    ErrCode INTERFACE_FUNC getSampleType(SampleType* sampleType) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;
    ErrCode INTERFACE_FUNC getValueRange(IRange** range) override;
    ErrCode INTERFACE_FUNC getRule(IDataRule** rule) override;
    ErrCode INTERFACE_FUNC getOrigin(IString** origin) override;
    ErrCode INTERFACE_FUNC getTickResolution(IRatio** resolution) override;
    ErrCode INTERFACE_FUNC getPostScaling(IScaling** scaling) override;
    ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) override;
    ErrCode INTERFACE_FUNC getStructFields(IList** structFields) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    // IScalingCalcPrivate
    void* INTERFACE_FUNC scaleData(void* data, SizeT sampleCount) const override;
    void INTERFACE_FUNC scaleData(void* data, SizeT sampleCount, void** output) const override;
    bool INTERFACE_FUNC hasScalingCalc() const override;

    // IDataRuleCalcPrivate
    void* INTERFACE_FUNC calculateRule(const NumberPtr& packetOffset, SizeT sampleCount) const override;
    void INTERFACE_FUNC calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void** output) const override;
    bool INTERFACE_FUNC hasDataRuleCalc() const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

protected:
    ListPtr<IDimension> dimensions;
    StringPtr name;

    SampleType sampleType;
    UnitPtr unit;
    RangePtr valueRange;
    DataRulePtr dataRule;
    ScalingPtr scaling;
    StringPtr origin;
    RatioPtr resolution;

    ListPtr<IDataDescriptor> structFields;

    DictPtr<IString, IString> metadata;

private:
    ErrCode validate();
    void initCalcs();
    std::unique_ptr<ScalingCalc> scalingCalc;
    std::unique_ptr<DataRuleCalc> dataRuleCalc;

    static DictPtr<IString, IBaseObject> PackBuilder(IDataDescriptorBuilder* dataDescriptorBuilder);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DataDescriptorImpl)

END_NAMESPACE_OPENDAQ
