#include <cstdint>
#include <string>

#include <opendaq/opendaq.h>

#include <nlohmann/json.hpp>

#include "metadata.hpp"

using namespace daq;

static nlohmann::json define(const RangePtr& range)
{
    return {
        { "high", range.getHighValue().getFloatValue() },
        { "low", range.getLowValue().getFloatValue() },
    };
}

static nlohmann::json interpret(const RangePtr& range)
{
    return {
        { "high", std::to_string(range.getHighValue().getFloatValue()) },
        { "low", std::to_string(range.getLowValue().getFloatValue()) },
    };
}

static const char *to_string(DataRuleType type)
{
    switch (type)
    {
        case DataRuleType::Constant:
            return "constant";

        case DataRuleType::Explicit:
            return "explicit";

        case DataRuleType::Linear:
            return "linear";

        default:
            throw NotSupportedException("unsupported data rule " + std::to_string(static_cast<unsigned>(type)));
    }
}

static nlohmann::json to_string_and_params(const DataRulePtr& rule)
{
    nlohmann::json result = { { "rule", to_string(rule.getType()) } };

    if (rule.getType() == DataRuleType::Linear)
        result["linear"] = { { "delta", static_cast<std::uint64_t>(rule.getParameters()["delta"]) } };

    return result;
}

static const char *to_string(DimensionRuleType type)
{
    switch (type)
    {
        case DimensionRuleType::Linear:
            return "linear";

        case DimensionRuleType::List:
            return "list";

        case DimensionRuleType::Logarithmic:
            return "logarithmic";

        default:
            throw NotSupportedException("unsupported dimension rule " + std::to_string(static_cast<unsigned>(type)));
    }
}

static nlohmann::json to_string_and_params(const DimensionRulePtr& rule)
{
    nlohmann::json result = { { "rule", to_string(rule.getType()) } };

    if (rule.getType() == DimensionRuleType::Linear)
        result["linear"] = {
            { "start", static_cast<std::uint64_t>(rule.getParameters()["start"]) },
            { "delta", static_cast<std::uint64_t>(rule.getParameters()["delta"]) },
            { "size", static_cast<std::uint64_t>(rule.getParameters()["size"]) },
        };

    return result;
}

static nlohmann::json describe(SampleType type)
{
    switch (type)
    {
        case SampleType::Int8: return "int8";
        case SampleType::Int16: return "int16";
        case SampleType::Int32: return "int32";
        case SampleType::Int64: return "int64";
        case SampleType::UInt8: return "uint8";
        case SampleType::UInt16: return "uint16";
        case SampleType::UInt32: return "uint32";
        case SampleType::UInt64: return "uint64";
        case SampleType::Float32: return "real32";
        case SampleType::Float64: return "real64";
        case SampleType::Struct: return "struct";

        default:
            throw NotSupportedException("unsupported sample type " + std::to_string(static_cast<unsigned>(type)));
    }
}

static nlohmann::json describe(const BaseObjectPtr& obj)
{
    if (!obj.assigned())
        return nullptr;

    switch (obj.getCoreType())
    {
        case CoreType::ctInt:
        case CoreType::ctFloat:
            return static_cast<double>(obj);

        case CoreType::ctString:
            return static_cast<std::string>(obj);

        default:
            break;
    }

    throw NotSupportedException("unsupported raw BaseObject type " + std::to_string(static_cast<unsigned>(obj.getCoreType())));
}

static nlohmann::json describe(const DataRulePtr& rule)
{
    nlohmann::json parameters = nullptr;

    for (const auto& entry : rule.getParameters())
    {
        if (parameters.is_null())
            parameters = nlohmann::json::object();
        parameters[static_cast<std::string>(entry.first)] = describe(entry.second);
    }

    return {
        { "type", rule.getType() },
        { "parameters", parameters },
    };
}

static nlohmann::json define(const UnitPtr& unit)
{
    return {
        { "displayName", unit.getSymbol() },
        { "quantity", unit.getQuantity() },
        { "unitId", unit.getId() },
    };
}

static nlohmann::json interpret(const UnitPtr& unit)
{
    return {
        { "id", unit.getId() },
        { "name", unit.getName() },
        { "quantity", unit.getQuantity() },
        { "symbol", unit.getSymbol() },
    };
}

static nlohmann::json describe(const RatioPtr& ratio)
{
    return {
        { "num", ratio.getNumerator() },
        { "denom", ratio.getDenominator() },
    };
}

static nlohmann::json to_dimension(const DimensionPtr& dimension)
{
    nlohmann::json result = { { "name", dimension.getName() } };
    result.update(to_string_and_params(dimension.getRule()));
    return result;
}

static nlohmann::json to_definition(const DataDescriptorPtr& descriptor)
{
    nlohmann::json definition = {
        { "dataType", describe(descriptor.getSampleType()) },
        { "name", descriptor.getName() },
    };

    definition.update(to_string_and_params(descriptor.getRule()));

    if (descriptor.getSampleType() == SampleType::Struct)
    {
        nlohmann::json fields;

        if (descriptor.getStructFields().assigned())
            for (const auto& field : descriptor.getStructFields())
                fields.push_back(to_definition(field));

        definition["struct"] = fields;
    }

    if (descriptor.getDimensions().assigned())
    {
        nlohmann::json dimensions;

        for (const auto& dimension : descriptor.getDimensions())
            dimensions.push_back(to_dimension(dimension));

        if (!dimensions.empty())
            definition["dimensions"] = dimensions;
    }

    return definition;
}

nlohmann::json daq::ws_streaming::to_metadata(
    std::string id,
    const DataDescriptorPtr& descriptor,
    std::string description,
    std::string domain_signal_id)
{
    nlohmann::json result =
    {
        { "definition", to_definition(descriptor) },
        { "interpretation", {
            { "desc_name", descriptor.getName() },
            { "metadata", nullptr },
            { "origin", descriptor.getOrigin() },
            { "rule", describe(descriptor.getRule()) },
            { "sig_desc", description },
            { "sig_name", descriptor.getName() },
        } },
        { "tableId", domain_signal_id.empty() ? id : domain_signal_id }
    };

    if (descriptor.getUnit().assigned())
    {
        result["definition"]["unit"] = define(descriptor.getUnit());
        result["interpretation"]["unit"] = interpret(descriptor.getUnit());
    }

    if (descriptor.getValueRange().assigned())
    {
        result["definition"]["range"] = define(descriptor.getValueRange());
        result["interpretation"]["range"] = interpret(descriptor.getValueRange());
    }

    if (descriptor.getRule().getType() != DataRuleType::Constant)
        result["valueIndex"] = 0;

    if (descriptor.getTickResolution().assigned())
        result["definition"]["resolution"] = describe(descriptor.getTickResolution());

    if (domain_signal_id.empty())
        result["definition"]["absoluteReference"] = "1970-01-01";

    return result;
}
