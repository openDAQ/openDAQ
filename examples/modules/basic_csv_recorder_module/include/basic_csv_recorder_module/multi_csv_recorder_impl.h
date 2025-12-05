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

#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <thread>

#include <coretypes/filesystem.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/multi_csv_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

/*!
 * @brief A basic recorder function block which records data from its input signals into a CSV file.
 */
class MultiCsvRecorderImpl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
{
public:
    static constexpr const char* TYPE_ID = "MultiCsvRecorder";

    struct Tags
    {
        static constexpr const char* RECORDER = "Recorder";
    };

    struct Props
    {
        static constexpr const char* PATH = "Path";
    };

    /*!
     * @brief Creates a new function block.
     * @param context The openDAQ context object.
     * @param parent The component object which will contain this function block.
     * @param localId The local identifier of this function block.
     * @param config A property object containing configuration data for this function block.
     */
    MultiCsvRecorderImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);

    ~MultiCsvRecorderImpl() = default;

    static FunctionBlockTypePtr createType();

    ErrCode INTERFACE_FUNC startRecording() override;
    ErrCode INTERFACE_FUNC stopRecording() override;
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;

protected:
    virtual void activeChanged() override;

private:
    void initProperties();

    std::string getNextPortID() const;

    bool updateInputPorts();
    void updateReader();
    void configure(const DataDescriptorPtr& domainDescriptor,
                   const ListPtr<IDataDescriptor>& valueDescriptors,
                   const ListPtr<IString>& signalNames);
    void reconfigure();
    void onPathChanged();

    void onConnected(const InputPortPtr& inputPort) override;
    void onDisconnected(const InputPortPtr& inputPort) override;
    void onDataReceived();

    std::vector<InputPortPtr> connectedPorts;
    InputPortPtr disconnectedPort;

    std::unordered_map<std::string, DataDescriptorPtr> cachedDescriptors;
    std::unordered_map<std::string, StringPtr> cachedSignalNames;
    DataDescriptorPtr recorderDomainDataDescriptor;

    PacketReadyNotification notificationMode;
    MultiReaderPtr reader;

    bool recordingActive = false;
    std::optional<fs::path> filePath = std::nullopt;
    std::optional<MultiCsvWriter> writer = std::nullopt;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
