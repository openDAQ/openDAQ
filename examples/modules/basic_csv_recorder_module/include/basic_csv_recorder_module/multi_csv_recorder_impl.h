/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <coretypes/filesystem.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/multi_reader2_factory.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/multi_csv_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

/*!
 * @brief A basic recorder function block which records aligned data from its input signals into a CSV file.
 *
 * The block always offers one free input port outside the reader, with the block as its listener. When a signal
 * connects the port joins the reader, which judges the descriptor through the block's `accepts`, and the next
 * port is offered; a port that loses its signal is removed. The reader is the only listener of the ports it
 * reads and the block reads from its onDataAvailable event until drained.
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
        static constexpr const char* DIR = "Directory";
        static constexpr const char* BASENAME = "Basename";
        static constexpr const char* FILE_TIMESTAMP_ENABLED = "FileTimestampEnabled";
        static constexpr const char* WRITE_DOMAIN = "WriteDomain";
    };

    MultiCsvRecorderImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~MultiCsvRecorderImpl() = default;

    static FunctionBlockTypePtr createType();

    ErrCode INTERFACE_FUNC startRecording() override;
    ErrCode INTERFACE_FUNC stopRecording() override;
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;

protected:
    void activeChanged() override;
    void removed() override;
    void onConnected(const InputPortPtr& port) override;

private:
    void initProperties();
    void onPropertiesChanged();

    void addFreePort();
    ListPtr<IComponent> readerPorts() const;
    void createReader();
    void drain();
    bool onChanges(const MultiReader2StatusPtr& status);
    static bool accepts(const DataDescriptorPtr& descriptor);
    void writeSamples(SizeT count, SizeT packetOffset);

    /**
     * @brief Opens a new CSV writer from the cached descriptors; fails into a warning when something is missing.
     */
    void configureWriter();
    void stopRecordingInternal(bool recover);
    void startRecordingInternal();

    InputPortConfigPtr freePort;
    int nextPortId = 1;

    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    std::vector<std::vector<double>> storage;
    std::vector<void*> buffers;
    std::vector<ComponentPtr> slotInputs;      // slot order as of the last status
    std::vector<bool> activeSlots;             // per slot: healthy as of the last status
    std::unordered_map<std::string, DataDescriptorPtr> cachedDescriptors;
    std::unordered_map<std::string, StringPtr> cachedSignalNames;
    DataDescriptorPtr recorderDomainDataDescriptor;

    bool recordingActive = false;
    bool recoverToActive = false;

    std::optional<fs::path> filePath = std::nullopt;
    std::string fileBasename;
    bool timestampEnabled = true;
    bool writeDomain = false;

    std::optional<MultiCsvWriter> writer = std::nullopt;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
