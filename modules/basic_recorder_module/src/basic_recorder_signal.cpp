#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include <boost/algorithm/string.hpp>

#include <opendaq/opendaq.h>

#include <basic_recorder_module/basic_recorder_signal.h>
#include <basic_recorder_module/common.h>
#include <basic_recorder_module/csv_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

/**
 * Generates a CSV filename (without path) for a signal to be recorded based on the signal's
 * global ID. The ID is sanitized by converting characters not allowed in filenames to
 * underscores. Only alphanumeric characters, hyphens, periods and underscores are permitted. If
 * a file with the chosen name already exists, a Windows-style sequence number of the form " (1)"
 * (using the lowest number that results in an unused filename) is appended.
 *
 * @todo There is a race condition finding an unused filename. It is possible for two recorders
 *     to simultaneously select the same filename and then overwrite each other.
 *
 * @param path The path to the directory where the file will be created.
 * @param signal The openDAQ signal object to generate a CSV filename for.
 *
 * @returns A filename, including a ".csv" extension.
 */
static std::string getFilename(const std::filesystem::path& path, const daq::SignalPtr& signal)
{
    std::string id = signal.getGlobalId();

    for (std::size_t i = 0; i < id.length(); ++i)
    {
        auto ch = static_cast<unsigned char>(id[i]);
        if (!std::isalnum(ch) && id[i] != '-' && id[i] != '.')
            id[i] = '_';
    }

    boost::trim_if(id, boost::is_any_of("_-."));

    unsigned sequence = 0;
    std::string name = id + ".csv";

    while (std::filesystem::exists(path / name))
        name = id + " (" + std::to_string(++sequence) + ").csv";

    return name;
}

/**
 * Gets the name of the signal's associated domain signal, for use as a CSV header. If the signal
 * has an associated domain signal, that signal's name is returned. Otherwise, "Domain" is
 * returned.
 *
 * @param signal The value signal.
 *
 * @return The name of the domain signal associated with @p signal, or if there is no associated
 *     domain signal, "Domain".
 */
static std::string getDomainName(const daq::SignalPtr& signal)
{
    auto domainSignal = signal.getDomainSignal();
    return domainSignal.assigned() ? domainSignal.getName() : "Domain";
}

/**
 * Gets the name of the signal, for use as a CSV header.
 *
 * @param signal The value signal.
 *
 * @return The name of the @p signal.
 */
static std::string getValueName(const daq::SignalPtr& signal)
{
    return signal.getName();
}

BasicRecorderSignal::BasicRecorderSignal(std::filesystem::path path, const daq::SignalPtr& signal)
    : writer(path / getFilename(path, signal))
{
    writer.headers(
        getDomainName(signal).c_str(),
        getValueName(signal).c_str());
}

void BasicRecorderSignal::onPacketReceived(const daq::InputPortPtr& port)
{
    daq::PacketPtr packet;

    while ((packet = port.getConnection().dequeue()).assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Data:
                onDataPacketReceived(packet);
                break;

            default:
                break;
        }
    }
}

/**
 * Records the values in the specified data packet to the CSV file.
 *
 * @todo The current implementation uses `daq::IDataPacket::getData()` on both the value signal
 *     and the associated domain signal. Directly supporting linear-rule domain signals would
 *     avoid unnecessary in-memory expansion of linear-rule domain values.
 *
 * @tparam Sample The type of values in the value signal's data buffer.
 * @tparam Domain The type of values in the domain signal's data buffer.
 *
 * @param packet The data packet to record.
 * @param domainPacket The domain packet associated with the data packet.
 * @param writer The CSV writer to write to.
 */
template <typename Sample, typename Domain>
void writeSamples(daq::DataPacketPtr packet, daq::DataPacketPtr domainPacket, CsvWriter& writer)
{
    auto data = reinterpret_cast<const Sample *>(packet.getData());
    auto domainData = reinterpret_cast<const Domain *>(domainPacket.getData());

    auto count = packet.getSampleCount();
    if (domainPacket.getSampleCount() != count)
        return;

    for (std::size_t i = 0; i < count; ++i)
        writer.write(domainData[i], data[i]);
}

/**
 * Records the values in the specified data packet to the CSV file. This function examines the
 * sample type of the associated domain packet and calls the appropriate specialization of the
 * two-parameter writeSamples() function above.
 *
 * @tparam Sample The type of values in the value signal's data buffer.
 *
 * @param packet The data packet to record.
 * @param writer The CSV writer to write to.
 */
template <typename Sample>
void writeSamples(daq::DataPacketPtr packet, CsvWriter& writer)
{
    auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned())
        return;

    auto domainDescriptor = domainPacket.getDataDescriptor();
    if (!domainDescriptor.assigned())
        return;

    switch (domainDescriptor.getSampleType())
    {
        case SampleType::Int8:      writeSamples<Sample, std::int8_t>(packet, domainPacket, writer); return;
        case SampleType::Int16:     writeSamples<Sample, std::int16_t>(packet, domainPacket, writer); return;
        case SampleType::Int32:     writeSamples<Sample, std::int32_t>(packet, domainPacket, writer); return;
        case SampleType::Int64:     writeSamples<Sample, std::int64_t>(packet, domainPacket, writer); return;
        case SampleType::UInt8:     writeSamples<Sample, std::uint8_t>(packet, domainPacket, writer); return;
        case SampleType::UInt16:    writeSamples<Sample, std::uint16_t>(packet, domainPacket, writer); return;
        case SampleType::UInt32:    writeSamples<Sample, std::uint32_t>(packet, domainPacket, writer); return;
        case SampleType::UInt64:    writeSamples<Sample, std::uint64_t>(packet, domainPacket, writer); return;
        case SampleType::Float32:   writeSamples<Sample, float>(packet, domainPacket, writer); return;
        case SampleType::Float64:   writeSamples<Sample, double>(packet, domainPacket, writer); return;
        default: break;
    }
}

void BasicRecorderSignal::onDataPacketReceived(daq::DataPacketPtr packet)
{
    auto descriptor = packet.getDataDescriptor();
    if (!descriptor.assigned())
        return;

    switch (descriptor.getSampleType())
    {
        case SampleType::Int8:      writeSamples<std::int8_t>(packet, writer); return;
        case SampleType::Int16:     writeSamples<std::int16_t>(packet, writer); return;
        case SampleType::Int32:     writeSamples<std::int32_t>(packet, writer); return;
        case SampleType::Int64:     writeSamples<std::int64_t>(packet, writer); return;
        case SampleType::UInt8:     writeSamples<std::uint8_t>(packet, writer); return;
        case SampleType::UInt16:    writeSamples<std::uint16_t>(packet, writer); return;
        case SampleType::UInt32:    writeSamples<std::uint32_t>(packet, writer); return;
        case SampleType::UInt64:    writeSamples<std::uint64_t>(packet, writer); return;
        case SampleType::Float32:   writeSamples<float>(packet, writer); return;
        case SampleType::Float64:   writeSamples<double>(packet, writer); return;
        default: break;
    }
}

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
