#include <ctime>
#include <iostream>

#include <file_writer_module/parquet_fb_impl.h>
#include <file_writer_module/dispatch.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>

#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>

#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>

#include "coreobjects/unit_factory.h"
#include "opendaq/data_packet.h"
#include "opendaq/data_packet_ptr.h"
#include "opendaq/event_packet_ids.h"
#include "opendaq/packet_factory.h"
#include "opendaq/range_factory.h"
#include "opendaq/sample_type_traits.h"
#include <coreobjects/eval_value_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/component_status_container_private_ptr.h>

BEGIN_NAMESPACE_FILE_WRITER_MODULE

namespace FileWriter
{

static const char* InputDisconnected = "Disconnected";
static const char* InputConnected = "Connected";
static const char* InputInvalid = "Invalid";

ParquetFbImpl::ParquetFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , inputPortCount(0)
    , tableNumberCount(0)
    , recordingActive(false)
{
    initProperties();
    createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);
    initStatuses();
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

    const auto writeBatchCylceInSecProp = IntProperty("WriteBatchCylceInSec", 20);
    objPtr.addProperty(writeBatchCylceInSecProp);
    objPtr.getOnPropertyValueWrite("WriteBatchCylceInSec") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

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
    writeBatchCylceInSec = static_cast<int>(objPtr.getPropertyValue("WriteBatchCylceInSec"));
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
        dataTable.second.batchCylceReached = currentTime + writeBatchCylceInSec;
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
        PARQUET_THROW_NOT_OK(entry.second.Finish(&doubleArray));
        dataVector.emplace_back(doubleArray);
        entry.second.Reset();
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
    PARQUET_THROW_NOT_OK( parquet::arrow::WriteTable(table, arrow::default_memory_pool(), outfile, 1000));
}

FunctionBlockTypePtr ParquetFbImpl::CreateType()
{
    return FunctionBlockType("file_writer_module_parquet", "Parquet", "Stores signals into the apache parquet data format.");
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
                    const auto sampleType = port.getSignal().getDescriptor().getSampleType();
                    const auto globalId = port.getSignal().getGlobalId();
                    const auto domainGlobalId = port.getSignal().getDomainSignal().getGlobalId();
                    SAMPLE_TYPE_DISPATCH(sampleType, processDataPacket, globalId, domainGlobalId, std::move(packet), outQueue, outDomainQueue);
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
        const size_t sampleCount = packet.getSampleCount();
        double* inputData = static_cast<double*>(packet.getData());
        int64_t* domainData = static_cast<int64_t*>(packet.getDomainPacket().getData());
        
        auto& dataTable = dataTablesMap.at(domainGlobalId);
        PARQUET_THROW_NOT_OK(dataTable.dataBuilderMap[globalId].AppendValues(inputData, sampleCount));
        PARQUET_THROW_NOT_OK(dataTable.domainBuilder.AppendValues(domainData, sampleCount));
        dataTable.empty = false;

        time(&currentTime);
        if (currentTime > dataTable.batchCylceReached)
        {
            std::shared_ptr<arrow::Table> table = generateTable(dataTable);
            writeParquetFile(*table, dataTable.tableNr, dataTable.batchCount);
            ++dataTable.batchCount;
            dataTable.batchCylceReached = currentTime + writeBatchCylceInSec;
        }
    } 
}

void ParquetFbImpl::onConnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    time_t currentTime;
    LOG_T("Connected to port {}", inputPort.getLocalId());

    std::string domainId = inputPort.getSignal().getDomainSignal().getGlobalId();
    std::string signalId = inputPort.getSignal().getGlobalId();

    if (dataTablesMap.find(domainId) == dataTablesMap.end()) 
    {
        tableNumberCount++;
        dataTablesMap.emplace(std::piecewise_construct, std::forward_as_tuple(domainId), std::forward_as_tuple(domainId, tableNumberCount));
    }
    dataTablesMap.at(domainId).dataBuilderMap[signalId] = arrow::DoubleBuilder();
    PARQUET_THROW_NOT_OK(dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::float64())));

    time(&currentTime);
    dataTablesMap.at(domainId).batchCylceReached = currentTime + writeBatchCylceInSec;

    createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);
}

void ParquetFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    LOG_T("Disconnected from port {}", inputPort.getLocalId());
    removeInputPort(inputPort);
}


void ParquetFbImpl::initStatuses()
{
    auto inputStatusType = EnumerationType("InputStatusType", List<IString>(InputDisconnected, InputConnected, InputInvalid));

    try
    {
        this->context.getTypeManager().addType(inputStatusType);
    }
    catch (const std::exception& e)
    {
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("Parquet");
        LOG_W("Couldn't add type {} to type manager: {}", inputStatusType.getName(), e.what());
    }
    catch (...)
    {
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("Parquet");
        LOG_W("Couldn't add type {} to type manager!", inputStatusType.getName());
    }

    auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    auto inputStatusValue = Enumeration("InputStatusType", InputDisconnected, context.getTypeManager());
    thisStatusContainer.addStatus("InputStatus", inputStatusValue);
}

}

END_NAMESPACE_FILE_WRITER_MODULE
