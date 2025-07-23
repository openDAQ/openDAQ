#include <parquet_recorder_module/parquet_writer.h>

#include <boost/algorithm/string/trim.hpp>

#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/work_factory.h>

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

#include <parquet_recorder_module/type_resolver.h>

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

static inline std::string sequencePostfix(unsigned sequence, unsigned width = 4)
{
    std::ostringstream oss;
    oss << "_" << std::setw(width) << std::setfill('0') << sequence;
    return oss.str();
}

static std::string getFilename(const fs::path& path, const SignalPtr& signal)
{
    constexpr const char* extension = ".parquet";

    std::string id = signal.getGlobalId();
    std::transform(id.begin(), id.end(), id.begin(), [](auto c) { return std::isalnum(c) ? c : '_'; });
    boost::trim_if(id, boost::is_any_of("_ "));

    std::string name;
    unsigned sequence = 0;
    do
    {
        if (sequence == std::numeric_limits<unsigned>::max())
            throw InvalidStateException("Too many files with the same base name in directory: " + path.string());
        name = id + sequencePostfix(sequence++) + extension;
    } while (fs::exists(path / name));

    return name;
}

ParquetWriter::ParquetWriter(fs::path path, SignalPtr signal, daq::LoggerComponentPtr logger_component, daq::SchedulerPtr scheduler)
    : path(std::move(path))
    , signal(std::move(signal))
    , loggerComponent(std::move(logger_component))
    , scheduler(std::move(scheduler))
    , filename(getFilename(this->path, this->signal))
{
    packetBuffer.reserve(PACKET_BUFFER_SIZE_TO_WRITE);
}

ParquetWriter::~ParquetWriter()
{
    isClosing = true;
    scheduler.waitAll();
    std::lock_guard bufferLock(packetBufferMutex);
    std::lock_guard lock(mutex);
    closeFile();
}

void ParquetWriter::onPacket(const PacketPtr& packet)
{
    if (!packet.assigned())
        return;

    switch (packet.getType())
    {
        case PacketType::Data:
            onDataPacket(packet);
            break;
        case PacketType::Event:
            onEventPacket(packet);
            break;
        case PacketType::None:
        default:
            LOG_W("Packet has no type");
    }
}

void ParquetWriter::onDataPacket(const DataPacketPtr& packet)
{
    auto domainPacket = packet.getDomainPacket();
    if (!schema)
    {
        DataDescriptorPtr dataDescriptor = packet.getDataDescriptor();
        DataDescriptorPtr domainDescriptor = domainPacket.getDataDescriptor();
        reconfigure(dataDescriptor, domainDescriptor);
    }

    writePackets(packet, domainPacket);
}

void ParquetWriter::writePackets(const DataPacketPtr& data, const DataPacketPtr& domain)
{
    SampleType dataType = !currentDataDescriptor.assigned() ? SampleType::Null : currentDataDescriptor.getSampleType();

    switch (dataType)
    {
        case SampleType::Float32:
            writePackets<SampleTypeToType<SampleType::Float32>::Type>(data, domain);
            break;
        case SampleType::Float64:
            writePackets<SampleTypeToType<SampleType::Float64>::Type>(data, domain);
            break;
        case SampleType::UInt8:
            writePackets<SampleTypeToType<SampleType::UInt8>::Type>(data, domain);
            break;
        case SampleType::Int8:
            writePackets<SampleTypeToType<SampleType::Int8>::Type>(data, domain);
            break;
        case SampleType::UInt16:
            writePackets<SampleTypeToType<SampleType::UInt16>::Type>(data, domain);
            break;
        case SampleType::Int16:
            writePackets<SampleTypeToType<SampleType::Int16>::Type>(data, domain);
            break;
        case SampleType::UInt32:
            writePackets<SampleTypeToType<SampleType::UInt32>::Type>(data, domain);
            break;
        case SampleType::Int32:
            writePackets<SampleTypeToType<SampleType::Int32>::Type>(data, domain);
            break;
        case SampleType::UInt64:
            writePackets<SampleTypeToType<SampleType::UInt64>::Type>(data, domain);
            break;
        case SampleType::Int64:
            writePackets<SampleTypeToType<SampleType::Int64>::Type>(data, domain);
            break;
        case SampleType::String:
        case SampleType::Binary:
        case SampleType::ComplexFloat64:
        case SampleType::ComplexFloat32:
        case SampleType::RangeInt64:
        case SampleType::Struct:
            LOG_W("ParquetWriter::writePackets: Unsupported data type: {}, skipping write operation", convertSampleTypeToString(dataType));
            break;
        case SampleType::Undefined:
        case SampleType::Null:
        default:
            LOG_W("ParquetWriter::writePackets: Undefined or null data type, skipping write operation");
            break;
    }
}

template <typename TDataType>
void ParquetWriter::writePackets(const DataPacketPtr& data, const DataPacketPtr& domain)
{
    SampleType domainType = !currentDomainDescriptor.assigned() ? SampleType::Null : currentDomainDescriptor.getSampleType();

    switch (domainType)
    {
        case SampleType::Float32:
            writePackets<TDataType, SampleTypeToType<SampleType::Float32>::Type>(data, domain);
            break;
        case SampleType::Float64:
            writePackets<TDataType, SampleTypeToType<SampleType::Float64>::Type>(data, domain);
            break;
        case SampleType::UInt8:
            writePackets<TDataType, SampleTypeToType<SampleType::UInt8>::Type>(data, domain);
            break;
        case SampleType::Int8:
            writePackets<TDataType, SampleTypeToType<SampleType::Int8>::Type>(data, domain);
            break;
        case SampleType::UInt16:
            writePackets<TDataType, SampleTypeToType<SampleType::UInt16>::Type>(data, domain);
            break;
        case SampleType::Int16:
            writePackets<TDataType, SampleTypeToType<SampleType::Int16>::Type>(data, domain);
            break;
        case SampleType::UInt32:
            writePackets<TDataType, SampleTypeToType<SampleType::UInt32>::Type>(data, domain);
            break;
        case SampleType::Int32:
            writePackets<TDataType, SampleTypeToType<SampleType::Int32>::Type>(data, domain);
            break;
        case SampleType::UInt64:
            writePackets<TDataType, SampleTypeToType<SampleType::UInt64>::Type>(data, domain);
            break;
        case SampleType::Int64:
            writePackets<TDataType, SampleTypeToType<SampleType::Int64>::Type>(data, domain);
            break;
        case SampleType::String:
        case SampleType::Binary:
        case SampleType::ComplexFloat64:
        case SampleType::ComplexFloat32:
        case SampleType::RangeInt64:
        case SampleType::Struct:
            LOG_W("ParquetWriter::writePackets<{}>: Unsupported data type, skipping write operation", typeid(TDataType).name());
            break;
        case SampleType::Undefined:
        case SampleType::Null:
        default:
            LOG_W("ParquetWriter::writePackets<{}>: Undefined or null data type, skipping write operation", typeid(TDataType).name());
            break;
    }
}

template <typename TDataType, typename TDomainType>
void ParquetWriter::writePackets(const DataPacketPtr& data, const DataPacketPtr& domain)
{
    typename ArrowTypeResolver<TDataType>::BuilderType sampleBuilder;
    typename ArrowTypeResolver<TDomainType>::BuilderType domainBuilder;

    if (!data.assigned())
    {
        LOG_E("Data packet is null, cannot write samples");
        return;
    }

    auto status = sampleBuilder.AppendValues(static_cast<TDataType*>(data.getData()), data.getSampleCount());
    if(!status.ok()) {
        LOG_E("Failed to append sample values: {}", status.ToString());
        return;
    }

    if (domain.assigned())
    {
        status = domainBuilder.AppendValues(static_cast<TDomainType*>(domain.getData()), domain.getSampleCount());
        if(!status.ok()) {
            LOG_E("Failed to append domain values: {}", status.ToString());
            return;
        }
    } else {
        status = domainBuilder.AppendNulls(data.getSampleCount());
        if(!status.ok()) {
            LOG_E("Failed to append null domain values: {}", status.ToString());
            return;
        }
    }

    const auto sampleCount = data.getSampleCount();
    const auto domainCount = domain.assigned() ? domain.getSampleCount() : sampleCount;
    const auto packetId = data.getPacketId();

    if (sampleCount != domainCount)
    {
        LOG_E("Sample count ({}) does not match domain count ({}), cannot write to Parquet file", sampleCount, domainCount);
        return;
    }

    auto samples = sampleBuilder.Finish().ValueOr(nullptr);
    auto domains = domainBuilder.Finish().ValueOr(nullptr);

    if (!domains)
    {
        LOG_E("Failed to finish domain values");
        return;
    }
    if (!samples)
    {
        LOG_E("Failed to finish sample values");
        return;
    }

    auto batch = arrow::RecordBatch::Make(schema, sampleCount, {domains, samples});

    if (!batch)
    {
        LOG_E("Failed to create RecordBatch for Parquet file");
        return;
    }

    status = writer->WriteRecordBatch(*batch);
    if (!status.ok())
    {
        LOG_E("Failed to write record batch to Parquet file: {}", status.ToString());
    }
    else
    {
        LOG_D("ParquetWriter::writePackets: Successfully wrote record batch with ID: {} and sample count: {}", packetId, sampleCount);
    }
}

void ParquetWriter::onEventPacket(const EventPacketPtr& packet)
{
    LOG_D("ParquetWriter::onEventPacket: Processing event packet with ID: {}", packet.getEventId());
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr dataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr domainDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        reconfigure(dataDescriptor, domainDescriptor);
    }
}

void ParquetWriter::generateMetadata(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor)
{
    LOG_D("ParquetWriter::generateMetadata: Generating metadata for data and domain descriptors");
    if (!dataDescriptor.assigned() || !domainDescriptor.assigned())
    {
        LOG_E("Data or domain descriptor is null, cannot generate metadata");
        return;
    }

    auto dataName = dataDescriptor.getName().toString();
    auto dataType = dataDescriptor.getSampleType();

    auto domainName = domainDescriptor.getName().toString();
    auto domainType = domainDescriptor.getSampleType();

    auto getMetadata = [](const auto& descriptor) -> std::string
    {
        auto json_serializer = JsonSerializer();
        descriptor.serialize(json_serializer);
        return json_serializer.getOutput();
    };

    auto dataMetadata = getMetadata(dataDescriptor);
    auto domainMetadata = getMetadata(domainDescriptor);

    LOG_D("ParquetWriter::generateMetadata: Data field '{}' of type '{}' with metadata '{}', Domain field '{}' of type '{}' with metadata "
          "'{}'",
          dataName,
          convertSampleTypeToString(dataType),
          dataMetadata,
          domainName,
          convertSampleTypeToString(domainType),
          domainMetadata);

    currentDataDescriptor = dataDescriptor;
    currentDomainDescriptor = domainDescriptor;

    schema = arrow::schema(
        {arrow::field(domainName, arrow_type_from_sample_type(domainType), true, arrow::key_value_metadata({{"metadata", domainMetadata}})),
         arrow::field(dataName, arrow_type_from_sample_type(dataType), false, arrow::key_value_metadata({{"metadata", dataMetadata}}))});

    if (schema)
    {
        LOG_D("ParquetWriter::generateMetadata: Schema generated with data field '{}' of type '{}' and domain field '{}' of type '{}'",
              dataName,
              convertSampleTypeToString(dataType),
              domainName,
              convertSampleTypeToString(domainType));
    }
    else
    {
        LOG_E("Failed to generate schema for Parquet file");
    }
}

void ParquetWriter::openFile()
{
    LOG_D("ParquetWriter::openFile: Opening Parquet file for writing at path: {}", path.string());

    // Set up Parquet file output
    outfile = arrow::io::FileOutputStream::Open(getFilename(path, signal)).ValueOr(nullptr);
    if (outfile && schema)
    {
        auto arrowPropertiesBuilder = parquet::ArrowWriterProperties::Builder();
        arrowPropertiesBuilder.store_schema();

        auto writerPropertiesBuilder = parquet::WriterProperties::Builder();

        // Create Parquet FileWriter
        writer = parquet::arrow::FileWriter::Open(
                     *schema, arrow::default_memory_pool(), outfile, writerPropertiesBuilder.build(), arrowPropertiesBuilder.build())
                     .ValueOr(nullptr);
    }
    LOG_D("ParquetWriter::openFile: Parquet file file {} writer {}",
          outfile ? "opened successfully" : "failed to open",
          writer ? "created successfully" : "failed to create");
}

void ParquetWriter::closeFile()
{
    LOG_D("ParquetWriter::closeFile: Closing Parquet file and writer");
    if (writer)
    {
        auto status = writer->Close();
        if (!status.ok())
        {
            LOG_E("Failed to close Parquet writer: {}", status.ToString());
        }
        writer.reset();
    }
    if (outfile)
    {
        auto status = outfile->Close();
        if (!status.ok())
        {
            LOG_E("Failed to close Parquet file output: {}", status.ToString());
        }
        outfile.reset();
    }
}

void ParquetWriter::configure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor)
{
    LOG_D("ParquetWriter::configure: Configuring ParquetWriter with data and domain descriptors");
    generateMetadata(dataDescriptor, domainDescriptor);
    openFile();
}

void ParquetWriter::reconfigure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor)
{
    LOG_D("ParquetWriter::reconfigure: Reconfiguring ParquetWriter with new data and domain descriptors");
    if (!dataDescriptor.assigned() || !domainDescriptor.assigned())
    {
        LOG_E("Data or domain descriptor is null, cannot reconfigure");
        return;
    }

    if (dataDescriptor == currentDataDescriptor && domainDescriptor == currentDomainDescriptor)
    {
        LOG_D("ParquetWriter::reconfigure: No changes in data or domain descriptor, skipping reconfiguration.");
        return;
    }

    closeFile();
    configure(dataDescriptor, domainDescriptor);
}

void ParquetWriter::enqueuePacketList(ListPtr<IPacket>& packets)
{
    std::lock_guard lock(packetBufferMutex);
    if (isClosing) return;

    for (auto&& packet : packets)
    {
        packetBuffer.push_back(packet);
    }

    if (packetBuffer.size() >= PACKET_BUFFER_SIZE_TO_WRITE)
    {
        scheduler.scheduleWork(Work(  // main processing task
            [this]()
            {
                std::lock_guard lock(mutex);
                try
                {
                    auto packets = dequeuePacketList();
                    processPacketList(packets);
                }
                catch (const std::exception& e)
                {
                    LOG_E("ParquetWriter::enqueuePacketList: Exception while processing packet list: {}", e.what());
                }
            }));
    }
}

std::vector<PacketPtr> ParquetWriter::dequeuePacketList()
{
    std::lock_guard lock(packetBufferMutex);
    std::vector<PacketPtr> packets;
    std::swap(packetBuffer, packets);
    return packets;
}

void ParquetWriter::processPacketList(const std::vector<PacketPtr>& packets)
{
    auto active = true;
    auto recording = true;
    auto taskId = 0;
    for (const auto& packet : packets)
    {
        onPacket(packet);
    }
}

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE