#include <ctime>
#include <iostream>

#include <file_writer_module/parquet_fb_impl.h>
#include <file_writer_module/dispatch.h>
#include <file_writer_module/builder_type_transit.h>

#include <coreobjects/unit_factory.h>
#include <coreobjects/eval_value_factory.h>

#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/data_packet.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <coreobjects/validator_factory.h>


BEGIN_NAMESPACE_FILE_WRITER_MODULE

namespace FileWriter
{

ParquetFbImpl::ParquetFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , inputPortCount(0)
    , tableNumberCount(0)
    , recordingActive(false)
    , downScalingDevider(1)
{
    initProperties();
    createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);
}

void ParquetFbImpl::initProperties()
{
    path = "";
    const auto pathProp = StringProperty("Path", path);
    objPtr.addProperty(pathProp);
    objPtr.getOnPropertyValueWrite("Path") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto fileNameProp = StringProperty("FileName", "ExampleFile");
    objPtr.addProperty(fileNameProp);
    objPtr.getOnPropertyValueWrite("FileName") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto recordingActiveProp = BoolProperty("RecordingActive", false);
    objPtr.addProperty(recordingActiveProp);
    objPtr.getOnPropertyValueWrite("RecordingActive") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyActiveChanged(); };

    const auto batchCylce = IntPropertyBuilder("BatchCylce", 20)
                            .setUnit(Unit("s"))
                            .setMinValue(1)
                            .setMaxValue(3600)
                            .setDescription("After how many seconds a batch is stored.")
                            .build();
    objPtr.addProperty(batchCylce);
    objPtr.getOnPropertyValueWrite("BatchCylce") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };


    const auto downScalingDevider = IntPropertyBuilder("DownScalingDevider", 1)
                            .setMinValue(1)
                            .setMaxValue(1000000)
                            .setDescription("Only every nth value is stored to the file.")
                            //.setValidator(Validator("$RecordingActive == False"))
                            .build();
    objPtr.addProperty(downScalingDevider);
    objPtr.getOnPropertyValueWrite("DownScalingDevider") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    readProperties();
}

void ParquetFbImpl::readProperties()
{
    path = static_cast<std::string>(objPtr.getPropertyValue("Path"));

    if (!path.empty())
    {
        if (path.compare(path.length() -1, 1,"/"))
        {
            path.append("/");
        }
    }

    fileName = static_cast<std::string>(objPtr.getPropertyValue("FileName"));
    recordingActive = static_cast<bool>(objPtr.getPropertyValue("RecordingActive"));
    batchCycle = static_cast<int>(objPtr.getPropertyValue("BatchCylce"));
    downScalingDevider = static_cast<int>(objPtr.getPropertyValue("DownScalingDevider"));
}

void ParquetFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
    if (configure)
        configureBatchCyleInSecChanged();
}

void ParquetFbImpl::propertyActiveChanged()
{
    std::scoped_lock lock(sync);
    readProperties();
    if (!recordingActive)
    {
        for (auto& dataTable : dataTablesMap)
        {
            if (!dataTable.second.empty)
            {
                std::shared_ptr<arrow::Table> table = generateTable(dataTable.second);
                writeParquetFile(*table, dataTable.second.tableNr, dataTable.second.batchCount);
            }
        }
    } 
    else
    {
        configureBatchCyleInSecChanged();
    }
}

void ParquetFbImpl::configureBatchCyleInSecChanged()
{
    for (auto& dataTable : dataTablesMap)
    {
        time_t currentTime;
        time(&currentTime);
        dataTable.second.batchCylceReached = currentTime + batchCycle;
    }
}

std::shared_ptr<arrow::Table> ParquetFbImpl::generateTable(DataTable& dataTable) 
{
    arrow::ArrayVector dataVector;
    std::shared_ptr<arrow::Array> int64Array;

    PARQUET_THROW_NOT_OK(dataTable.domainBuilder.Finish(&int64Array));
    dataTable.domainBuilder.Reset();
    dataVector.emplace_back(int64Array);

    // Iterate and print key-value pairs
    for (auto& entry : dataTable.dataBuilderMap) 
    {
        std::shared_ptr<arrow::Array> doubleArray;
        PARQUET_THROW_NOT_OK(entry.second.get()->Finish(&doubleArray));
        dataVector.emplace_back(doubleArray);
        entry.second.get()->Reset();
    }

    dataTable.empty = true;
    auto schema = dataTable.schemaBuilder.Finish();

    return arrow::Table::Make(schema.ValueOrDie(), dataVector);
}

void ParquetFbImpl::writeParquetFile(const arrow::Table& table, const int tableWriteCount, const int tableWriteSubCount) 
{
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    PARQUET_ASSIGN_OR_THROW( outfile, arrow::io::FileOutputStream::Open(path + fileName 
                                                                             + "_" + std::to_string(tableWriteCount) 
                                                                             + "_" + std::to_string(tableWriteSubCount) 
                                                                             + ".parquet"));
    PARQUET_THROW_NOT_OK( parquet::arrow::WriteTable(table, arrow::default_memory_pool(), outfile, 10000));
}

FunctionBlockTypePtr ParquetFbImpl::CreateType()
{
    return FunctionBlockType("FileWriterModuleParquet", "Parquet", "Stores signals into the apache parquet data format.");
}

void ParquetFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto outQueue = List<IPacket>();
    auto outDomainQueue = List<IPacket>();

    std::scoped_lock lock(sync);

    PacketPtr packet;
    const auto connection = port.getConnection();
    if (!connection.assigned())
        return;

    packet = connection.dequeue();
   
    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                break;
            case PacketType::Data:
                if (recordingActive)
                {
                    const auto signalContextItr = inputPortSignalContextMap.find(port.getGlobalId());
                    if (signalContextItr == inputPortSignalContextMap.end())
                        LOG_T("No Signal Context found for Port {}", inputPort.getGlobalId());
                    else 
                    {
                        const auto signalContext = signalContextItr->second;
                        SAMPLE_TYPE_DISPATCH(signalContext.sampleType, processDataPacket, signalContext.signalId, signalContext.domainId, std::move(packet), outQueue, outDomainQueue);
                    }
                }
                break;
            default:
                break;
        }

        packet = connection.dequeue();
    };
}

template <SampleType InputSampleType>
void ParquetFbImpl::processDataPacket(const std::string& globalId, const std::string& domainGlobalId, DataPacketPtr&& packet, ListPtr<IPacket>& outQueue, ListPtr<IPacket>& outDomainQueue)
{
    if (dataTablesMap.find(domainGlobalId) != dataTablesMap.end()) 
    {
        time_t currentTime;
        time(&currentTime);

        const size_t sampleCount = packet.getSampleCount();
        size_t adaptedSampleCount = sampleCount;
        using InputType = typename SampleTypeToType<InputSampleType>::Type;
        auto inputData = static_cast<InputType*>(packet.getData());

        int64_t* domainData = static_cast<int64_t*>(packet.getDomainPacket().getData());
        
        using BuilderType = typename SampleToBuilderType<InputSampleType>::Type;
        auto& dataTable = dataTablesMap.at(domainGlobalId);
        auto builder = static_cast<BuilderType*>(dataTable.dataBuilderMap[globalId].get());

        if (downScalingDevider > 1)
        {
            adaptedSampleCount = 0;
            for (size_t i = 0; i < sampleCount; ++i)
            {
                dataTable.dividerCounter++;
                if (dataTable.dividerCounter == downScalingDevider)
                {
                    inputData[adaptedSampleCount] = inputData[i];
                    domainData[adaptedSampleCount] = domainData[i];
                    adaptedSampleCount++;
                    dataTable.dividerCounter = 0;
                }
            }
        }

        if (adaptedSampleCount == 0)
        {
            // Nothing to store
            return;
        }

        PARQUET_THROW_NOT_OK(builder->AppendValues(inputData, adaptedSampleCount));
        // Domain data only add if it is the first domain package.
        if (dataTable.lastDomainValue  != domainData[adaptedSampleCount-1])
        {
            PARQUET_THROW_NOT_OK(dataTable.domainBuilder.AppendValues(domainData, adaptedSampleCount));
            dataTable.lastDomainValue = domainData[adaptedSampleCount-1];
        }
        dataTable.empty = false;

        if (currentTime > dataTable.batchCylceReached)
        {
            std::shared_ptr<arrow::Table> table = generateTable(dataTable);
            writeParquetFile(*table, dataTable.tableNr, dataTable.batchCount);
            ++dataTable.batchCount;
            dataTable.batchCylceReached = currentTime + batchCycle;
        }
    }
    else
    {
        LOG_T("Data tables have not the domain id: {} stored.", inputPort.getGlobalId());
    }
}

void ParquetFbImpl::onConnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    time_t currentTime;
    std::string domainId = inputPort.getSignal().getDomainSignal().getGlobalId();
    std::string signalId = inputPort.getSignal().getGlobalId();
    auto sampleType = inputPort.getSignal().getDescriptor().getSampleType();

    inputPortSignalContextMap.emplace(std::piecewise_construct, std::forward_as_tuple(inputPort.getGlobalId()), 
                                                        std::forward_as_tuple(signalId, domainId, sampleType));


    if (dataTablesMap.find(domainId) == dataTablesMap.end()) 
    {
        tableNumberCount++;
        dataTablesMap.emplace(std::piecewise_construct, std::forward_as_tuple(domainId), 
                                                        std::forward_as_tuple(domainId, tableNumberCount));
    }

    switch(sampleType)
    {
        case SampleType::Int8:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::Int8Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::int8())));
            break;
        case SampleType::Int16:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::Int16Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::int16())));
            break;
        case SampleType::Int32:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::Int32Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::int32())));
            break;
        case SampleType::Int64:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::Int64Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::int64())));
            break;
        case SampleType::UInt8:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::UInt8Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::uint8())));
            break;
        case SampleType::UInt16:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::UInt16Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::uint16())));
            break;
        case SampleType::UInt32:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::UInt32Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::uint32())));
            break;
        case SampleType::UInt64:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::UInt64Builder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::uint64())));
            break;
        case SampleType::Float32:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::FloatBuilder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::float32())));
            break;
        case SampleType::Float64:
            dataTablesMap.at(domainId).dataBuilderMap[signalId] = std::make_unique<arrow::DoubleBuilder>();
            PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::float64())));
            break;
        default:
            throw std::runtime_error("Invalid sample type");
    }


    time(&currentTime);
    dataTablesMap.at(domainId).batchCylceReached = currentTime + batchCycle;
    LOG_T("Connected to port {}", inputPort.getLocalId());
    createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);
}

void ParquetFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    LOG_T("Disconnected from port {}", inputPort.getLocalId());
    removeInputPort(inputPort);
}

}

END_NAMESPACE_FILE_WRITER_MODULE
