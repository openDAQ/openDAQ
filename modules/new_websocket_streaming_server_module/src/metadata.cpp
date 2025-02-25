#include <cstdint>
#include <string>

#include <opendaq/opendaq.h>

#include <nlohmann/json.hpp>

#include "metadata.hpp"

static nlohmann::json define(daq::RangePtr range)
{
    return {
        { "high", range.getHighValue().getFloatValue() },
        { "low", range.getLowValue().getFloatValue() },
    };
}

static nlohmann::json interpret(daq::RangePtr range)
{
    return {
        { "high", std::to_string(range.getHighValue().getFloatValue()) },
        { "low", std::to_string(range.getLowValue().getFloatValue()) },
    };
}

static nlohmann::json describe(daq::DataRuleType type)
{
    switch (type)
    {
        case daq::DataRuleType::Constant:
            return "constant";

        case daq::DataRuleType::Explicit:
            return "explicit";

        case daq::DataRuleType::Linear:
            return "linear";

        default:
            throw new daq::NotSupportedException("unsupported data rule");
    }
}

static nlohmann::json describe(daq::SampleType type)
{
    switch (type)
    {
        case daq::SampleType::Int8: return "int8";
        case daq::SampleType::Int16: return "int16";
        case daq::SampleType::Int32: return "int32";
        case daq::SampleType::Int64: return "int64";
        case daq::SampleType::UInt8: return "uint8";
        case daq::SampleType::UInt16: return "uint16";
        case daq::SampleType::UInt32: return "uint32";
        case daq::SampleType::UInt64: return "uint64";
        case daq::SampleType::Float32: return "real32";
        case daq::SampleType::Float64: return "real64";

        default:
            throw new daq::NotSupportedException("unsupported sample type");
    }
}

static nlohmann::json describe(daq::BaseObjectPtr obj)
{
    if (!obj.assigned())
        return nullptr;

    switch (obj.getCoreType())
    {
        case daq::CoreType::ctInt:
        case daq::CoreType::ctFloat:
            return static_cast<double>(obj);

        case daq::CoreType::ctString:
            return static_cast<std::string>(obj);

        default:
            break;
    }

    throw new daq::NotSupportedException("unsupported raw BaseObject type");
}

static nlohmann::json describe(daq::DataRulePtr rule)
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

static nlohmann::json define(daq::UnitPtr unit)
{
    return {
        { "displayName", unit.getSymbol() },
        { "quantity", unit.getQuantity() },
        { "unitId", unit.getId() },
    };
}

static nlohmann::json interpret(daq::UnitPtr unit)
{
    return {
        { "id", unit.getId() },
        { "name", unit.getName() },
        { "quantity", unit.getQuantity() },
        { "symbol", unit.getSymbol() },
    };
}

static nlohmann::json describe(daq::RatioPtr ratio)
{
    return {
        { "num", ratio.getNumerator() },
        { "denom", ratio.getDenominator() },
    };
}

nlohmann::json daq::ws_streaming::to_metadata(
    std::string id,
    daq::DataDescriptorPtr descriptor,
    std::string description,
    std::string domain_signal_id)
{
    nlohmann::json result =
    {
        { "definition", {
            { "dataType", describe(descriptor.getSampleType()) },
            { "name", descriptor.getName() },
            { "rule", describe(descriptor.getRule().getType()) },
        } },
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

    if (descriptor.getRule().getType() == daq::DataRuleType::Linear)
        result["definition"]["linear"] = { { "delta", static_cast<std::uint64_t>(descriptor.getRule().getParameters()["delta"]) } };

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

    if (descriptor.getRule().getType() != daq::DataRuleType::Constant)
        result["valueIndex"] = 0;

    if (descriptor.getTickResolution().assigned())
        result["definition"]["resolution"] = describe(descriptor.getTickResolution());

    if (domain_signal_id.empty())
        result["definition"]["absoluteReference"] = "1970-01-01";

    return result;
}
