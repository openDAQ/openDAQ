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

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>

BEGIN_NAMESPACE_FILE_WRITER_MODULE

namespace FileWriter
{

static const char* InputDisconnected = "Disconnected";
static const char* InputConnected = "Connected";
static const char* InputInvalid = "Invalid";

// #0 Build dummy data to pass around
// To have some input data, we first create an Arrow Table that holds
// some data.
static std::shared_ptr<arrow::Table> generate_table() {
  arrow::Int64Builder i64builder;
  PARQUET_THROW_NOT_OK(i64builder.AppendValues({1, 2, 3, 4, 5}));
  std::shared_ptr<arrow::Array> i64array;
  PARQUET_THROW_NOT_OK(i64builder.Finish(&i64array));

  arrow::StringBuilder strbuilder;
  PARQUET_THROW_NOT_OK(strbuilder.Append("some"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("string"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("content"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("in"));
  PARQUET_THROW_NOT_OK(strbuilder.Append("rows"));
  std::shared_ptr<arrow::Array> strarray;
  PARQUET_THROW_NOT_OK(strbuilder.Finish(&strarray));

  std::shared_ptr<arrow::Schema> schema = arrow::schema(
      {arrow::field("int", arrow::int64()), arrow::field("str", arrow::utf8())});

  return arrow::Table::Make(schema, {i64array, strarray});
}


// #1 Write out the data as a Parquet file
static void write_parquet_file(const arrow::Table& table) {
  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(
      outfile, arrow::io::FileOutputStream::Open("parquet-arrow-example.parquet"));
  // The last argument to the function call is the size of the RowGroup in
  // the parquet file. Normally you would choose this to be rather large but
  // for the example, we use a small value to have multiple RowGroups.
  PARQUET_THROW_NOT_OK(
      parquet::arrow::WriteTable(table, arrow::default_memory_pool(), outfile, 3));
}

ParquetFbImpl::ParquetFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    createInputPorts();
    initProperties();
    initStatuses();

    std::shared_ptr<arrow::Table> table = generate_table();
    write_parquet_file(*table);
}

void ParquetFbImpl::initProperties()
{
    const auto fileNameProp = StringProperty("FileName", "");
    objPtr.addProperty(fileNameProp);
    objPtr.getOnPropertyValueWrite("FileName") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void ParquetFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
    if (configure)
        this->configure();
}

void ParquetFbImpl::readProperties()
{
    fileName = static_cast<std::string>(objPtr.getPropertyValue("FileName"));
}

FunctionBlockTypePtr ParquetFbImpl::CreateType()
{
    return FunctionBlockType("file_writer_module_parquet", "Parquet", "Stores signals into the apache parquet data format.");
}

void ParquetFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                                   const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    configure();
}

void ParquetFbImpl::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        setInputStatus(InputInvalid);
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
            throw std::runtime_error("Arrays not supported");

        inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 &&
            inputSampleType != SampleType::Float32 &&
            inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 &&
            inputSampleType != SampleType::Int32 &&
            inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 &&
            inputSampleType != SampleType::UInt16 &&
            inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
            throw std::runtime_error("Invalid sample type");

       
    }
    catch (const std::exception& e)
    {
        setInputStatus(InputInvalid);
        LOG_W("Failed to set descriptor for power signal: {}", e.what())
    }

    setInputStatus(InputConnected);
}


void ParquetFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto outQueue = List<IPacket>();
    auto outDomainQueue = List<IPacket>();

    std::scoped_lock lock(sync);

    PacketPtr packet;
    const auto connection = inputPort.getConnection();
    if (!connection.assigned())
        return;

    packet = connection.dequeue();

    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;

            case PacketType::Data:
                SAMPLE_TYPE_DISPATCH(inputSampleType, processDataPacket, std::move(packet), outQueue, outDomainQueue);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    };

}

void ParquetFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(inputDataDescriptor, inputDomainDataDescriptor);
    }
}


 template <SampleType InputSampleType>
void ParquetFbImpl::processDataPacket(DataPacketPtr&& packet, ListPtr<IPacket>& outQueue, ListPtr<IPacket>& outDomainQueue)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    std::cout << "bla" << std::endl;
    // ToDO
    //for (size_t i = 0; i < sampleCount; i++)
        
}

void ParquetFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::SchedulerQueueWasEmpty);
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
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("ScalingFunctionBlock");
        LOG_W("Couldn't add type {} to type manager: {}", inputStatusType.getName(), e.what());
    }
    catch (...)
    {
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("ScalingFunctionBlock");
        LOG_W("Couldn't add type {} to type manager!", inputStatusType.getName());
    }

    auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    auto inputStatusValue = Enumeration("InputStatusType", InputDisconnected, context.getTypeManager());
    thisStatusContainer.addStatus("InputStatus", inputStatusValue);
}

void ParquetFbImpl::setInputStatus(const StringPtr& value)
{
    auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    auto inputStatusValue = Enumeration("InputStatusType", value, context.getTypeManager());
    thisStatusContainer.setStatus("InputStatus", inputStatusValue);
}

void ParquetFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    if (this->inputPort == inputPort)
    {
        setInputStatus(InputDisconnected);
    }
}

}

END_NAMESPACE_FILE_WRITER_MODULE
