#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <opendaq/custom_log.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/signal_file_writer.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

using namespace file_format;

namespace
{

/*!
 * The share of the queue's limit at which it is reported as filling up, and the share it has to
 * drain back to before that report is withdrawn. The gap between them keeps a queue hovering
 * around the mark from reporting itself over and over.
 */
constexpr double QUEUE_HIGH_WATERMARK_RATIO = 0.5;
constexpr double QUEUE_LOW_WATERMARK_RATIO = 0.3;

/*!
 * What one queued packet is counted as costing besides its samples. It covers the packet's own
 * bookkeeping, and it bounds the number of packets a queue can hold even when they carry no
 * buffer of their own, as rule-generated packets do.
 */
constexpr std::uint64_t QUEUED_PACKET_OVERHEAD_BYTES = 256;


/*!
 * Converts a signal name into something usable as a filename by replacing every character which
 * is not alphanumeric, a hyphen or a period with an underscore, and trimming the result. Falls
 * back to "signal" if nothing usable remains.
 */
std::string sanitizeForFilename(const StringPtr& name)
{
    std::string result = name.assigned() ? name.toStdString() : std::string();

    for (auto& ch : result)
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '-' && ch != '.')
            ch = '_';

    const auto first = result.find_first_not_of("_-.");
    const auto last = result.find_last_not_of("_-.");
    if (first == std::string::npos)
        return "signal";

    return result.substr(first, last - first + 1);
}

std::string localTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return stream.str();
}

bool isLinearRule(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return false;

    const auto rule = descriptor.getRule();
    return rule.assigned() && rule.getType() == DataRuleType::Linear;
}

/*!
 * Returns a descriptor describing samples which have already had their rule and post scaling
 * applied: the sample type is unchanged, but the rule becomes explicit and the scaling is
 * dropped, so that a reader reproduces the stored values as-is instead of transforming them a
 * second time.
 */
DataDescriptorPtr materializedDescriptor(const DataDescriptorPtr& descriptor)
{
    return DataDescriptorBuilderCopy(descriptor).setRule(ExplicitDataRule()).setPostScaling(nullptr).build();
}

/*!
 * Writes the domain value of a block's first sample, converted to the domain sample type. Used
 * for implicit domains, where the remaining ticks follow from the linear rule in the header.
 */
void writeDomainStart(std::ostream& stream, const NumberPtr& offset, SampleType sampleType)
{
    const auto writeAs = [&stream](auto value) { stream.write(reinterpret_cast<const char*>(&value), sizeof(value)); };

    switch (sampleType)
    {
        case SampleType::Int8:
            writeAs(static_cast<std::int8_t>(offset.getIntValue()));
            break;
        case SampleType::Int16:
            writeAs(static_cast<std::int16_t>(offset.getIntValue()));
            break;
        case SampleType::Int32:
            writeAs(static_cast<std::int32_t>(offset.getIntValue()));
            break;
        case SampleType::Int64:
            writeAs(static_cast<std::int64_t>(offset.getIntValue()));
            break;
        case SampleType::UInt8:
            writeAs(static_cast<std::uint8_t>(offset.getIntValue()));
            break;
        case SampleType::UInt16:
            writeAs(static_cast<std::uint16_t>(offset.getIntValue()));
            break;
        case SampleType::UInt32:
            writeAs(static_cast<std::uint32_t>(offset.getIntValue()));
            break;
        case SampleType::UInt64:
            writeAs(static_cast<std::uint64_t>(offset.getIntValue()));
            break;
        case SampleType::Float32:
            writeAs(static_cast<float>(offset.getFloatValue()));
            break;
        case SampleType::Float64:
            writeAs(static_cast<double>(offset.getFloatValue()));
            break;
        default:
            throw std::runtime_error("Unsupported domain sample type for an implicit domain");
    }
}

}

bool SignalFileWriter::Layout::sameAs(const Layout& other) const
{
    if (domainKind != other.domainKind || materializeValues != other.materializeValues ||
        materializeDomain != other.materializeDomain)
        return false;

    if (header.valueDescriptor != other.header.valueDescriptor)
        return false;

    if (header.domainDescriptor.assigned() != other.header.domainDescriptor.assigned())
        return false;

    return !header.domainDescriptor.assigned() || header.domainDescriptor == other.header.domainDescriptor;
}

std::string SignalFileWriter::buildFilenameStem(const StringPtr& signalName)
{
    return sanitizeForFilename(signalName) + "_" + localTimestamp();
}

fs::path SignalFileWriter::partPath(const fs::path& directory, const std::string& filenameStem, Int partIndex)
{
    std::ostringstream filename;
    filename << filenameStem << '_' << std::setfill('0') << std::setw(4) << partIndex << FILE_EXTENSION;

    return directory / filename.str();
}

SignalFileWriter::SignalFileWriter(const fs::path& directory,
                                   const SignalPtr& signal,
                                   std::string filenameStem,
                                   std::uint64_t maxFileSizeBytes,
                                   std::uint64_t sampleLimit,
                                   std::uint64_t maxQueuedBytes,
                                   std::function<void()> onStateChanged,
                                   const LoggerComponentPtr& loggerComponent)
    : directory(directory)
    , signalName(signal.getName())
    , signalGlobalId(signal.getGlobalId())
    , maxFileSizeBytes(maxFileSizeBytes)
    , sampleLimit(sampleLimit)
    , maxQueuedBytes(maxQueuedBytes)
    , highWatermark(static_cast<std::uint64_t>(maxQueuedBytes * QUEUE_HIGH_WATERMARK_RATIO))
    , lowWatermark(static_cast<std::uint64_t>(maxQueuedBytes * QUEUE_LOW_WATERMARK_RATIO))
    , onStateChanged(std::move(onStateChanged))
    , filenameStem(std::move(filenameStem))
    , loggerComponent(loggerComponent)
{
    thread = std::thread(&SignalFileWriter::threadMain, this);
}

SignalFileWriter::~SignalFileWriter()
{
    close();
}

void SignalFileWriter::close()
{
    {
        std::lock_guard lock(mutex);
        stopRequested = true;
    }
    cv.notify_one();

    if (thread.joinable())
        thread.join();
}

std::uint64_t SignalFileWriter::queuedBytesOf(const PacketPtr& packet)
{
    auto bytes = QUEUED_PACKET_OVERHEAD_BYTES;

    if (packet.assigned() && packet.getType() == PacketType::Data)
    {
        const DataPacketPtr dataPacket = packet;
        bytes += dataPacket.getRawDataSize();

        const auto domainPacket = dataPacket.getDomainPacket();
        if (domainPacket.assigned())
            bytes += domainPacket.getRawDataSize();
    }

    return bytes;
}

void SignalFileWriter::post(const PacketPtr& packet)
{
    bool stateChanged = false;

    {
        std::lock_guard lock(mutex);

        // Once the queue has overflowed the writer takes nothing more: what is already queued is
        // being written out, and adding to it would only delay the end of a recording which has
        // a hole in it either way.
        if (finished || stopRequested || queueOverflowed)
            return;

        const auto bytes = queuedBytesOf(packet);

        // A packet arriving at an empty queue is always taken, so that a limit smaller than a
        // single packet still records something instead of failing on the first one.
        if (maxQueuedBytes != 0 && !queue.empty() && queuedBytes + bytes > maxQueuedBytes)
        {
            if (!queueOverflowed)
            {
                queueOverflowed = true;
                stateChanged = true;
            }
        }
        else
        {
            queue.push({packet, bytes});
            queuedBytes += bytes;

            if (maxQueuedBytes != 0 && !queueHigh && queuedBytes >= highWatermark)
            {
                queueHigh = true;
                stateChanged = true;
            }
        }
    }

    cv.notify_one();

    if (stateChanged)
        notifyStateChanged();
}

fs::path SignalFileWriter::getCurrentFilename() const
{
    std::lock_guard lock(mutex);
    return currentFilename;
}

SignalFileWriter::Status SignalFileWriter::getStatus() const
{
    std::lock_guard lock(mutex);

    Status status;
    status.finished = finished;
    status.failed = !failureMessage.empty();
    status.queueHigh = queueHigh;
    status.message = failureMessage;

    return status;
}

void SignalFileWriter::notifyStateChanged() const
{
    if (onStateChanged)
        onStateChanged();
}

void SignalFileWriter::threadMain()
{
    while (true)
    {
        PacketPtr packet;
        bool stateChanged = false;

        {
            std::unique_lock lock(mutex);
            cv.wait(lock, [this] { return stopRequested || queueOverflowed || !queue.empty(); });

            // Drain whatever is still queued before exiting, so that neither stopping a recording
            // nor a queue which filled up truncates the packets already accepted.
            if (queue.empty())
                break;

            packet = std::move(queue.front().packet);
            queuedBytes -= queue.front().bytes;
            queue.pop();

            if (queueHigh && queuedBytes <= lowWatermark)
            {
                queueHigh = false;
                stateChanged = true;
            }
        }

        if (stateChanged)
            notifyStateChanged();

        processPacket(packet);
    }

    closeFile();

    // The queue is empty by now, so everything accepted before it filled up has been written and
    // the file ends at a packet boundary.
    if (queueOverflowed)
    {
        LOG_W("File recorder: recording of signal {} stopped: more data was queued than could be written", signalGlobalId)
        finish("More data was queued than could be written", false);
    }
}

void SignalFileWriter::processPacket(const PacketPtr& packet)
{
    // finished is only ever written by this thread, so it is read here without the mutex.
    if (failed || finished || !packet.assigned())
        return;

    try
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;
            case PacketType::Data:
                processDataPacket(packet);
                break;
            default:
                break;
        }
    }
    catch (const std::exception& e)
    {
        fail(e.what());
    }
}

void SignalFileWriter::processEventPacket(const EventPacketPtr& packet)
{
    // The layout of every file is derived from the data packets themselves, so a descriptor
    // change only needs to close the current file here; the next data packet opens a new one
    // with the new descriptors.
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        closeFile();
}

void SignalFileWriter::processDataPacket(const DataPacketPtr& packet)
{
    auto sampleCount = static_cast<std::uint64_t>(packet.getSampleCount());
    if (sampleCount == 0)
        return;

    if (sampleLimit != 0)
    {
        if (samplesWritten >= sampleLimit)
            return;

        // Only the samples still missing are taken from the packet crossing the limit, so that
        // the recording holds exactly the requested number of them.
        sampleCount = std::min(sampleCount, sampleLimit - samplesWritten);
    }

    const Layout layout = buildLayout(packet);

    if (!file.is_open() || !currentLayout.sameAs(layout))
    {
        closeFile();
        openNextFile(layout);
    }

    const BlockHeader blockHeader{layout.domainKind, sampleCount};
    writeBlockHeader(file, blockHeader);

    const auto domainPacket = packet.getDomainPacket();

    if (layout.domainKind == DomainKind::Implicit)
        writeDomainStart(file, domainPacket.getOffset(), layout.header.domainDescriptor.getSampleType());

    const auto* valueData = layout.materializeValues ? static_cast<const char*>(packet.getData())
                                                     : static_cast<const char*>(packet.getRawData());
    const auto valueSize = static_cast<std::size_t>(sampleCount) * layout.valueSampleSize;
    file.write(valueData, static_cast<std::streamsize>(valueSize));

    if (layout.domainKind == DomainKind::Explicit)
    {
        const auto* domainData = layout.materializeDomain ? static_cast<const char*>(domainPacket.getData())
                                                          : static_cast<const char*>(domainPacket.getRawData());
        file.write(domainData, static_cast<std::streamsize>(static_cast<std::size_t>(sampleCount) * layout.domainSampleSize));
    }

    bytesWritten += sizeof(std::uint8_t) + sizeof(std::uint64_t) +
                    blockPayloadSize(blockHeader, layout.valueSampleSize, layout.domainSampleSize);
    samplesWritten += sampleCount;

    if (sampleLimit != 0 && samplesWritten >= sampleLimit)
    {
        LOG_I("File recorder: signal {} reached its limit of {} samples", signalGlobalId, sampleLimit)
        closeFile();
        finish("", false);
        return;
    }

    if (maxFileSizeBytes != 0 && bytesWritten >= maxFileSizeBytes)
        closeFile();
}

SignalFileWriter::Layout SignalFileWriter::buildLayout(const DataPacketPtr& packet) const
{
    Layout layout;

    layout.header.signalName = signalName;
    layout.header.signalGlobalId = signalGlobalId;

    const auto valueDescriptor = packet.getDataDescriptor();
    if (!valueDescriptor.assigned())
        throw std::runtime_error("Data packet has no value descriptor");

    layout.materializeValues = packet.getRawData() == nullptr;
    layout.header.valueDescriptor = layout.materializeValues ? materializedDescriptor(valueDescriptor) : valueDescriptor;
    layout.valueSampleSize = layout.header.valueDescriptor.getRawSampleSize();
    if (layout.valueSampleSize == 0)
        throw std::runtime_error("Cannot determine the size of a value sample");

    const auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned())
        return layout;

    const auto domainDescriptor = domainPacket.getDataDescriptor();
    if (!domainDescriptor.assigned())
        return layout;

    if (domainPacket.getRawData() != nullptr)
    {
        layout.domainKind = DomainKind::Explicit;
        layout.header.domainDescriptor = domainDescriptor;
    }
    else if (isLinearRule(domainDescriptor))
    {
        layout.domainKind = DomainKind::Implicit;
        layout.header.domainDescriptor = domainDescriptor;
    }
    else
    {
        // An implicit domain which is not linear cannot be reduced to a start value, so its ticks
        // are calculated once here and stored explicitly.
        layout.domainKind = DomainKind::Explicit;
        layout.materializeDomain = true;
        layout.header.domainDescriptor = materializedDescriptor(domainDescriptor);
    }

    layout.domainSampleSize = layout.domainKind == DomainKind::Implicit
                                  ? getSampleSize(layout.header.domainDescriptor.getSampleType())
                                  : layout.header.domainDescriptor.getRawSampleSize();
    if (layout.domainSampleSize == 0)
        throw std::runtime_error("Cannot determine the size of a domain sample");

    return layout;
}

void SignalFileWriter::openNextFile(const Layout& layout)
{
    fs::create_directories(directory);

    Layout nextLayout = layout;
    nextLayout.header.partIndex = ++partIndex;
    nextLayout.header.createdAt = toIso8601(std::chrono::system_clock::now());

    const fs::path path = partPath(directory, filenameStem, partIndex);

    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(path, std::ios::binary | std::ios::trunc);

    bytesWritten = writeHeader(file, nextLayout.header);
    currentLayout = std::move(nextLayout);

    {
        std::lock_guard lock(mutex);
        currentFilename = path;
    }

    LOG_I("File recorder: recording signal {} to {}", signalGlobalId, path.string())
}

void SignalFileWriter::closeFile()
{
    if (file.is_open())
    {
        // Closing happens on the thread's exit path too, so it reports a flush failure rather
        // than throwing. fail() calls this, so it must never call fail() back.
        file.exceptions(std::ios::goodbit);
        file.close();

        if (file.fail())
            LOG_W("File recorder: failed to flush and close {}", currentFilename.string())

        file.clear();
    }

    bytesWritten = 0;
    currentLayout = Layout{};

    std::lock_guard lock(mutex);
    currentFilename.clear();
}

void SignalFileWriter::fail(const std::string& message)
{
    LOG_W("File recorder: recording of signal {} stopped: {}", signalGlobalId, message)

    closeFile();

    failed = true;

    // Nothing more can be written, so the packets still queued are let go of rather than kept
    // alive for a file which is closed for good.
    finish(message, true);
}

void SignalFileWriter::finish(const std::string& message, bool dropQueued)
{
    {
        std::lock_guard lock(mutex);
        if (finished)
            return;

        finished = true;
        failureMessage = message;

        if (dropQueued)
        {
            std::queue<QueuedPacket> empty;
            queue.swap(empty);
            queuedBytes = 0;
            queueHigh = false;
        }
    }

    // Called outside the mutex: the recorder reacts to it by destroying this writer, which joins
    // the very thread running this callback only after it has returned.
    notifyStateChanged();
}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
