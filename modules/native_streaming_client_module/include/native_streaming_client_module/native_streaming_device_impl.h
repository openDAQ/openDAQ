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
#include <native_streaming_client_module/common.h>
#include <opendaq/device_impl.h>
#include <opendaq/streaming_ptr.h>


BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

class NativeStreamingDeviceImpl : public Device
{
public:
    explicit NativeStreamingDeviceImpl(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const StringPtr& connectionString,
                                       const StringPtr& host,
                                       const StringPtr& port,
                                       const StringPtr& path);

protected:
    DeviceInfoPtr onGetInfo() override;

    void updateSignalProperties(const SignalPtr& signal,
                                const StringPtr& name,
                                const StringPtr& description);
    void signalAvailableHandler(const StringPtr& signalStringId,
                                const StringPtr &domainSignalStringId,
                                const DataDescriptorPtr& signalDescriptor,
                                const StringPtr& name,
                                const StringPtr& description);
    void signalUnavailableHandler(const StringPtr& signalStringId);
    void createNativeStreaming(const StringPtr& host, const StringPtr& port, const StringPtr& path);
    void activateStreaming();

    StringPtr connectionString;
    StreamingPtr nativeStreaming;
    std::unordered_map<StringPtr, std::pair<SignalPtr, StringPtr>, StringHash, StringEqualTo> deviceSignals;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
