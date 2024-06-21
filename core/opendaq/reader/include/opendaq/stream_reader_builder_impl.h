/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/stream_reader_builder.h>
#include <opendaq/stream_reader_ptr.h>
#include <opendaq/input_port_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class StreamReaderBuilderImpl : public ImplementationOf<IStreamReaderBuilder>
{
public:
    StreamReaderBuilderImpl();

    ErrCode INTERFACE_FUNC build(IStreamReader** streamReader) override;

    ErrCode INTERFACE_FUNC setOldStreamReader(IStreamReader* streamReader) override;
    ErrCode INTERFACE_FUNC getOldStreamReader(IStreamReader** streamReader) override;

    ErrCode INTERFACE_FUNC setSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;

    ErrCode INTERFACE_FUNC setInputPort(IInputPort* port) override;
    ErrCode INTERFACE_FUNC getInputPort(IInputPort** ports) override;

    ErrCode INTERFACE_FUNC setValueReadType(SampleType valueReadType) override;
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* valueReadType) override;

    ErrCode INTERFACE_FUNC setDomainReadType(SampleType domainReadType) override;
    ErrCode INTERFACE_FUNC getDomainReadType(SampleType* domainReadType) override;

    ErrCode INTERFACE_FUNC setReadTimeoutType(ReadTimeoutType type) override;
    ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* type) override;

    ErrCode INTERFACE_FUNC setReadMode(ReadMode mode) override;
    ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) override;

    ErrCode INTERFACE_FUNC setSkipEvents(Bool skipEvents) override;
    ErrCode INTERFACE_FUNC getSkipEvents(Bool* skipEvents) override;

private:
    SampleType valueReadType;
    SampleType domainReadType;
    ReadMode readMode;
    SignalPtr signal;
    InputPortPtr inputPort;
    StreamReaderPtr oldStreamReader;
    ReadTimeoutType readTimeoutType;
    bool used;
    bool skipEvents;
};

END_NAMESPACE_OPENDAQ