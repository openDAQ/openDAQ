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
#include <opendaq/multi_reader_builder.h>
#include <opendaq/input_port_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class MultiReaderBuilderImpl : public ImplementationOf<IMultiReaderBuilder>
{
public:
    MultiReaderBuilderImpl();

    ErrCode INTERFACE_FUNC build(IMultiReader** multiReader) override;

    ErrCode INTERFACE_FUNC addSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC addInputPort(IInputPort* port) override;
    ErrCode INTERFACE_FUNC getSourceComponents(IList** ports) override;
   
    ErrCode INTERFACE_FUNC setValueReadType(SampleType type) override;
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* type) override;

    ErrCode INTERFACE_FUNC setDomainReadType(SampleType type) override;
    ErrCode INTERFACE_FUNC getDomainReadType(SampleType* type) override;

    ErrCode INTERFACE_FUNC setReadMode(ReadMode mode) override;
    ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) override;

    ErrCode INTERFACE_FUNC setReadTimeoutType(ReadTimeoutType type) override;
    ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* type) override;

    ErrCode INTERFACE_FUNC setRequiredCommonSampleRate(Int sampleRate) override;
    ErrCode INTERFACE_FUNC getRequiredCommonSampleRate(Int* sampleRate) override;

    ErrCode INTERFACE_FUNC setStartOnFullUnitOfDomain(Bool enabled) override;
    ErrCode INTERFACE_FUNC getStartOnFullUnitOfDomain(Bool* enabled) override;

private:
    ListPtr<IComponent> sources;
    SampleType valueReadType;
    SampleType domainReadType;
    ReadMode readMode;
    ReadTimeoutType readTimeoutType;
    Int requiredCommonSampleRate;
    Bool startOnFullUnitOfDomain;
};

END_NAMESPACE_OPENDAQ
