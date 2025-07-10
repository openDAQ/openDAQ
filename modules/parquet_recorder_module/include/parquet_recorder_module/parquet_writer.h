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

#include <opendaq/opendaq.h>
#include <coretypes/filesystem.h>

#include <parquet_recorder_module/common.h>

namespace parquet
{
    namespace arrow
    {
        class FileWriter;
    }
}

namespace arrow
{
    class Schema;
    namespace io
    {
        class FileOutputStream;
    }
}

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

class ParquetWriter
{
public:
    static constexpr const size_t PACKET_BUFFER_SIZE_TO_WRITE = 10;

    ParquetWriter(fs::path path, SignalPtr signal, daq::LoggerComponentPtr logger_component, daq::SchedulerPtr scheduler);
    ~ParquetWriter();

    void enqueuePacketList(ListPtr<IPacket>& packets);

private:
    fs::path path;
    SignalPtr signal;
    daq::LoggerComponentPtr loggerComponent;
    daq::SchedulerPtr scheduler;
    std::string filename;

    std::mutex mutex;
    std::shared_ptr<arrow::Schema> schema;
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    std::unique_ptr<parquet::arrow::FileWriter> writer;
    DataDescriptorPtr currentDataDescriptor;
    DataDescriptorPtr currentDomainDescriptor;

    std::vector<PacketPtr> packetBuffer;
    std::mutex packetBufferMutex;

    std::vector<PacketPtr> dequeuePacketList();
    void processPacketList(const std::vector<PacketPtr>& packets);

    void onPacket(const PacketPtr& packet);
    void onDataPacket(const DataPacketPtr& packet);
    void onEventPacket(const EventPacketPtr& packet);

    void configure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor);
    void reconfigure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor);
    void generateMetadata(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor);
    void openFile();
    void closeFile();

    template <typename TDataType, typename TDomainType>
    void writePackets(const DataPacketPtr& data, const DataPacketPtr& domain);
    template <typename TDataType>
    void writePackets(const DataPacketPtr& data, const DataPacketPtr& domain);
    void writePackets(const DataPacketPtr& data, const DataPacketPtr& domain);
};

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE