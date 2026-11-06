#include <coretypes/ctutils.h>
#include <coretypes/ratio_ptr.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/multi_reader2_domain.h>
#include <date/date.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

namespace
{
    constexpr std::int64_t int64Max = std::numeric_limits<std::int64_t>::max();
    constexpr std::int64_t int64Min = std::numeric_limits<std::int64_t>::min();

    bool isIntegerType(SampleType type)
    {
        switch (type)
        {
            case SampleType::Int8:
            case SampleType::UInt8:
            case SampleType::Int16:
            case SampleType::UInt16:
            case SampleType::Int32:
            case SampleType::UInt32:
            case SampleType::Int64:
            case SampleType::UInt64:
                return true;
            default:
                return false;
        }
    }

    bool digits(const std::string& text)
    {
        return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; });
    }
}

std::optional<std::int64_t> checkedAdd(std::int64_t a, std::int64_t b)
{
    if ((b > 0 && a > int64Max - b) || (b < 0 && a < int64Min - b))
        return std::nullopt;
    return a + b;
}

std::optional<std::int64_t> checkedSub(std::int64_t a, std::int64_t b)
{
    if ((b < 0 && a > int64Max + b) || (b > 0 && a < int64Min + b))
        return std::nullopt;
    return a - b;
}

std::optional<std::int64_t> checkedMul(std::int64_t a, std::int64_t b)
{
    if (a > 0 ? (b > 0 ? a > int64Max / b : b < int64Min / a) : (b > 0 ? a < int64Min / b : a != 0 && b < int64Max / a))
        return std::nullopt;
    return a * b;
}

bool exceeds(std::int64_t a, std::int64_t b, std::int64_t limit)
{
    const auto difference = checkedSub(a, b);
    return difference.has_value() ? *difference > limit : a > b;
}

std::optional<std::int64_t> parseOrigin(const std::string& origin)
{
    std::string text = origin;
    if (text.size() == 4)
        text += "-01-01";
    else if (text.size() == 7)
        text += "-01";
    const auto time = text.find('T');
    if (time == std::string::npos)
        text += "T00:00:00";

    // The offset after the time, read here: the date library's %z takes +hh and stops at a colon, so +01:30 would read as
    // +01:00
    std::int64_t offsetSeconds = 0;
    const auto zone = text.find_first_of("Z+-", time == std::string::npos ? text.size() : time);
    if (zone != std::string::npos)
    {
        const std::string suffix = text.substr(zone);
        text.erase(zone);
        if (suffix != "Z")
        {
            std::string hhmm = suffix.substr(1);
            if (hhmm.size() == 5 && hhmm[2] == ':')
                hhmm.erase(2, 1);
            if (!digits(hhmm) || (hhmm.size() != 2 && hhmm.size() != 4))
                return std::nullopt;
            const std::int64_t hours = std::stoll(hhmm.substr(0, 2));
            const std::int64_t minutes = hhmm.size() == 4 ? std::stoll(hhmm.substr(2, 2)) : 0;
            if (hours > 23 || minutes > 59)
                return std::nullopt;
            offsetSeconds = (suffix[0] == '-' ? -1 : 1) * (hours * 3600 + minutes * 60);
        }
    }

    std::istringstream in(text);
    std::chrono::system_clock::time_point local;
    date::from_stream(in, "%FT%T", local);
    if (in.fail() || in.peek() != std::char_traits<char>::eof())
        return std::nullopt;
    const auto sinceEpoch = local.time_since_epoch();
    if (sinceEpoch % std::chrono::seconds(1) != std::chrono::system_clock::duration::zero())
        return std::nullopt;
    return std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch).count() - offsetSeconds;
}

std::int64_t TickDomain::samplesIn(std::int64_t seconds) const
{
    return checkedMul(rate, seconds).value_or(int64Max);
}

std::int64_t TickDomain::ticksIn(std::int64_t seconds) const
{
    // rate * delta is the tick resolution's denominator over its numerator, so it fits
    return checkedMul(rate * delta, seconds).value_or(int64Max);
}

std::optional<TickDomain> parseDomain(const DataDescriptorPtr& domain)
{
    if (!domain.assigned() || !isIntegerType(domain.getSampleType()))
        return std::nullopt;
    const auto dimensions = domain.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() > 0)
        return std::nullopt;
    const auto unit = domain.getUnit();
    if (!unit.assigned() || unit.getSymbol() != "s")
        return std::nullopt;

    // Rule: linear, with an integer delta above 0 and an integer start
    const auto rule = domain.getRule();
    if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
        return std::nullopt;
    const auto parameters = rule.getParameters();
    if (!parameters.assigned() || !parameters.hasKey("delta") || !parameters.hasKey("start"))
        return std::nullopt;
    const BaseObjectPtr delta = parameters.get("delta");
    const BaseObjectPtr start = parameters.get("start");
    if (delta.getCoreType() != ctInt || static_cast<std::int64_t>(delta) <= 0 || start.getCoreType() != ctInt)
        return std::nullopt;

    TickDomain ticks;
    ticks.delta = delta;
    ticks.start = start;

    // Sampling rate: an integer number of samples per second
    const auto resolution = domain.getTickResolution();
    if (!resolution.assigned() || resolution.getNumerator() <= 0 || resolution.getDenominator() <= 0)
        return std::nullopt;
    ticks.resolutionNum = resolution.getNumerator();
    ticks.resolutionDen = resolution.getDenominator();
    const auto secondsPerSample = checkedMul(ticks.resolutionNum, ticks.delta);
    if (!secondsPerSample || ticks.resolutionDen % *secondsPerSample != 0)
        return std::nullopt;
    ticks.rate = ticks.resolutionDen / *secondsPerSample;

    const auto originText = domain.getOrigin();
    const auto origin = originText.assigned() && originText.getLength() > 0 ? parseOrigin(originText.toStdString()) : std::nullopt;
    if (!origin)
        return std::nullopt;
    ticks.originSeconds = *origin;
    return ticks;
}

std::optional<std::int64_t> TickMap::firstTick(std::int64_t packetOffset) const
{
    const auto tick = checkedAdd(packetOffset, start);
    if (!tick || *tick % q != 0)
        return std::nullopt;
    const auto scaled = checkedMul(*tick / q, p);
    return scaled ? checkedAdd(*scaled, offset) : std::nullopt;
}

std::optional<TickMap> relate(const TickDomain& input, const TickDomain& main)
{
    if (input.rate != main.rate)
        return std::nullopt;

    // (input num / input den) / (main num / main den), reduced
    const auto p = checkedMul(input.resolutionNum, main.resolutionDen);
    const auto q = checkedMul(input.resolutionDen, main.resolutionNum);
    if (!p || !q)
        return std::nullopt;
    const std::int64_t divisor = std::gcd(*p, *q);

    // The origins' difference in seconds, over main num / main den seconds per tick
    const auto seconds = checkedSub(input.originSeconds, main.originSeconds);
    const auto scaled = seconds ? checkedMul(*seconds, main.resolutionDen) : std::nullopt;
    if (!scaled || *scaled % main.resolutionNum != 0)
        return std::nullopt;

    return TickMap{*p / divisor, *q / divisor, *scaled / main.resolutionNum, input.start, main.delta};
}

}

END_NAMESPACE_OPENDAQ
