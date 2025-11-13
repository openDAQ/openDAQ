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
#include <audio_device_module/miniaudio_utils.h>
#include <opendaq/channel_ptr.h>
#include <opendaq/device_impl.h>
#include <miniaudio/miniaudio.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class AudioDeviceImpl final : public Device
{
public:
    explicit AudioDeviceImpl(const std::shared_ptr<ma_utils::MiniaudioContext>& maContext,
                             const ma_device_id& id,
                             const ContextPtr& ctx,
                             const ComponentPtr& parent,
                             const StringPtr& localId);
    ~AudioDeviceImpl() override;
    
    static DeviceTypePtr createType();
    static DeviceInfoPtr CreateDeviceInfo(const std::shared_ptr<ma_utils::MiniaudioContext>& maContext, const ma_device_info& deviceInfo);

protected:
    uint64_t onGetTicksSinceOrigin() override;

private:
    ma_device maDevice;
    ma_device_id maId;
    std::shared_ptr<ma_utils::MiniaudioContext> maContext;
    
    ChannelPtr channel;
    bool started;
    uint32_t sampleRate;

    void initProperties();
    void createAudioChannel();
    void setDeviceInfo();
    
    void sampleRateChanged(uint32_t sampleRate);
    void start();
    void stop();

};

END_NAMESPACE_AUDIO_DEVICE_MODULE
