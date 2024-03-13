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
#include <audio_device_module/common.h>
#include <audio_device_module/miniaudio_utils.h>
#include <opendaq/channel_ptr.h>
#include <opendaq/device_impl.h>
#include <miniaudio/miniaudio.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class AudioDeviceImpl final : public Device
{
public:
    explicit AudioDeviceImpl(const std::shared_ptr<MiniaudioContext>& maContext, const ma_device_id& id, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~AudioDeviceImpl() override;

    static DeviceInfoPtr CreateDeviceInfo(const std::shared_ptr<MiniaudioContext>& maContext, const ma_device_info& deviceInfo);
    static ma_device_id getIdFromConnectionString(std::string connectionString);
    static std::string getConnectionStringFromId(ma_backend backend, ma_device_id id);
    static DeviceTypePtr createType();

    // IDevice
    void onSetDeviceInfo() override;

    // IDeviceDomain
    RatioPtr onGetResolution() override;
    uint64_t onGetTicksSinceOrigin() override;
    std::string onGetOrigin() override;
    UnitPtr onGetDomainUnit() override;

    void addData(const void* data, size_t sampleCount);

private:
    ChannelPtr channel;
    ma_device maDevice;
    ma_device_id maId;
    std::shared_ptr<MiniaudioContext> maContext;
    bool started;
    SignalConfigPtr timeSignal;
    uint32_t sampleRate;
    Int samplesCaptured;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    void initProperties();
    void start();
    void stop();
    void readProperties();
    void createAudioChannel();
    void propertyChanged();
    void configureTimeSignal();
    void configure();
};

END_NAMESPACE_AUDIO_DEVICE_MODULE
