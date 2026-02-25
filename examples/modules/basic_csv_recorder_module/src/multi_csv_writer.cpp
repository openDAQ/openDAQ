#include <basic_csv_recorder_module/multi_csv_writer.h>
#include <opendaq/custom_log.h>
#include <boost/algorithm/string.hpp>

#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

namespace
{
std::string getValueName(const StringPtr& signalName, const DataDescriptorPtr& descriptor)
{
    if (!signalName.assigned())
        return "Value";

    std::string unit = MultiCsvWriter::unitLabel(descriptor);
    if (unit.length() == 0)
    {
        return signalName;
    }
    return fmt::format("{} ({})", signalName.toStdString(), unit);
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

bool MultiCsvWriter::DomainMetadata::operator==(const DomainMetadata& rhs)
{
    return origin == rhs.origin && unitName == rhs.unitName && tickResolution == rhs.tickResolution && ruleStart == rhs.ruleStart &&
           ruleDelta == rhs.ruleDelta && referenceDomainOffset == rhs.referenceDomainOffset &&
           referenceDomainTimeProtocol == rhs.referenceDomainTimeProtocol;
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
}

MultiCsvWriter::~MultiCsvWriter()
{
    std::unique_lock lock(mutex);
    exitFlag = true;
    lock.unlock();
    cv.notify_all();

    writerThread.join();
}

void MultiCsvWriter::setHeaderInformation(const DataDescriptorPtr& domainDescriptor,
                                          const ListPtr<IDataDescriptor>& valueDescriptors,
                                          const ListPtr<IString>& signalNames,
                                          bool writeDomain)
{
    if (valueDescriptors.getCount() != signalNames.getCount())
    {
        valueNames.clear();
        return;
    }

    metadata = getDomainMetadata(domainDescriptor);

    valueNames.clear();
    valueNames.reserve(valueDescriptors.getCount());

    for (SizeT i = 0; i < valueDescriptors.getCount(); ++i)
    {
        valueNames.push_back(getValueName(signalNames.getItemAt(i), valueDescriptors.getItemAt(i)));
    }
    writeDomainColumn = writeDomain;
}

void MultiCsvWriter::writeSamples(std::vector<std::unique_ptr<double[]>>&& jaggedArray, size_t count, Int packetOffset)
{
    std::unique_lock lock(mutex);
    queue.emplace(JaggedBuffer{count, std::move(jaggedArray), packetOffset});
    lock.unlock();
    cv.notify_all();
}

std::string MultiCsvWriter::unitLabel(const DataDescriptorPtr& descriptor)
{
    auto unit = descriptor.getUnit();
    if (!unit.assigned())
    {
        return "";
    }

    if (auto symbol = unit.getSymbol(); symbol.assigned())
        return symbol.toStdString();
    else if (auto unitName = unit.getName(); unitName.assigned())
        return unitName.toStdString();
    return "";
}

void MultiCsvWriter::threadLoop()
{
    while (true)
    {
        std::unique_lock lock{mutex};
        cv.wait(lock, [this]() { return !queue.empty() || exitFlag; });

        if (exitFlag && queue.empty())
        {
            return;
        }

        JaggedBuffer samples = std::move(queue.front());
        queue.pop();
        lock.unlock();

        if (!headersWritten)
        {
            writeHeaders(samples.packetOffset, writeDomainColumn);
        }

        const size_t signalNum = samples.buffers.size();
        // Write to file
        for (size_t i = 0; i < samples.count; ++i)
        {
            if (writeDomainColumn)
            {
                outFile << (samples.packetOffset + i * metadata.ruleDelta) << ",";
            }
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

void MultiCsvWriter::writeHeaders(Int firstPacketOffset, bool writeDomainColumn)
{
    outFile.open(filepath);

    Int startingTick = metadata.ruleStart + metadata.referenceDomainOffset + firstPacketOffset;
    std::string metadataHeader = fmt::format("# domain;unit={};resolution={}/{};delta={};origin={};starting_tick={};",
                                             metadata.unitName,
                                             metadata.tickResolution.first,
                                             metadata.tickResolution.second,
                                             metadata.ruleDelta,
                                             metadata.origin,
                                             startingTick);
    outFile << metadataHeader << "\n";
    if (writeDomainColumn)
    {
        outFile << "Domain,";
    }
    for (size_t i = 0; i < valueNames.size(); ++i)
    {
        outFile << quote(valueNames[i]);

        if (i != valueNames.size() - 1)
        {
            outFile << ",";
        }
    }
    outFile << "\n";
    headersWritten = true;
}

MultiCsvWriter::DomainMetadata MultiCsvWriter::getDomainMetadata(const DataDescriptorPtr& domainDescriptor)
{
    DomainMetadata metadata{"N/A", "N/A", {1, 1}, 0, 1, 0, TimeProtocol::Unknown};

    if (auto domainOrigin = domainDescriptor.getOrigin(); domainOrigin.assigned())
    {
        metadata.origin = domainOrigin.toStdString();
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
        metadata.tickResolution = {resolution.getNumerator(), resolution.getDenominator()};

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
        metadata.unitName = unit.getName().toStdString();
    }

    return metadata;
}

std::string MultiCsvWriter::getFilename()
{
    return filepath.string();
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE