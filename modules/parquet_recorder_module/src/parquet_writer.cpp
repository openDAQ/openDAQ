#include <parquet_recorder_module/parquet_writer.h>

#include <boost/algorithm/string/trim.hpp>

#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

#include <parquet_recorder_module/type_resolver.h>

using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

static inline std::string sequencePostfix(unsigned sequence, unsigned width = 4)
{
    std::ostringstream oss;
    oss << "_" << std::setw(width) << std::setfill('0') << sequence;
    return oss.str();
}

static std::string getFilename(const fs::path& path, const SignalPtr& signal)
{
    std::string id = signal.getGlobalId();

    std::transform(id.begin(), id.end(), id.begin(), [](auto c) { return std::isalnum(c) ? c : '_'; });

    boost::trim_if(id, boost::is_any_of("_ "));

    constexpr const char* extension = ".parquet";

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
}

ParquetWriter::~ParquetWriter()
{
    setClosing(true);
    waitForNoRunningTask();
    // waitForFinalTask();
    // waitForNoListsToWrite();
    closeFile();
}

void ParquetWriter::onPacketList(const ListPtr<PacketPtr>& packets, bool active, bool recording, uint64_t taskId)
{
    LOG_I("ParquetWriter::onPacketList: Processing packet list with {} packets, active: {}, recording: {}",
          packets.getCount(),
          active,
          recording);
    for (const auto& packet : packets)
    {
        onPacket(packet, active, recording, taskId);
    }
}

void ParquetWriter::onPacket(const PacketPtr& packet, bool active, bool recording, uint64_t taskId)
{
    if (!packet.assigned())
        return;

    switch (packet.getType())
    {
        case PacketType::Data:
            if (active && recording)
                onDataPacket(packet, taskId);
            break;
        case PacketType::Event:
            onEventPacket(packet, taskId);
            break;
        case PacketType::None:
        default:
            LOG_W("Packet has no type");
    }
}

void ParquetWriter::onDataPacket(const DataPacketPtr& packet, uint64_t taskId)
{
    auto domainPacket = packet.getDomainPacket();
    if (auto ulock = std::unique_lock(dataMutex); !schema)
    {
        ulock.unlock();
        DataDescriptorPtr dataDescriptor = packet.getDataDescriptor();
        DataDescriptorPtr domainDescriptor = domainPacket.getDataDescriptor();
        reconfigure(dataDescriptor, domainDescriptor, taskId);
    }

    writePackets(packet, domainPacket, taskId);
}

void ParquetWriter::writePackets(const DataPacketPtr& data, const DataPacketPtr& domain, uint64_t taskId)
{
    SampleType dataType = !currentDataDescriptor.assigned() ? SampleType::Null : currentDataDescriptor.getSampleType();
    SampleType domainType = !currentDomainDescriptor.assigned() ? SampleType::Null : currentDomainDescriptor.getSampleType();

    switch (dataType)
    {
        case SampleType::Float32:
            writePackets<SampleTypeToType<SampleType::Float32>::Type>(data, domain, taskId);
            break;
        case SampleType::Float64:
            writePackets<SampleTypeToType<SampleType::Float64>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt8:
            writePackets<SampleTypeToType<SampleType::UInt8>::Type>(data, domain, taskId);
            break;
        case SampleType::Int8:
            writePackets<SampleTypeToType<SampleType::Int8>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt16:
            writePackets<SampleTypeToType<SampleType::UInt16>::Type>(data, domain, taskId);
            break;
        case SampleType::Int16:
            writePackets<SampleTypeToType<SampleType::Int16>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt32:
            writePackets<SampleTypeToType<SampleType::UInt32>::Type>(data, domain, taskId);
            break;
        case SampleType::Int32:
            writePackets<SampleTypeToType<SampleType::Int32>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt64:
            writePackets<SampleTypeToType<SampleType::UInt64>::Type>(data, domain, taskId);
            break;
        case SampleType::Int64:
            writePackets<SampleTypeToType<SampleType::Int64>::Type>(data, domain, taskId);
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

template <typename DataType>
void ParquetWriter::writePackets(const DataPacketPtr& data, const DataPacketPtr& domain, uint64_t taskId)
{
    SampleType dataType = !currentDataDescriptor.assigned() ? SampleType::Null : currentDataDescriptor.getSampleType();
    SampleType domainType = !currentDomainDescriptor.assigned() ? SampleType::Null : currentDomainDescriptor.getSampleType();

    switch (domainType)
    {
        case SampleType::Float32:
            writePackets<DataType, SampleTypeToType<SampleType::Float32>::Type>(data, domain, taskId);
            break;
        case SampleType::Float64:
            writePackets<DataType, SampleTypeToType<SampleType::Float64>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt8:
            writePackets<DataType, SampleTypeToType<SampleType::UInt8>::Type>(data, domain, taskId);
            break;
        case SampleType::Int8:
            writePackets<DataType, SampleTypeToType<SampleType::Int8>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt16:
            writePackets<DataType, SampleTypeToType<SampleType::UInt16>::Type>(data, domain, taskId);
            break;
        case SampleType::Int16:
            writePackets<DataType, SampleTypeToType<SampleType::Int16>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt32:
            writePackets<DataType, SampleTypeToType<SampleType::UInt32>::Type>(data, domain, taskId);
            break;
        case SampleType::Int32:
            writePackets<DataType, SampleTypeToType<SampleType::Int32>::Type>(data, domain, taskId);
            break;
        case SampleType::UInt64:
            writePackets<DataType, SampleTypeToType<SampleType::UInt64>::Type>(data, domain, taskId);
            break;
        case SampleType::Int64:
            writePackets<DataType, SampleTypeToType<SampleType::Int64>::Type>(data, domain, taskId);
            break;
        case SampleType::String:
        case SampleType::Binary:
        case SampleType::ComplexFloat64:
        case SampleType::ComplexFloat32:
        case SampleType::RangeInt64:
        case SampleType::Struct:
            LOG_W("ParquetWriter::writePackets<{}>: Unsupported data type: {}, skipping write operation",
                  typeid(DataType).name(),
                  convertSampleTypeToString(dataType));
            break;
        case SampleType::Undefined:
        case SampleType::Null:
        default:
            LOG_W("ParquetWriter::writePackets<{}>: Undefined or null data type, skipping write operation", typeid(DataType).name());
            break;
    }
}

template <typename DataType, typename DomainType>
void ParquetWriter::writePackets(const DataPacketPtr& data, const DataPacketPtr& domain, uint64_t taskId)
{
    typename ArrowTypeResolver<DataType>::BuilderType sampleBuilder;
    typename ArrowTypeResolver<DomainType>::BuilderType domainBuilder;

    if (!data.assigned())
    {
        LOG_E("Data packet is null, cannot write samples");
        return;
    }

    auto status = sampleBuilder.AppendValues(static_cast<DataType*>(data.getData()), data.getSampleCount());

    if (domain.assigned())
    {
        status = domainBuilder.AppendValues(static_cast<DomainType*>(domain.getData()), domain.getSampleCount());
    }

    const auto sampleCount = data.getSampleCount();
    const auto domainCount = domain.assigned() ? domain.getSampleCount() : 0;
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

    {
        std::unique_lock lock(dataMutex);
        // if(!tryWaitSequence(lock, packetId)) {
        //     LOG_W("Packet could be written out of sequence, packet ID: {}, expected packet ID: {}",
        //           packetId,
        //           nextPacketId.load(std::memory_order_relaxed));
        // }
        status = writer->WriteRecordBatch(*batch);
        if (!status.ok())
        {
            LOG_E("Failed to write record batch to Parquet file: {}", status.ToString());
        } else 
        {
            LOG_I("ParquetWriter::writePackets: Successfully wrote record batch with ID: {} and sample count: {}", packetId, sampleCount);
        }
    }
}

void ParquetWriter::onEventPacket(const EventPacketPtr& packet, uint64_t taskId)
{
    LOG_I("ParquetWriter::onEventPacket: Processing event packet with ID: {}", packet.getEventId());
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr dataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr domainDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        reconfigure(dataDescriptor, domainDescriptor, taskId);
    }
}

void ParquetWriter::generateMetadata(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor)
{
    LOG_I("ParquetWriter::generateMetadata: Generating metadata for data and domain descriptors");
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

    LOG_I("ParquetWriter::generateMetadata: Data field '{}' of type '{}' with metadata '{}', Domain field '{}' of type '{}' with metadata "
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
        LOG_I("ParquetWriter::generateMetadata: Schema generated with data field '{}' of type '{}' and domain field '{}' of type '{}'",
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
    LOG_I("ParquetWriter::openFile: Opening Parquet file for writing at path: {}", path.string());

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
    LOG_I("ParquetWriter::openFile: Parquet file file {} writer {}",
          outfile ? "opened successfully" : "failed to open",
          writer ? "created successfully" : "failed to create");
}

void ParquetWriter::closeFile()
{
    LOG_I("ParquetWriter::closeFile: Closing Parquet file and writer");
    std::lock_guard<std::mutex> lock(dataMutex);
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
    LOG_I("ParquetWriter::configure: Configuring ParquetWriter with data and domain descriptors");
    auto lock = std::lock_guard(dataMutex);
    generateMetadata(dataDescriptor, domainDescriptor);
    openFile();
}

void ParquetWriter::reconfigure(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor, uint64_t taskId)
{
    LOG_I("ParquetWriter::reconfigure: Reconfiguring ParquetWriter with new data and domain descriptors");
    if (!dataDescriptor.assigned() || !domainDescriptor.assigned())
    {
        LOG_E("Data or domain descriptor is null, cannot reconfigure");
        return;
    }

    if (auto lock = std::lock_guard(dataMutex); dataDescriptor == currentDataDescriptor && domainDescriptor == currentDomainDescriptor)
    {
        LOG_I("ParquetWriter::reconfigure: No changes in data or domain descriptor, skipping reconfiguration.");
        return;
    }

    if (getClosing())
    {
        LOG_W("ParquetWriter::reconfigure: Writer is closing, skipping reconfiguration.");
        return;
    }

    setClosing(true);
    closeFile();
    setClosing(false);

    configure(dataDescriptor, domainDescriptor);
}

void ParquetWriter::setClosing(bool value)
{
    closing.store(value, std::memory_order_relaxed);
}

bool ParquetWriter::getClosing() const
{
    return closing.load(std::memory_order_relaxed);
}

bool ParquetWriter::getHasRunningTask() const
{
    return hasRunningTask.load(std::memory_order_relaxed);
}

void ParquetWriter::setHasRunningTask(bool value)
{
    hasRunningTask.store(value, std::memory_order_relaxed);
    if (!value)
    {
        noRunningTaskCondition.notify_all();
    }
}

void ParquetWriter::waitForNoRunningTask()
{
    std::unique_lock lock(closingMutex);
    if (getHasRunningTask())
    {
        LOG_I("ParquetWriter::waitForNoRunningTask: Waiting for no running tasks");
        noRunningTaskCondition.wait(lock, [this] { return !getHasRunningTask(); });
    }
    else
    {
        LOG_I("ParquetWriter::waitForNoRunningTask: No need to wait");
    }
}


END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE