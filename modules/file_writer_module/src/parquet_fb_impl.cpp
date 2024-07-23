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
    , recordingActive(false)
    , dataRecorded(false)
{
    initProperties();
    createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);
    initStatuses();
}

void ParquetFbImpl::initProperties()
{
    const auto fileNameProp = StringProperty("FileName", "");
    objPtr.addProperty(fileNameProp);
    objPtr.getOnPropertyValueWrite("FileName") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto recordingActive = BoolProperty("RecordingActive", false);
    objPtr.addProperty(recordingActive);
    objPtr.getOnPropertyValueWrite("RecordingActive") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void ParquetFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
    if (!recordingActive && dataRecorded)
    { 
        for (auto& dataTable : dataTablesMap)
        {
            
            std::shared_ptr<arrow::Table> table = generateTable(dataTable.second);
            writeParquetFile(*table);
        }

    }
}

std::shared_ptr<arrow::Table> ParquetFbImpl::generateTable(DataTable& dataTable) 
{
    arrow::ArrayVector dataVector;
    std::shared_ptr<arrow::Array> int64Array;
    dataTable.domainBuilder.Finish(&int64Array);

    dataVector.emplace_back(int64Array);

    // Iterate and print key-value pairs
    for (auto& entry : dataTable.dataBuilderMap) 
    {
        std::shared_ptr<arrow::Array> doubleArray;
        PARQUET_THROW_NOT_OK(entry.second.Finish(&doubleArray));
        dataVector.emplace_back(doubleArray);
    }
    auto schema = dataTable.schemaBuilder.Finish();
    return arrow::Table::Make(schema.ValueOrDie(), dataVector);
}

void ParquetFbImpl::writeParquetFile(const arrow::Table& table) 
{
  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(
      outfile, arrow::io::FileOutputStream::Open("parquet-arrow-example.parquet"));
  // The last argument to the function call is the size of the RowGroup in
  // the parquet file. Normally you would choose this to be rather large but
  // for the example, we use a small value to have multiple RowGroups.
  PARQUET_THROW_NOT_OK(
      parquet::arrow::WriteTable(table, arrow::default_memory_pool(), outfile, 3));
}

void ParquetFbImpl::readProperties()
{
    fileName = static_cast<std::string>(objPtr.getPropertyValue("FileName"));
    recordingActive = static_cast<bool>(objPtr.getPropertyValue("RecordingActive"));
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
        const size_t sampleCount = packet.getSampleCount();
        double* inputData = static_cast<double*>(packet.getData());
        int64_t* domainData =  static_cast<int64_t*>(packet.getDomainPacket().getData());
        auto& dataTable = dataTablesMap.at(domainGlobalId);
        PARQUET_THROW_NOT_OK(dataTable.dataBuilderMap[globalId].AppendValues(inputData, sampleCount));
        PARQUET_THROW_NOT_OK(dataTable.domainBuilder.AppendValues(domainData, sampleCount));

        dataRecorded = true;
    } 

    
}

void ParquetFbImpl::onConnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    LOG_T("Connected to port {}", inputPort.getLocalId());

    std::string domainId = inputPort.getSignal().getDomainSignal().getGlobalId();
    std::string signalId = inputPort.getSignal().getGlobalId();

    if (dataTablesMap.find(domainId) == dataTablesMap.end()) 
    {
        dataTablesMap.emplace(domainId, domainId);
    }
    dataTablesMap.at(domainId).dataBuilderMap[signalId] = arrow::DoubleBuilder();
    dataTablesMap.at(domainId).schemaBuilder.AddField(arrow::field(signalId, arrow::float64()));

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
