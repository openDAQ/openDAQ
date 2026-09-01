#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <coretypes/boolean_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/file_format.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

namespace file_format
{

namespace
{

template <typename T>
void writeScalarLE(std::ostream& stream, T value)
{
    static_assert(std::is_unsigned_v<T>, "writeScalarLE expects an unsigned integer");

    char bytes[sizeof(T)];
    for (std::size_t i = 0; i < sizeof(T); ++i)
        bytes[i] = static_cast<char>((value >> (8 * i)) & 0xFFu);

    stream.write(bytes, sizeof(T));
}

/*!
 * Reads a little-endian scalar. Returns false without throwing if the stream is exhausted before
 * a single byte is read, so that callers can distinguish a clean end of file from a truncated
 * value.
 */
template <typename T>
bool readScalarLE(std::istream& stream, T& value)
{
    static_assert(std::is_unsigned_v<T>, "readScalarLE expects an unsigned integer");

    char bytes[sizeof(T)];
    stream.read(bytes, sizeof(T));

    const auto read = static_cast<std::size_t>(stream.gcount());
    if (read == 0)
        return false;
    if (read != sizeof(T))
        throw std::runtime_error("Truncated recording: incomplete scalar value");

    value = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i)
        value |= static_cast<T>(static_cast<unsigned char>(bytes[i])) << (8 * i);

    return true;
}

/*!
 * Applies @p visitor to a value of the C++ type corresponding to @p sampleType. Keeps the
 * per-sample-type switch in one place instead of repeating it in every tick accessor.
 */
template <typename Visitor>
auto dispatchSampleType(SampleType sampleType, Visitor&& visitor)
{
    switch (sampleType)
    {
        case SampleType::Int8:
            return visitor(std::int8_t{});
        case SampleType::Int16:
            return visitor(std::int16_t{});
        case SampleType::Int32:
            return visitor(std::int32_t{});
        case SampleType::Int64:
            return visitor(std::int64_t{});
        case SampleType::UInt8:
            return visitor(std::uint8_t{});
        case SampleType::UInt16:
            return visitor(std::uint16_t{});
        case SampleType::UInt32:
            return visitor(std::uint32_t{});
        case SampleType::UInt64:
            return visitor(std::uint64_t{});
        default:
            throw std::runtime_error("Unsupported domain sample type");
    }
}

StringPtr serializeToJson(const BaseObjectPtr& object)
{
    const auto serializer = JsonSerializer();
    object.asPtr<ISerializable>().serialize(serializer);
    return serializer.getOutput();
}

}

bool isHostLittleEndian()
{
    const std::uint16_t probe = 1;
    unsigned char firstByte{};
    std::memcpy(&firstByte, &probe, 1);
    return firstByte == 1;
}

StringPtr toIso8601(const std::chrono::system_clock::time_point& timePoint)
{
    const std::time_t time = std::chrono::system_clock::to_time_t(timePoint);

    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif

    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return String(stream.str());
}

std::size_t writeHeader(std::ostream& stream, const Header& header)
{
    auto fields = Dict<IString, IBaseObject>();

    fields.set(header_key::SIGNAL_NAME, header.signalName.assigned() ? header.signalName : String(""));
    fields.set(header_key::SIGNAL_GLOBAL_ID, header.signalGlobalId.assigned() ? header.signalGlobalId : String(""));
    fields.set(header_key::VALUE_DESCRIPTOR, header.valueDescriptor);
    if (header.domainDescriptor.assigned())
        fields.set(header_key::DOMAIN_DESCRIPTOR, header.domainDescriptor);
    fields.set(header_key::PART_INDEX, Integer(header.partIndex));
    fields.set(header_key::CREATED_AT, header.createdAt.assigned() ? header.createdAt : String(""));
    fields.set(header_key::LITTLE_ENDIAN_SAMPLES, BooleanPtr(isHostLittleEndian()));

    const std::string json = serializeToJson(fields).toStdString();

    stream.write(MAGIC, MAGIC_SIZE);
    writeScalarLE<std::uint16_t>(stream, VERSION);
    writeScalarLE<std::uint32_t>(stream, static_cast<std::uint32_t>(json.size()));
    stream.write(json.data(), static_cast<std::streamsize>(json.size()));

    return PREAMBLE_SIZE + json.size();
}

void readHeader(std::istream& stream, Header& header)
{
    char magic[MAGIC_SIZE];
    stream.read(magic, MAGIC_SIZE);
    if (static_cast<std::size_t>(stream.gcount()) != MAGIC_SIZE || std::memcmp(magic, MAGIC, MAGIC_SIZE) != 0)
        throw std::runtime_error("Not an openDAQ recording: bad magic");

    std::uint16_t version = 0;
    if (!readScalarLE(stream, version))
        throw std::runtime_error("Truncated recording: missing format version");
    if (version != VERSION)
        throw std::runtime_error("Unsupported recording format version " + std::to_string(version));

    std::uint32_t headerLength = 0;
    if (!readScalarLE(stream, headerLength))
        throw std::runtime_error("Truncated recording: missing header length");

    std::string json(headerLength, '\0');
    stream.read(json.data(), static_cast<std::streamsize>(headerLength));
    if (static_cast<std::size_t>(stream.gcount()) != headerLength)
        throw std::runtime_error("Truncated recording: incomplete header");

    DictPtr<IString, IBaseObject> fields;
    try
    {
        fields = JsonDeserializer().deserialize(json).asPtr<IDict>();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Malformed recording header: ") + e.what());
    }

    const BooleanPtr littleEndianSamples = fields.getOrDefault(header_key::LITTLE_ENDIAN_SAMPLES, BooleanPtr(True));
    if (static_cast<bool>(littleEndianSamples) != isHostLittleEndian())
        throw std::runtime_error("Recording was written with the opposite byte order and cannot be replayed on this host");

    header.signalName = fields.getOrDefault(header_key::SIGNAL_NAME, String(""));
    header.signalGlobalId = fields.getOrDefault(header_key::SIGNAL_GLOBAL_ID, String(""));
    header.createdAt = fields.getOrDefault(header_key::CREATED_AT, String(""));

    header.partIndex = static_cast<Int>(fields.getOrDefault(header_key::PART_INDEX, Integer(1)));

    if (!fields.hasKey(header_key::VALUE_DESCRIPTOR))
        throw std::runtime_error("Malformed recording header: no value descriptor");
    header.valueDescriptor = fields.get(header_key::VALUE_DESCRIPTOR).asPtr<IDataDescriptor>();

    if (fields.hasKey(header_key::DOMAIN_DESCRIPTOR))
        header.domainDescriptor = fields.get(header_key::DOMAIN_DESCRIPTOR).asPtr<IDataDescriptor>();
    else
        header.domainDescriptor = nullptr;
}

void writeBlockHeader(std::ostream& stream, const BlockHeader& blockHeader)
{
    writeScalarLE<std::uint8_t>(stream, static_cast<std::uint8_t>(blockHeader.domainKind));
    writeScalarLE<std::uint64_t>(stream, blockHeader.sampleCount);
}

bool readBlockHeader(std::istream& stream, BlockHeader& blockHeader)
{
    std::uint8_t domainKind = 0;
    if (!readScalarLE(stream, domainKind))
        return false;

    if (domainKind > static_cast<std::uint8_t>(DomainKind::Explicit))
        throw std::runtime_error("Malformed recording: unrecognized domain kind " + std::to_string(domainKind));

    if (!readScalarLE(stream, blockHeader.sampleCount))
        throw std::runtime_error("Truncated recording: incomplete block header");

    blockHeader.domainKind = static_cast<DomainKind>(domainKind);
    return true;
}

std::size_t blockPayloadSize(const BlockHeader& blockHeader, std::size_t valueSampleSize, std::size_t domainSampleSize)
{
    std::size_t size = blockHeader.sampleCount * valueSampleSize;

    switch (blockHeader.domainKind)
    {
        case DomainKind::Implicit:
            size += domainSampleSize;
            break;
        case DomainKind::Explicit:
            size += blockHeader.sampleCount * domainSampleSize;
            break;
        case DomainKind::None:
            break;
    }

    return size;
}


bool isFloatingPointSampleType(SampleType sampleType)
{
    return sampleType == SampleType::Float32 || sampleType == SampleType::Float64;
}

double readTickAsDouble(const void* raw, SampleType sampleType)
{
    if (sampleType == SampleType::Float32)
    {
        float value{};
        std::memcpy(&value, raw, sizeof(value));
        return value;
    }

    if (sampleType == SampleType::Float64)
    {
        double value{};
        std::memcpy(&value, raw, sizeof(value));
        return value;
    }

    return static_cast<double>(readTickAsInt(raw, sampleType));
}

Int readTickAsInt(const void* raw, SampleType sampleType)
{
    return dispatchSampleType(sampleType,
                              [raw](auto probe) -> Int
                              {
                                  decltype(probe) value{};
                                  std::memcpy(&value, raw, sizeof(value));
                                  return static_cast<Int>(value);
                              });
}

void writeTickAsInt(void* raw, SampleType sampleType, Int tick)
{
    dispatchSampleType(sampleType,
                       [raw, tick](auto probe) -> int
                       {
                           const auto value = static_cast<decltype(probe)>(tick);
                           std::memcpy(raw, &value, sizeof(value));
                           return 0;
                       });
}

void writeTickAsDouble(void* raw, SampleType sampleType, double tick)
{
    if (sampleType == SampleType::Float32)
    {
        const auto value = static_cast<float>(tick);
        std::memcpy(raw, &value, sizeof(value));
        return;
    }

    if (sampleType == SampleType::Float64)
    {
        std::memcpy(raw, &tick, sizeof(tick));
        return;
    }

    throw std::runtime_error("writeTickAsDouble called for an integer domain sample type");
}

double tickResolutionSeconds(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return 0.0;

    const auto resolution = descriptor.getTickResolution();
    if (!resolution.assigned())
        return 0.0;

    const auto denominator = resolution.getDenominator();
    if (denominator == 0)
        return 0.0;

    return static_cast<double>(resolution.getNumerator()) / static_cast<double>(denominator);
}

}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
