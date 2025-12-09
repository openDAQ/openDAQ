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
#include <queue>
#include <string>
#include <thread>

#include <coretypes/filesystem.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

class MultiCsvWriter
{
public:
    MultiCsvWriter(const fs::path& file);
    ~MultiCsvWriter();

    void setHeaderInformation(const DataDescriptorPtr& domainDescriptor,
                              const ListPtr<IDataDescriptor>& valueDescriptors,
                              const ListPtr<IString>& signalNames);
    void writeSamples(std::vector<std::unique_ptr<double[]>>&& jaggedArray, int count, Int packetOfset);

private:
    struct JaggedBuffer
    {
        int count;
        std::vector<std::unique_ptr<double[]>> buffers;
        Int packetOffset;
    };

    struct DomainMetadata
    {
        std::string origin;
        std::string unitName;
        std::pair<std::int64_t, std::int64_t> tickResolution;  // {numerator, denominator}
        std::int64_t ruleStart;
        std::int64_t ruleDelta;
        std::int64_t referenceDomainOffset;
        TimeProtocol referenceDomainTimeProtocol;
    };

    void threadLoop();

    void writeHeaders(Int firstPacketOffset);
    DomainMetadata getDomainMetadata(const DataDescriptorPtr& domainDescriptor);
    std::string getMetadataHeader(const DomainMetadata& metadata);

    std::mutex mutex;
    std::condition_variable cv;

    bool exitFlag;
    bool headersWritten;
    std::queue<JaggedBuffer> queue;

    fs::path filepath;
    std::ofstream outFile;

    std::string domainName;
    std::vector<std::string> valueNames;

    std::string domainMetadata;
    DomainMetadata metadata;

    std::thread writerThread;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE