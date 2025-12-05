#include <basic_csv_recorder_module/multi_csv_writer.h>
#include <opendaq/custom_log.h>
#include <boost/algorithm/string.hpp>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

namespace
{
/**
 * If a signal has a Unit attached to its descriptor, writes the unit's symbol (or name, if the
 * symbol is empty) to an output stream in parentheses. If a tick resolution is specified, the
 * resolution is also included after the unit.
 *
 * @param stream The output stream to write to.
 * @param signal The signal whose unit name to write.
 *
 * @return @p stream.
 */
std::ostream& appendUnitInfo(std::ostream& stream, const DataDescriptorPtr& descriptor)
{
    bool parentheses = false;

    if (auto unit = descriptor.getUnit(); unit.assigned())
    {
        parentheses = true;
        if (auto symbol = unit.getSymbol(); symbol.assigned())
            stream << " (" << symbol;
        else if (auto unitName = unit.getName(); unitName.assigned())
            stream << " (" << unitName;
        else
            parentheses = false;
    }

    if (auto ratio = descriptor.getTickResolution(); ratio.assigned())
    {
        Int numerator = ratio.getNumerator();
        Int denominator = ratio.getDenominator();

        if (numerator != 1)
            stream << " * " << numerator;
        if (denominator != 1)
            stream << " / " << denominator;
    }

    if (parentheses)
        stream << ')';

    return stream;
}

/**
 * Gets the name of the signal's associated domain signal, for use as a CSV header. If the signal
 * has an associated domain signal, that signal's name is returned. If the signal additionally has
 * a Unit assigned to its descriptor, the unit's symbol (or name, if no symbol) is appended in
 * parentheses. Otherwise, "Domain" is returned.
 *
 * @param descriptor The data descriptor of the domain signal.
 *
 * @return The name of the domain signal associated with @p signal, or if there is no associated
 *     domain signal, "Domain".
 */
std::string getDomainName(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return "Domain";

    std::ostringstream stream;
    stream << descriptor.getName();
    appendUnitInfo(stream, descriptor);
    return stream.str();
}

/**
 * Gets the name of the signal, for use as a CSV header. If the signal has a Unit assigned to its
 * descriptor, the unit's symbol (or name, if no symbol) is appended in parentheses.
 *
 * @param descriptor The data descriptor of the value signal.
 *
 * @return The name of the signal.
 */
std::string getValueName(const DataDescriptorPtr& descriptor, SizeT serialNumber)
{
    if (!descriptor.assigned())
        return "Value";

    StringPtr name = descriptor.getName();
    if (!name.assigned() || name.getLength() == 0)
        return "Value";

    std::ostringstream stream;
    stream << descriptor.getName();
    appendUnitInfo(stream, descriptor);
    return stream.str();
}

std::string getDomainMetadataLine(const DataDescriptorPtr& domainDescriptor)
{
    std::list<std::string> parts;

    if (auto origin = domainDescriptor.getOrigin(); origin.assigned())
        parts.push_back("origin=" + origin);

    auto resolution = domainDescriptor.getTickResolution();
    auto rule = domainDescriptor.getRule();
    if (resolution.assigned() && rule.assigned())
    {
        std::int64_t numerator = resolution.getNumerator();
        std::int64_t denominator = resolution.getDenominator();

        auto params = rule.getParameters();
        if (rule.getType() == DataRuleType::Linear && params.assigned() && params.hasKey("delta"))
        {
            std::int64_t delta = params["delta"];
            std::int64_t totalNumerator = delta * numerator;
            std::int64_t commonDivisor = std::gcd(totalNumerator, denominator);

            parts.push_back("period=" + std::to_string(totalNumerator / commonDivisor) + "/" + std::to_string(denominator / commonDivisor));
        }
        else
        {
            parts.push_back("resolution=" + std::to_string(resolution.getNumerator()) + '/' + std::to_string(resolution.getDenominator()));
        }
    }
    if (auto unit = domainDescriptor.getUnit(); unit.assigned())
        parts.push_back("unit=" + unit.getName());

    return boost::algorithm::join(parts, ";");
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

MultiCsvWriter::MultiCsvWriter(const fs::path& filename)
    : exitFlag(false)
    , headersWritten(false)
    , writerThread([this]() { this->threadLoop(); })
{
    if (filename.has_parent_path())
    {
        fs::create_directories(filename.parent_path());
    }

    outFile.exceptions(std::ios::failbit | std::ios::badbit);
    outFile.open(filename);
}

MultiCsvWriter::~MultiCsvWriter()
{
    std::unique_lock lock(mutex);
    exitFlag = true;
    lock.unlock();
    cv.notify_all();

    writerThread.join();
}

void MultiCsvWriter::setHeaderInformation(const DataDescriptorPtr& domainDescriptor, const ListPtr<IDataDescriptor>& valueDescriptors)
{
    domainName = getDomainName(domainDescriptor);
    domainMetadata = getDomainMetadataLine(domainDescriptor);

    valueNames.clear();
    valueNames.reserve(valueDescriptors.getCount());
    SizeT i = 0;
    for (const auto& desc : valueDescriptors)
    {
        valueNames.push_back(getValueName(desc, i));
        ++i;
    }
}

void MultiCsvWriter::writeSamples(std::vector<std::unique_ptr<double[]>>&& jaggedArray, int count, Int offset)
{
    std::unique_lock lock(mutex);
    exitFlag = false;
    queue.emplace(JaggedBuffer{count, std::move(jaggedArray), offset});
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
            writeHeaders(samples.offset);
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

void MultiCsvWriter::writeHeaders(Int offset)
{
    outFile << "# Domain: " << "name=" << quote(domainName) << ";" << domainMetadata << ";offset=" << offset << "\n";
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

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE