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
#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/streaming.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/object_keys.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/streaming_private.h>
#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/ids_parser.h>
#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/connection_status_container_private_ptr.h>
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming_to_device.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class StreamingToDeviceImpl;

using StreamingToDevice = StreamingToDeviceImpl<>;

template <typename... Interfaces>
class StreamingToDeviceImpl : public StreamingImpl<IStreamingToDevice, Interfaces...>
{
public:
    using Super = StreamingImpl<IStreamingToDevice, Interfaces...>;
    using Self = StreamingToDeviceImpl<Interfaces...>;

    explicit StreamingToDeviceImpl(const StringPtr& connectionString, const ContextPtr& context, bool skipDomainSignalSubscribe);

    ~StreamingToDeviceImpl() override;

    // IStreamingToDevice
    ErrCode INTERFACE_FUNC test() override;

protected:
    virtual void onTest() = 0;
};

template<typename... Interfaces>
StreamingToDeviceImpl<Interfaces...>::StreamingToDeviceImpl(const StringPtr& connectionString,
                                                            const ContextPtr& context,
                                                            bool skipDomainSignalSubscribe)
    : Super(connectionString, context, skipDomainSignalSubscribe)
{
}

template<typename... Interfaces>
StreamingToDeviceImpl<Interfaces...>::~StreamingToDeviceImpl()
{
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::test()
{
    onTest();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
