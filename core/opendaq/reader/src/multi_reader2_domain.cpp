#include <coretypes/ratio_ptr.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/multi_reader2_domain.h>
#include <opendaq/reader_utils.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

namespace
{

// A signed 128-bit value as sign and magnitude; enough for products of two Int64 and a floor division by an Int64
struct Wide
{
    bool negative = false;
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;
};

std::uint64_t magnitude(std::int64_t v)
{
    return v < 0 ? ~static_cast<std::uint64_t>(v) + 1 : static_cast<std::uint64_t>(v);
}

Wide multiply(std::int64_t a, std::int64_t b)
{
    const std::uint64_t x = magnitude(a), y = magnitude(b);
    const std::uint64_t x0 = x & 0xffffffffu, x1 = x >> 32, y0 = y & 0xffffffffu, y1 = y >> 32;
    const std::uint64_t p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, p11 = x1 * y1;
    const std::uint64_t mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
    Wide w;
    w.lo = (mid << 32) | (p00 & 0xffffffffu);
    w.hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
    w.negative = (a < 0) != (b < 0) && (w.hi != 0 || w.lo != 0);
    return w;
}

// floor(w / d) for d > 0, with the remainder in [0, d); nullopt when the quotient does not fit Int64
std::optional<std::int64_t> floorDivide(const Wide& w, std::int64_t d, std::int64_t* remainder)
{
    const auto divisor = static_cast<std::uint64_t>(d);
    std::uint64_t q = 0, r = 0;
    if (w.hi == 0)
    {
        q = w.lo / divisor;
        r = w.lo % divisor;
    }
    else
    {
        if (w.hi >= divisor)
            return std::nullopt;
        r = w.hi;
        for (int bit = 63; bit >= 0; --bit)
        {
            const bool carry = (r >> 63) != 0;
            r = (r << 1) | ((w.lo >> bit) & 1u);
            q <<= 1;
            if (carry || r >= divisor)
            {
                r -= divisor;
                q |= 1u;
            }
        }
    }
    if (w.negative && r != 0)
    {
        if (q == std::numeric_limits<std::uint64_t>::max())
            return std::nullopt;
        q += 1;
        r = divisor - r;
    }
    constexpr auto max = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (q > max + (w.negative ? 1u : 0u))
        return std::nullopt;
    if (remainder)
        *remainder = static_cast<std::int64_t>(r);
    return w.negative ? static_cast<std::int64_t>(~q + 1) : static_cast<std::int64_t>(q);
}

std::optional<std::int64_t> exactProduct(std::int64_t a, std::int64_t b)
{
    std::int64_t remainder = 0;
    return floorDivide(multiply(a, b), 1, &remainder);
}

bool isIntegral(const BaseObjectPtr& value, std::int64_t* out)
{
    const auto number = value.asPtrOrNull<INumber>();
    if (!number.assigned())
        return false;
    const double f = number.getFloatValue();
    if (f != std::floor(f))
        return false;
    *out = number.getIntValue();
    return true;
}

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

}

std::optional<std::int64_t> exactSum(std::int64_t a, std::int64_t b)
{
    if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) || (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b))
        return std::nullopt;
    return a + b;
}

std::optional<std::int64_t> parseOrigin(const std::string& origin)
{
    bool parsed = false;
    const auto sinceEpoch = reader::parseEpoch(origin, &parsed).time_since_epoch();
    if (!parsed || sinceEpoch % std::chrono::seconds(1) != std::chrono::system_clock::duration::zero())
        return std::nullopt;
    return std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch).count();
}

std::optional<TickDomain> parseDomain(const DataDescriptorPtr& domain, const char** fault)
{
    const auto fail = [fault](const char* cause) -> std::optional<TickDomain>
    {
        if (fault)
            *fault = cause;
        return std::nullopt;
    };
    if (!domain.assigned())
        return fail("no domain descriptor");
    if (!isIntegerType(domain.getSampleType()))
        return fail("ticks not of an integer type");
    const auto dimensions = domain.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() != 0)
        return fail("ticks not scalar");
    const auto unit = domain.getUnit();
    if (!unit.assigned() || unit.getSymbol() != "s")
        return fail("unit not seconds");
    const auto rule = domain.getRule();
    if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
        return fail("rule not linear");
    const auto resolution = domain.getTickResolution();
    if (!resolution.assigned())
        return fail("no tick resolution");
    const auto origin = domain.getOrigin();

    TickDomain d;
    const auto parameters = rule.getParameters();
    if (!parameters.hasKey("delta") || !isIntegral(parameters.get("delta"), &d.delta) || d.delta <= 0)
        return fail("rule delta not an integer above 0");
    if (!parameters.hasKey("start") || !isIntegral(parameters.get("start"), &d.start))
        return fail("rule start not an integer");

    d.relative = !origin.assigned() || origin.getLength() == 0;
    const auto seconds = d.relative ? std::optional<std::int64_t>(0) : parseOrigin(origin.toStdString());
    if (!seconds)
        return fail("origin not an ISO 8601 time on a full second");
    d.originSeconds = *seconds;

    const auto reference = domain.getReferenceDomainInfo();
    if (reference.assigned() && reference.getReferenceDomainOffset().assigned())
        d.referenceOffset = reference.getReferenceDomainOffset();

    d.resNum = resolution.getNumerator();
    d.resDen = resolution.getDenominator();
    if (d.resNum <= 0 || d.resDen <= 0)
        return fail("tick resolution not above 0");
    const std::int64_t g = std::gcd(d.resNum, d.resDen);
    d.resNum /= g;
    d.resDen /= g;

    // An integer rate: resDen is a multiple of resNum * delta
    const auto period = exactProduct(d.resNum, d.delta);
    if (!period || d.resDen % *period != 0)
        return fail("sampling rate not an integer");
    return d;
}

std::int64_t Relation::toMain(std::int64_t tick, std::int64_t* remainder) const
{
    std::int64_t r = 0;
    std::optional<std::int64_t> result;
    bool negative = tick < 0;  // the sign of the step that leaves Int64
    if (const auto shifted = exactSum(tick, shift))
    {
        const Wide product = multiply(*shifted, num);
        negative = product.negative;
        if (const auto quotient = floorDivide(product, den, &r))
        {
            negative = offset < 0;
            result = exactSum(*quotient, offset);
        }
    }
    if (remainder)
        *remainder = r;
    // Out of Int64: such ticks lie centuries away from every origin and never align with the main input
    if (!result)
        return negative ? std::numeric_limits<std::int64_t>::min() / 2 : std::numeric_limits<std::int64_t>::max() / 2;
    return *result;
}

std::optional<Relation> relate(const TickDomain& input, const TickDomain& main, const char** fault)
{
    const auto fail = [fault](const char* cause) -> std::optional<Relation>
    {
        if (fault)
            *fault = cause;
        return std::nullopt;
    };
    // input resolution / main resolution, reduced across before multiplying
    const std::int64_t g1 = std::gcd(input.resNum, main.resNum);
    const std::int64_t g2 = std::gcd(main.resDen, input.resDen);
    const auto num = exactProduct(input.resNum / g1, main.resDen / g2);
    const auto den = exactProduct(input.resDen / g2, main.resNum / g1);
    if (!num || !den)
        return fail("sampling rate differs from the main input's");

    // Equal rates: one input period is one main period, delta * num / den == main.delta
    const auto inputPeriod = exactProduct(input.delta, *num);
    const auto mainPeriod = exactProduct(main.delta, *den);
    if (!inputPeriod || !mainPeriod || *inputPeriod != *mainPeriod)
        return fail("sampling rate differs from the main input's");

    if (input.relative != main.relative)
        return fail(input.relative ? "no origin while the main input has one" : "origin while the main input has none");

    // The origin difference in main ticks: seconds * resDen / resNum, whole and within Int64
    std::int64_t remainder = 0;
    const auto offset = floorDivide(multiply(input.originSeconds - main.originSeconds, main.resDen), main.resNum, &remainder);
    if (!offset || remainder != 0)
        return fail("origin not a whole number of main ticks from the main input's");
    return Relation{*num, *den, *offset - main.referenceOffset, input.referenceOffset};
}

std::int64_t syncLimitTicks(const TickDomain& main)
{
    return std::max<std::int64_t>(1, 2 * main.resDen / main.resNum);  // a resolution coarser than 2 s still allows one tick
}

}

END_NAMESPACE_OPENDAQ
