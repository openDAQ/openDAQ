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
#include <memory>

#include "websocket_streaming/websocket_streaming.h"
#include "websocket_streaming/signal_info.h"
#include <opendaq/data_rule_ptr.h>
#include <opendaq/sample_type.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_ptr.h>
#include "streaming_protocol/SubscribedSignal.hpp"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class SignalDescriptorConverter
{
public:
    /**
     *  @param subscribedSignal The object holding everything about thee signal on the consumer side
     *  @throws ConversionFailedException
     */
    static SubscribedSignalInfo ToDataDescriptor(const daq::streaming_protocol::SubscribedSignal& subscribedSignal);
    /**
     *  @throws ConversionFailedException
     */
    static void ToStreamedSignal(const daq::SignalPtr& signal, daq::streaming_protocol::BaseSignalPtr stream, const SignalProps& sigProps);

private:
    static daq::DataRulePtr GetRule(const daq::streaming_protocol::SubscribedSignal& subscribedSignal);
    static void SetTimeRule(const daq::DataRulePtr& rule, daq::streaming_protocol::BaseSignalPtr signal);
    static daq::SampleType Convert(daq::streaming_protocol::SampleType dataType);
    static daq::streaming_protocol::SampleType Convert(daq::SampleType sampleType);
    static void EncodeInterpretationObject(const DataDescriptorPtr& dataDescriptor, nlohmann::json& extra);
    static void DecodeInterpretationObject(const nlohmann::json& extra, DataDescriptorBuilderPtr& dataDescriptor);
    static nlohmann::json DictToJson(const DictPtr<IString, IBaseObject>& dict);
    static DictPtr<IString, IBaseObject> JsonToDict(const nlohmann::json& json);
};
END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
