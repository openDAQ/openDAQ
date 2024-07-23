/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <algorithm>
#include <file_writer_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_config_ptr.h>

#include "opendaq/data_packet_ptr.h"
#include "opendaq/event_packet_ptr.h"

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>

BEGIN_NAMESPACE_FILE_WRITER_MODULE

namespace FileWriter
{

struct DataTable
{
    DataTable(std::string domainId, int tableNr)
    : schemaBuilder()
    , domainBuilder()
    , tableNr(tableNr)
    , batchCount(0)
    , empty(true)
    , batchCylceReached(0)
    {
        PARQUET_THROW_NOT_OK(schemaBuilder.AddField(arrow::field(domainId, arrow::int64())));
    }
    arrow::SchemaBuilder schemaBuilder;
    arrow::Int64Builder domainBuilder;
    int tableNr;
    int batchCount;
    bool empty;
    int64_t batchCylceReached;

    std::map<std::string, arrow::DoubleBuilder> dataBuilderMap;
    

};

class ParquetFbImpl final : public FunctionBlock
{
public:
    explicit ParquetFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~ParquetFbImpl() override = default;

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

    static FunctionBlockTypePtr CreateType();


private:

    std::map<std::string, DataTable> dataTablesMap;
    
    int inputPortCount;
    int tableNumberCount;
    bool recordingActive;
    std::string path;
    std::string fileName;
    int64_t writeBatchCylceInSec;  


    void updateInputPorts();
    void writeParquetFile(const arrow::Table& table, const int tableWriteCount, const int tableWriteSubCount);
    std::shared_ptr<arrow::Table> generateTable(DataTable& dataTable);

    template <SampleType InputSampleType>
    void processDataPacket(const std::string& globalId, const std::string& domainGlobalId, DataPacketPtr&& packet, ListPtr<IPacket>& outQueue, ListPtr<IPacket>& outDomainQueue);
    void onPacketReceived(const InputPortPtr& port) override;


    void initProperties();
    void propertyChanged(bool configure);
    void activeChanged();
    void configureBatchCyleInSecChanged();

    void readProperties();

    void initStatuses();
    void setInputStatus(const StringPtr& value);

};

}

END_NAMESPACE_FILE_WRITER_MODULE
