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
#include <miniaudio/miniaudio.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/stream_reader_ptr.h>


BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class WAVReaderFbImpl : public FunctionBlock
{
public:
    explicit WAVReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~WAVReaderFbImpl() override;

    static FunctionBlockTypePtr CreateType();


private:
    bool initializeDecoder();
    bool uninitializeDecoder();
    bool decoderReady();
    bool decoderReading();

    void initProperties();
    bool initializeSignal();

    bool updateFilePath(const std::string& newPath);
    void setRead(bool read);

    void startRead();
    void stopRead();

    DataPacketPtr buildPacket(const void* data, size_t sampleCount);
    void sendPacket(DataPacketPtr packet);

    static void miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

public:
    ma_decoder decoder;

private:
    std::string filePath;

    ma_device device;

    SignalConfigPtr timeSignal;
    SignalConfigPtr outputSignal;

    DataDescriptorPtr domainDescriptor;
    DataDescriptorPtr dataDescriptor;

    Int samplesCaptured;

    bool decoderInitialized;
    bool reading;
    std::atomic<bool> framesAvailable;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_AUDIO_DEVICE_MODULE

