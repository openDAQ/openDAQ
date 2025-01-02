
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
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/data_descriptor_builder_ptr.h>
#include <opendaq/data_rule_calc.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/scaling_calc.h>
#include <opendaq/reference_domain_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class DataDescriptorBuilderImpl : public ImplementationOf<IDataDescriptorBuilder>
{
public:
    explicit DataDescriptorBuilderImpl();
    explicit DataDescriptorBuilderImpl(const DataDescriptorPtr& descriptorCopy);

    ErrCode INTERFACE_FUNC build(IDataDescriptor** dataDescriptor) override;

    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;

    ErrCode INTERFACE_FUNC setDimensions(IList* dimensions) override;
    ErrCode INTERFACE_FUNC getDimensions(IList** dimensions) override;

    ErrCode INTERFACE_FUNC setSampleType(SampleType sampleType) override;
    ErrCode INTERFACE_FUNC getSampleType(SampleType* sampleType) override;

    ErrCode INTERFACE_FUNC setUnit(IUnit* unit) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;

    ErrCode INTERFACE_FUNC setValueRange(IRange* range) override;
    ErrCode INTERFACE_FUNC getValueRange(IRange** range) override;

    ErrCode INTERFACE_FUNC setRule(IDataRule* rule) override;
    ErrCode INTERFACE_FUNC getRule(IDataRule** rule) override;

    ErrCode INTERFACE_FUNC setOrigin(IString* origin) override;
    ErrCode INTERFACE_FUNC getOrigin(IString** origin) override;

    ErrCode INTERFACE_FUNC setTickResolution(IRatio* tickResolution) override;
    ErrCode INTERFACE_FUNC getTickResolution(IRatio** tickResolution) override;

    ErrCode INTERFACE_FUNC setPostScaling(IScaling* scaling) override;
    ErrCode INTERFACE_FUNC getPostScaling(IScaling** scaling) override;

    ErrCode INTERFACE_FUNC setMetadata(IDict* metadata) override;
    ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) override;

    ErrCode INTERFACE_FUNC setStructFields(IList* structFields) override;
    ErrCode INTERFACE_FUNC getStructFields(IList** structFields) override;

    ErrCode INTERFACE_FUNC setReferenceDomainInfo(IReferenceDomainInfo* referenceDomainInfo) override;
    ErrCode INTERFACE_FUNC getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo) override;

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
    ReferenceDomainInfoPtr referenceDomainInfo;
};

END_NAMESPACE_OPENDAQ
