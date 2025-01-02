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
#include <audio_device_module/common.h>
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/sample_type.h>
#include <optional>
#include <random>
#include <miniaudio/miniaudio.h>
#include <opendaq/data_packet_ptr.h>
#include <mutex>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

DECLARE_OPENDAQ_INTERFACE(IAudioChannel, IBaseObject)
{
    virtual void configure(const ma_device& device, const SignalPtr& timeSignal) = 0;
    virtual void addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount) = 0;
};

class AudioChannelImpl final : public ChannelImpl<IAudioChannel>
{
public:
    explicit AudioChannelImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~AudioChannelImpl() override;

    void configure(const ma_device& device, const SignalPtr& timeSignal) override;
    void addData(const DataPacketPtr& domainPacket, const void* data, size_t sampleCount) override;

private:
    SignalConfigPtr outputSignal;
};

END_NAMESPACE_AUDIO_DEVICE_MODULE
