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
#include <opendaq/function_block_impl.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/stream_reader_ptr.h>
#include <miniaudio/miniaudio.h>
#include <opendaq/reader_status_ptr.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class WAVWriterFbImpl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
{
public:
    explicit WAVWriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~WAVWriterFbImpl() override;

    static FunctionBlockTypePtr CreateType();
private:
    InputPortConfigPtr inputPort;
    std::string fileName;
    bool recording;
    ma_encoder encoder;
    DataDescriptorPtr inputValueDataDescriptor;
    DataDescriptorPtr inputTimeDataDescriptor;
    StreamReaderPtr reader;

    /*!
     * @brief Starts the recording, if it was not already started.
     * @return OPENDAQ_SUCCESS.
     */
    ErrCode INTERFACE_FUNC startRecording() override;

    /*!
     * @brief Stops the recording, if it was started.
     * @return OPENDAQ_SUCCESS.
     */
    ErrCode INTERFACE_FUNC stopRecording() override;

    /*!
     * @brief Checks whether data from connected signals is currently being recorded to the
     *     persistent storage medium.
     * @param isRecording A pointer to a boolean which is populated with the recording state.
     * @retval OPENDAQ_SUCCESS if the recording state was successfully stored.
     * @retval OPENDAQ_ERR_ARGUMENT_NULL if @p isRecording is `nullptr`.
     */
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;

    bool validateDataDescriptor() const;
    bool validateDomainDescriptor() const;
    bool initializeEncoder();

    void initProperties();
    void createInputPort();

    void fileNameChanged();

    void processEventPacket(const EventPacketPtr& packet);
    void processInputData();
};

END_NAMESPACE_AUDIO_DEVICE_MODULE
