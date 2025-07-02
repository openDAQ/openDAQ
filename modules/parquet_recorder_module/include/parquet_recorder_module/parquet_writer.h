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

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <condition_variable>

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

using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

class ParquetWriter
{
public:
    ParquetWriter(fs::path path, SignalPtr signal, daq::LoggerComponentPtr logger_component, daq::SchedulerPtr scheduler);
    ~ParquetWriter();

    void onPacketList(const ListPtr<PacketPtr>& packets, bool active, bool recording, uint64_t taskId);
    void onPacket(const PacketPtr& packet, bool active, bool recording, uint64_t taskId);
    void setClosing(bool value);
    bool getClosing() const;
    bool getHasRunningTask() const;
    void setHasRunningTask(bool value);
    void waitForNoRunningTask();

private:
    fs::path path;
    SignalPtr signal;
    daq::LoggerComponentPtr loggerComponent;
    daq::SchedulerPtr scheduler;

    std::string filename;

    std::shared_ptr<arrow::Schema> schema;
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    std::unique_ptr<parquet::arrow::FileWriter> writer;

    DataDescriptorPtr currentDataDescriptor;
    DataDescriptorPtr currentDomainDescriptor;

    std::mutex dataMutex;

    std::atomic_bool closing = false;

    std::mutex closingMutex;

    std::atomic_bool hasRunningTask = false;
    std::condition_variable noRunningTaskCondition;

    void onDataPacket(const DataPacketPtr& packet, uint64_t taskId);
    void onEventPacket(const EventPacketPtr& packet, uint64_t taskId);
    void generateMetadata(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor);
    void openFile();
    void closeFile();
    void configure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor);
    void reconfigure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor, uint64_t taskId = 0);
    template <typename DataType, typename DomainType>
    void writePackets(const DataPacketPtr& data, const DataPacketPtr& domain, uint64_t taskId);
    template <typename DataType>
    void writePackets(const DataPacketPtr& data, const DataPacketPtr& domain, uint64_t taskId);
    void writePackets(const DataPacketPtr& data, const DataPacketPtr& domain, uint64_t taskId);
};

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE