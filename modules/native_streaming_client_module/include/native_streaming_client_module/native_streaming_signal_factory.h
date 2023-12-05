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
#include <native_streaming_client_module/native_streaming_signal_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

inline SignalPtr NativeStreamingSignal(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const DataDescriptorPtr& descriptor,
                                       const StringPtr& streamingId)
{
    SignalPtr obj(createWithImplementation<ISignal, NativeStreamingSignalImpl>(ctx,
                                                                              parent,
                                                                              descriptor,
                                                                              streamingId));
    return obj;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
