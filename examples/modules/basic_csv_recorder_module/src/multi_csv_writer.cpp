#include <basic_csv_recorder_module/multi_csv_writer.h>
#include <opendaq/custom_log.h>
#include <boost/algorithm/string.hpp>

#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

namespace
{
std::string unitInfo(const DataDescriptorPtr& descriptor)
{
    auto unit = descriptor.getUnit();
    if (!unit.assigned())
    {
        return "";
    }

    if (auto symbol = unit.getSymbol(); symbol.assigned())
        return fmt::format("({})", symbol);
    else if (auto unitName = unit.getName(); unitName.assigned())
        return fmt::format("({})", unitName);

    return "";
}

std::string getDomainName(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return "N/A";

    std::ostringstream stream;
    stream << descriptor.getName();
    return stream.str();
}

std::string getValueName(const StringPtr& signalName, const DataDescriptorPtr& descriptor)
{
    if (!signalName.assigned())
        return "Value";

    std::ostringstream stream;
    stream << signalName << " " << unitInfo(descriptor);
    return stream.str();
}

std::string getValueMetadataLine(const DataDescriptorPtr& descriptor)
{
    std::list<std::string> parts;

    if (auto unit = descriptor.getUnit(); unit.assigned())
        parts.push_back("unit=" + unit.getName());

    return boost::algorithm::join(parts, ";");
}

/*!
 * @brief Quotes a header string by enclosing it in double-quotes.
 *
 * Double-quotes within the @p header itself are escaped by replacing them with two
 * double-quotes.
 *
 * @param header The string to quote.
 * @return The quoted string.
 */
std::string quote(const std::string& header)
{
    std::ostringstream quoted;
    quoted << '"';
    for (const auto& ch : header)
        if (ch == '"')
            quoted << "\"\"";
        else
            quoted << ch;
    quoted << '"';
    return quoted.str();
}
}

MultiCsvWriter::MultiCsvWriter(const fs::path& file)
    : exitFlag(false)
    , headersWritten(false)
    , filepath(file)
    , writerThread([this]() { this->threadLoop(); })
{
    if (filepath.has_parent_path())
    {
        fs::create_directories(filepath.parent_path());
    }

    outFile.exceptions(std::ios::failbit | std::ios::badbit);
    outFile.open(filepath);
}

MultiCsvWriter::~MultiCsvWriter()
{
    std::unique_lock lock(mutex);
    exitFlag = true;
    lock.unlock();
    cv.notify_all();

    writerThread.join();

    outFile.close();
    // If nothing was written, remove the file
    if (!headersWritten)
    {
        fs::remove(filepath);
    }
}

void MultiCsvWriter::setHeaderInformation(const DataDescriptorPtr& domainDescriptor,
                                          const ListPtr<IDataDescriptor>& valueDescriptors,
                                          const ListPtr<IString>& signalNames)
{
    if (valueDescriptors.getCount() != signalNames.getCount())
    {
        valueNames.clear();
        return;
    }

    domainName = getDomainName(domainDescriptor);
    metadata = getDomainMetadata(domainDescriptor);

    valueNames.clear();
    valueNames.reserve(valueDescriptors.getCount());

    for (SizeT i = 0; i < valueDescriptors.getCount(); ++i)
    {
        valueNames.push_back(getValueName(signalNames.getItemAt(i), valueDescriptors.getItemAt(i)));
    }
}

void MultiCsvWriter::writeSamples(std::vector<std::unique_ptr<double[]>>&& jaggedArray, int count, Int packetOffset)
{
    std::unique_lock lock(mutex);
    exitFlag = false;
    queue.emplace(JaggedBuffer{count, std::move(jaggedArray), packetOffset});
    lock.unlock();
    cv.notify_all();
}

void MultiCsvWriter::threadLoop()
{
    while (true)
    {
        std::unique_lock lock{mutex};
        cv.wait(lock, [this]() { return !queue.empty() || exitFlag; });

        if (exitFlag)
        {
            return;
        }

        JaggedBuffer samples = std::move(queue.front());
        queue.pop();
        lock.unlock();

        if (!headersWritten)
        {
            writeHeaders(samples.packetOffset);
            headersWritten = true;
        }

        const size_t signalNum = samples.buffers.size();
        // Write to file
        for (size_t i = 0; i < samples.count; ++i)
        {
            for (size_t signal = 0; signal < signalNum; ++signal)
            {
                double* signalSamplesPtr = samples.buffers[signal].get();
                outFile << signalSamplesPtr[i];

                if (signal != signalNum - 1)
                {
                    outFile << ",";
                }
            }
            outFile << "\n";
        }
    }
}

void MultiCsvWriter::writeHeaders(Int firstPacketOffset)
{
    Int startingTick = metadata.ruleStart + metadata.referenceDomainOffset + firstPacketOffset;
    std::string metadataHeader = fmt::format("# domain_name={};unit={};resolution={};delta={};starting_tick={};origin={};",
                                             domainName,
                                             metadata.unitName,
                                             metadata.tickResolution,
                                             metadata.ruleDelta,
                                             startingTick,
                                             metadata.origin);
    outFile << metadataHeader << "\n";
    for (size_t i = 0; i < valueNames.size(); ++i)
    {
        outFile << quote(valueNames[i]);

        if (i != valueNames.size() - 1)
        {
            outFile << ",";
        }
    }
    outFile << "\n";
}

MultiCsvWriter::DomainMetadata MultiCsvWriter::getDomainMetadata(const DataDescriptorPtr& domainDescriptor)
{
    DomainMetadata metadata{StringPtr("N/A"), StringPtr("N/A"), Ratio(1, 1), 0, 1, 0, TimeProtocol::Unknown};

    if (auto domainOrigin = domainDescriptor.getOrigin(); domainOrigin.assigned())
    {
        metadata.origin = domainOrigin;
    }
    if (auto refDomainInfo = domainDescriptor.getReferenceDomainInfo(); refDomainInfo.assigned())
    {
        if (auto refDomOffset = refDomainInfo.getReferenceDomainOffset(); refDomOffset.assigned())
        {
            metadata.referenceDomainOffset = refDomOffset;
        }
        metadata.referenceDomainTimeProtocol = refDomainInfo.getReferenceTimeProtocol();
    }

    auto resolution = domainDescriptor.getTickResolution();
    auto rule = domainDescriptor.getRule();
    if (resolution.assigned() && rule.assigned())
    {
        metadata.tickResolution = resolution;

        auto params = rule.getParameters();
        if (rule.getType() == DataRuleType::Linear && params.assigned() && params.hasKey("delta"))
        {
            if (params.hasKey("start"))
            {
                metadata.ruleStart = params["start"];
            }

            metadata.ruleDelta = params["delta"];
        }
    }
    if (auto unit = domainDescriptor.getUnit(); unit.assigned())
    {
        metadata.unitName = unit.getName();
    }

    return metadata;
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE