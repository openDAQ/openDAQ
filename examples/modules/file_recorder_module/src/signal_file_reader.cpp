#include <algorithm>
#include <stdexcept>

#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/signal_file_reader.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

using namespace file_format;

SignalFileReader::SignalFileReader(const fs::path& path)
{
    file.open(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Cannot open recording " + path.string());

    readHeader(file, header);

    valueSampleSize = header.valueDescriptor.getRawSampleSize();
    if (valueSampleSize == 0)
        throw std::runtime_error("Recording header does not describe a readable value sample size");

    if (header.domainDescriptor.assigned())
    {
        domainSampleType = header.domainDescriptor.getSampleType();
        domainIsFloatingPoint = isFloatingPointSampleType(domainSampleType);
        domainSampleSize = getSampleSize(domainSampleType);

        const auto rule = header.domainDescriptor.getRule();
        if (rule.assigned() && rule.getType() == DataRuleType::Linear)
        {
            domainDeltaInt = rule.getParameters().get("delta");
            domainDeltaDouble = static_cast<double>(domainDeltaInt);
        }
    }

    dataStart = file.tellg();
    currentBlock = BlockHeader{};
}

const Header& SignalFileReader::getHeader() const
{
    return header;
}

bool SignalFileReader::hasDomain() const
{
    return header.domainDescriptor.assigned();
}

std::size_t SignalFileReader::getValueSampleSize() const
{
    return valueSampleSize;
}

std::size_t SignalFileReader::getDomainSampleSize() const
{
    return domainSampleSize;
}

double SignalFileReader::getTickResolutionSeconds() const
{
    return tickResolutionSeconds(header.domainDescriptor);
}

void SignalFileReader::rewind()
{
    file.clear();
    file.seekg(dataStart);

    currentBlock = BlockHeader{};
    consumedInBlock = 0;
    blockDomainStart.clear();
}

bool SignalFileReader::beginNextBlock()
{
    // Blocks are consumed in order, but a block which is handed out as several runs is read out
    // of order, because its domain samples follow all of its value samples. Seek back to the end
    // of the block just finished before looking for the next header.
    if (currentBlock.sampleCount != 0)
    {
        const std::streamoff blockEnd =
            currentBlock.domainKind == DomainKind::Explicit
                ? blockDomainBase + static_cast<std::streamoff>(currentBlock.sampleCount * domainSampleSize)
                : blockDomainBase;
        file.seekg(blockEnd);
    }

    if (!readBlockHeader(file, currentBlock))
        return false;

    consumedInBlock = 0;

    if (currentBlock.domainKind != DomainKind::None && domainSampleSize == 0)
        throw std::runtime_error("Recording has domain samples but its header has no domain descriptor");

    if (currentBlock.domainKind == DomainKind::Implicit)
    {
        blockDomainStart.resize(domainSampleSize);
        file.read(blockDomainStart.data(), static_cast<std::streamsize>(domainSampleSize));
        if (static_cast<std::size_t>(file.gcount()) != domainSampleSize)
            throw std::runtime_error("Truncated recording: incomplete block domain start");
    }

    blockValueBase = file.tellg();
    blockDomainBase = blockValueBase + static_cast<std::streamoff>(currentBlock.sampleCount * valueSampleSize);

    return true;
}

bool SignalFileReader::readRun(std::size_t maxSamples, Run& run)
{
    if (maxSamples == 0)
        throw std::runtime_error("readRun requires a positive sample limit");

    if (consumedInBlock >= currentBlock.sampleCount && !beginNextBlock())
        return false;

    const auto count = std::min<std::size_t>(maxSamples, currentBlock.sampleCount - consumedInBlock);

    run.domainKind = currentBlock.domainKind;
    run.sampleCount = count;

    run.valueData.resize(count * valueSampleSize);
    file.seekg(blockValueBase + static_cast<std::streamoff>(consumedInBlock * valueSampleSize));
    file.read(run.valueData.data(), static_cast<std::streamsize>(run.valueData.size()));
    if (static_cast<std::size_t>(file.gcount()) != run.valueData.size())
        throw std::runtime_error("Truncated recording: incomplete value samples");

    switch (currentBlock.domainKind)
    {
        case DomainKind::None:
            run.domainData.clear();
            break;

        case DomainKind::Implicit:
        {
            // The run starts partway into the block when the block was split, so its first tick
            // has to be advanced by the rule's delta.
            run.domainData = blockDomainStart;
            if (consumedInBlock != 0)
            {
                if (domainIsFloatingPoint)
                {
                    const auto start = readTickAsDouble(run.domainData.data(), domainSampleType);
                    writeTickAsDouble(
                        run.domainData.data(), domainSampleType, start + static_cast<double>(consumedInBlock) * domainDeltaDouble);
                }
                else
                {
                    const auto start = readTickAsInt(run.domainData.data(), domainSampleType);
                    writeTickAsInt(run.domainData.data(), domainSampleType, start + static_cast<Int>(consumedInBlock) * domainDeltaInt);
                }
            }
            break;
        }

        case DomainKind::Explicit:
            run.domainData.resize(count * domainSampleSize);
            file.seekg(blockDomainBase + static_cast<std::streamoff>(consumedInBlock * domainSampleSize));
            file.read(run.domainData.data(), static_cast<std::streamsize>(run.domainData.size()));
            if (static_cast<std::size_t>(file.gcount()) != run.domainData.size())
                throw std::runtime_error("Truncated recording: incomplete domain samples");
            break;
    }

    consumedInBlock += count;
    return true;
}

double SignalFileReader::tickAt(const Run& run, std::size_t index) const
{
    if (run.domainKind == DomainKind::None || run.domainData.empty())
        throw std::runtime_error("Run has no domain");

    if (run.domainKind == DomainKind::Explicit)
        return readTickAsDouble(run.domainData.data() + index * domainSampleSize, domainSampleType);

    return readTickAsDouble(run.domainData.data(), domainSampleType) + static_cast<double>(index) * domainDeltaDouble;
}

Int SignalFileReader::tickAtAsInt(const Run& run, std::size_t index) const
{
    if (run.domainKind == DomainKind::None || run.domainData.empty())
        throw std::runtime_error("Run has no domain");

    if (domainIsFloatingPoint)
        throw std::runtime_error("tickAtAsInt called for a floating point domain");

    if (run.domainKind == DomainKind::Explicit)
        return readTickAsInt(run.domainData.data() + index * domainSampleSize, domainSampleType);

    return readTickAsInt(run.domainData.data(), domainSampleType) + static_cast<Int>(index) * domainDeltaInt;
}

Int SignalFileReader::getDomainDeltaInt() const
{
    return domainDeltaInt;
}

double SignalFileReader::getDomainDeltaDouble() const
{
    return domainDeltaDouble;
}

bool SignalFileReader::isDomainFloatingPoint() const
{
    return domainIsFloatingPoint;
}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
