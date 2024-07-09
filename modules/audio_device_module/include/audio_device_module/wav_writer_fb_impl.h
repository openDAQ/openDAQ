/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/function_block_impl.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/stream_reader_ptr.h>
#include <miniaudio/miniaudio.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class WAVWriterFbImpl final : public FunctionBlock
{
public:
    explicit WAVWriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~WAVWriterFbImpl() override;

    static FunctionBlockTypePtr CreateType();
private:
    InputPortConfigPtr inputPort;
    std::string fileName;
    bool storing;
    ma_encoder encoder;
    DataDescriptorPtr inputValueDataDescriptor;
    DataDescriptorPtr inputTimeDataDescriptor;
    StreamReaderPtr reader;
    bool selfChange;

    bool validateDataDescriptor() const;
    bool validateDomainDescriptor() const;
    bool initializeEncoder();

    void initProperties();
    void createInputPort();

    void fileNameChanged();
    void storingChanged(bool store);
    void storingChangedNoLock(bool store);
    void startStore();
    void stopStore();
    void stopStoreInternal();
    void processEventPacket(const EventPacketPtr& packet);
    void calculate();
};

END_NAMESPACE_AUDIO_DEVICE_MODULE
