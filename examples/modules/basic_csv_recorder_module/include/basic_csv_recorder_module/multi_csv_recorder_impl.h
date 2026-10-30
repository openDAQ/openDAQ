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

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <coretypes/filesystem.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/multi_csv_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

/*!
 * @brief A recorder function block which records its equal-rate input signals, aligned by a multi reader, into a CSV file.
 * It always offers one free port; an input that is not scalar numeric, or that the reader cannot align, is set aside
 * and recorded again once healthy, and a port that disconnects is removed. The file gets a new header, in a new file,
 * when the domain or the set of recorded inputs changes.
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

    static FunctionBlockTypePtr createType();

    ErrCode INTERFACE_FUNC startRecording() override;
    ErrCode INTERFACE_FUNC stopRecording() override;
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;

protected:
    void activeChanged() override;
    void removed() override;

private:
    void initProperties();
    void onPropertiesChanged();

    // The MultiReader2 block pattern (ref_fb_module's ReaderFbBase); the recorder keeps its own copy, as it lives in
    // another module
    void createReader();
    void drain();
    void onChanges(const MultiReader2StatusPtr& status);
    void addFreePort();
    void onConnected(const InputPortPtr& port) override;
    ListPtr<IComponent> readerPorts() const;

    // Opens the next file with the header of the recorded inputs
    void configureWriter();
    void stopRecordingInternal(bool recover);
    void startRecordingInternal();

    MultiReader2ParamsPtr params;
    MultiReader2Ptr reader;
    InputPortConfigPtr freePort;
    int nextPortId = 1;

    std::vector<std::string> recorded;  // the contributing inputs in slot order: the CSV columns
    std::unordered_map<std::string, DataDescriptorPtr> cached;
    std::unordered_map<std::string, StringPtr> names;
    DataDescriptorPtr domain;

    bool recordingActive = false;
    bool recoverToActive = false;

    std::optional<fs::path> filePath = std::nullopt;
    std::string fileBasename;
    bool timestampEnabled;
    bool writeDomain = false;

    std::optional<MultiCsvWriter> writer = std::nullopt;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
