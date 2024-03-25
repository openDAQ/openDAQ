#include <opendaq/data_rule_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <coreobjects/unit_factory.h>

#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/sample_type_traits.h>

#include <coretypes/exceptions.h>
#include <coretypes/ratio_factory.h>

#include "streaming_protocol/Types.h"

#include "websocket_streaming/signal_descriptor_converter.h"

#include <memory>
#include <opendaq/tags_factory.h>

#include "streaming_protocol/BaseDomainSignal.hpp"
#include "streaming_protocol/BaseValueSignal.hpp"
#include "streaming_protocol/LinearTimeSignal.hpp"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

/**
 *  @todo Only scalar values are supported for now. No structs. No dimensions.
 */
SubscribedSignalInfo SignalDescriptorConverter::ToDataDescriptor(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    SubscribedSignalInfo sInfo;

    // *** meta "definition" start ***
    // get metainfo received via signal "definition" object and stored in SubscribedSignal struct
    auto dataDescriptor = DataDescriptorBuilder().setRule(GetRule(subscribedSignal));

    if (subscribedSignal.isTimeSignal())
    {
        uint64_t numerator = 1;
        uint64_t denominator = subscribedSignal.timeBaseFrequency();
        auto resolution = Ratio(static_cast<daq::Int>(numerator), static_cast<daq::Int>(denominator));
        dataDescriptor.setTickResolution(resolution);
    }

    daq::streaming_protocol::SampleType streamingSampleType = subscribedSignal.dataValueType();
    daq::SampleType daqSampleType = Convert(streamingSampleType);
    dataDescriptor.setSampleType(daqSampleType);

    sInfo.signalName = subscribedSignal.memberName();

    if (subscribedSignal.unitId() != daq::streaming_protocol::Unit::UNIT_ID_NONE)
    {
        auto unit = Unit(subscribedSignal.unitDisplayName(),
                         subscribedSignal.unitId(),
                         "",
                         subscribedSignal.unitQuantity());

        dataDescriptor.setUnit(unit);
    }

    dataDescriptor.setOrigin(subscribedSignal.timeBaseEpochAsString());
    // *** meta "definition" end ***

    if (!subscribedSignal.isTimeSignal())
    {
        // "definition" object does not contain signal value range used to configure renderer,
        // we need to set the some default value for the fusion (non-openDAQ) device signals
        // if the signal is owned by the openDAQ-enabled server
        // than the value range from "interpretation" will overwrite the default value
        dataDescriptor.setValueRange(daq::Range(-15.0, 15.0));
    }

    // --- meta "interpretation" start ---
    // overwrite/add descriptor fields with ones from optional "interpretation" object
    auto extra = subscribedSignal.interpretationObject();
    DecodeInterpretationObject(extra, dataDescriptor);

    sInfo.dataDescriptor = dataDescriptor.build();
    if (extra.count("sig_name") > 0)
        sInfo.signalProps.name = extra["sig_name"];
    if (extra.count("sig_desc") > 0)
        sInfo.signalProps.description = extra["sig_desc"];
    // --- meta "interpretation" end ---

    return sInfo;
}

void SignalDescriptorConverter::ToStreamedValueSignal(const daq::SignalPtr& valueSignal,
                                                      daq::streaming_protocol::BaseValueSignalPtr valueStream,
                                                      const SignalProps& sigProps)
{
    auto dataDescriptor = valueSignal.getDescriptor();
    if (!dataDescriptor.assigned())
        return;

    // *** meta "definition" start ***
    // set/verify fields which will be lately encoded into signal "definition" object
    valueStream->setMemberName(valueSignal.getName());

    // Data type of stream can not be changed. Complain upon change!
    daq::SampleType daqSampleType = dataDescriptor.getSampleType();
    if (dataDescriptor.getPostScaling().assigned())
        daqSampleType = dataDescriptor.getPostScaling().getInputSampleType();

    daq::streaming_protocol::SampleType requestedSampleType = Convert(daqSampleType);
    if (requestedSampleType != valueStream->getSampleType())
        throw ConversionFailedException();

    UnitPtr unit = dataDescriptor.getUnit();
    if (unit.assigned())
        valueStream->setUnit(unit.getId(), unit.getSymbol());
    // *** meta "definition" end ***

    // --- meta "interpretation" start ---
    nlohmann::json extra;
    EncodeInterpretationObject(dataDescriptor, extra);

    if (sigProps.name.has_value())
        extra["sig_name"] = sigProps.name.value();
    if (sigProps.description.has_value())
        extra["sig_desc"] = sigProps.description.value();

    valueStream->setInterpretationObject(extra);
    // --- meta "interpretation" end ---
}

void SignalDescriptorConverter::ToStreamedLinearSignal(const daq::SignalPtr& domainSignal,
                                                       streaming_protocol::LinearTimeSignalPtr linearStream,
                                                       const SignalProps& sigProps)
{
    auto domainDescriptor = domainSignal.getDescriptor();
    if (!domainDescriptor.assigned())
        return;

    // *** meta "definition" start ***

    // streaming-lt supports only 64bit domain values
    daq::SampleType daqSampleType = domainDescriptor.getSampleType();
    daq::streaming_protocol::SampleType requestedSampleType = Convert(daqSampleType);
    if (requestedSampleType != daq::streaming_protocol::SampleType::SAMPLETYPE_S64 &&
        requestedSampleType != daq::streaming_protocol::SampleType::SAMPLETYPE_U64)
        throw ConversionFailedException();

    DataRulePtr rule = domainDescriptor.getRule();
    SetLinearTimeRule(rule, linearStream);

    auto resolution = domainDescriptor.getTickResolution();
    linearStream->setTimeTicksPerSecond(resolution.getDenominator() / resolution.getNumerator());
    // *** meta "definition" end ***

    // --- meta "interpretation" start ---

    // openDAQ does not encode meta "definition" object directly for domain signal
    // domain signal "definition" is hardcoded on library level
    // so openDAQ uses "interpretation" object to transmit metadata which describes domain signal
    nlohmann::json extra;
    if (domainDescriptor.assigned())
        EncodeInterpretationObject(domainDescriptor, extra);

    if (sigProps.name.has_value())
        extra["sig_name"] = sigProps.name.value();
    if (sigProps.description.has_value())
        extra["sig_desc"] = sigProps.description.value();

    linearStream->setInterpretationObject(extra);
    // --- meta "interpretation" end ---
}

/**
 *  @throws ConversionFailedException
 */
daq::DataRulePtr SignalDescriptorConverter::GetRule(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    switch (subscribedSignal.ruleType())
    {
        case daq::streaming_protocol::RULETYPE_CONSTANT:
        {
            return ConstantDataRule();
        }
        break;
        case daq::streaming_protocol::RULETYPE_EXPLICIT:
        {
            return ExplicitDataRule();
        }
        break;
        case daq::streaming_protocol::RULETYPE_LINEAR:
        {
            uint64_t start = subscribedSignal.time();
            return LinearDataRule(subscribedSignal.linearDelta(), start);
        }
        break;
        default:
            throw ConversionFailedException();
    }
}

/**
 *  @throws ConversionFailedException
 */
void SignalDescriptorConverter::SetLinearTimeRule(const daq::DataRulePtr& rule, daq::streaming_protocol::LinearTimeSignalPtr linearStream)
{
    if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
    {
        throw ConversionFailedException();
    }
    uint64_t delta = rule.getParameters().get("delta");
    linearStream->setOutputRate(delta);
}

/**
 *  @throws ConversionFailedException
 */
daq::SampleType SignalDescriptorConverter::Convert(daq::streaming_protocol::SampleType dataType)
{
    switch (dataType)
    {
        case daq::streaming_protocol::SampleType::SAMPLETYPE_S8:
            return daq::SampleType::Int8;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_S16:
            return daq::SampleType::Int16;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_S32:
            return daq::SampleType::Int32;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_S64:
            return daq::SampleType::Int64;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_U8:
            return daq::SampleType::UInt8;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_U16:
            return daq::SampleType::UInt16;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_U32:
            return daq::SampleType::UInt32;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_U64:
            return daq::SampleType::UInt64;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_COMPLEX32:
            return daq::SampleType::ComplexFloat32;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_COMPLEX64:
            return daq::SampleType::ComplexFloat64;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_REAL32:
            return daq::SampleType::Float32;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_REAL64:
            return daq::SampleType::Float64;
        case daq::streaming_protocol::SampleType::SAMPLETYPE_BITFIELD32:
        case daq::streaming_protocol::SampleType::SAMPLETYPE_BITFIELD64:
        default:
            throw ConversionFailedException();
    }
}

/**
 *  @throws ConversionFailedException
 */
daq::streaming_protocol::SampleType SignalDescriptorConverter::Convert(daq::SampleType sampleType)
{
    switch (sampleType)
    {
        case daq::SampleType::Int8:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_S8;
            break;
        case daq::SampleType::Int16:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_S16;
            break;
        case daq::SampleType::Int32:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_S32;
            break;
        case daq::SampleType::Int64:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_S64;
            break;
        case daq::SampleType::ComplexFloat32:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_COMPLEX32;
            break;
        case daq::SampleType::ComplexFloat64:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_COMPLEX64;
            break;
        case daq::SampleType::Float32:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_REAL32;
            break;
        case daq::SampleType::Float64:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_REAL64;
            break;
        case daq::SampleType::UInt8:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_U8;
            break;
        case daq::SampleType::UInt16:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_U16;
            break;
        case daq::SampleType::UInt32:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_U32;
            break;
        case daq::SampleType::UInt64:
            return daq::streaming_protocol::SampleType::SAMPLETYPE_U64;
            break;
        case daq::SampleType::Binary:
        case daq::SampleType::Struct:
        case daq::SampleType::Invalid:
        case daq::SampleType::String:
        case daq::SampleType::RangeInt64:
        default:
            throw ConversionFailedException();
    }
}

void SignalDescriptorConverter::EncodeInterpretationObject(const DataDescriptorPtr& dataDescriptor, nlohmann::json& extra)
{
    // put descriptor name into interpretation object
    if (dataDescriptor.getName().assigned())
        extra["desc_name"] = dataDescriptor.getName();

    if (dataDescriptor.getMetadata().assigned())
    {
        auto meta = extra["metadata"];
        for (const auto& [key, value] : dataDescriptor.getMetadata())
            meta[key.getCharPtr()] = value;
        extra["metadata"] = meta;
    }

    if (dataDescriptor.getUnit().assigned())
    {
        auto unit1 = dataDescriptor.getUnit();
        extra["unit"]["id"] = unit1.getId();
        extra["unit"]["name"] = unit1.getName();
        extra["unit"]["symbol"] = unit1.getSymbol();
        extra["unit"]["quantity"] = unit1.getQuantity();
    }

    if (dataDescriptor.getValueRange().assigned())
    {
        auto range = dataDescriptor.getValueRange();
        extra["range"]["low"] = range.getLowValue();
        extra["range"]["high"] = range.getHighValue();
    }

    if (dataDescriptor.getOrigin().assigned())
        extra["origin"] = dataDescriptor.getOrigin();

    if (dataDescriptor.getRule().assigned())
    {
        auto rule = dataDescriptor.getRule();
        extra["rule"]["type"] = rule.getType();
        extra["rule"]["parameters"] = DictToJson(rule.getParameters());
    }

    if (dataDescriptor.getPostScaling().assigned())
    {
        auto scaling = dataDescriptor.getPostScaling();
        extra["scaling"]["inputType"] = scaling.getInputSampleType();
        extra["scaling"]["outputType"] = scaling.getOutputSampleType();
        extra["scaling"]["scalingType"] = scaling.getType();
        extra["scaling"]["parameters"] = DictToJson(scaling.getParameters());
    }
}

void SignalDescriptorConverter::DecodeInterpretationObject(const nlohmann::json& extra, DataDescriptorBuilderPtr& dataDescriptor)
{
    // sets descriptor name when corresponding field is present in interpretation object
    if (extra.count("desc_name") > 0)
        dataDescriptor.setName(extra["desc_name"]);

    if (extra.count("metadata") > 0)
    {
        auto meta = JsonToDict(extra["metadata"]);
        dataDescriptor.setMetadata(meta);
    }

    if (extra.count("unit") > 0)
    {
        auto unitObj = extra["unit"];
        auto unit = Unit(unitObj["symbol"], unitObj["id"], unitObj["name"], unitObj["quantity"]);
        dataDescriptor.setUnit(unit);
    }

    if (extra.count("range") > 0)
    {
        auto rangeObj = extra["range"];
        auto low = std::stoi(std::string(rangeObj["low"]));
        auto high = std::stoi(std::string(rangeObj["high"]));
        auto range = Range(low, high);
        dataDescriptor.setValueRange(range);
    }

    if (extra.count("origin") > 0)
        dataDescriptor.setOrigin(extra["origin"]);

    if (extra.count("rule") > 0)
    {
        auto params = JsonToDict(extra["rule"]["parameters"]);
        params.freeze();

        auto rule = DataRuleBuilder().setType(extra["rule"]["type"]).setParameters(params).build();
        dataDescriptor.setRule(rule);
    }

    if (extra.count("scaling") > 0)
    {
        auto params = JsonToDict(extra["scaling"]["parameters"]);
        params.freeze();

        auto scaling = ScalingBuilder()
                           .setInputDataType(extra["scaling"]["inputType"])
                           .setOutputDataType(extra["scaling"]["outputType"])
                           .setScalingType(extra["scaling"]["scalingType"])
                           .setParameters(params)
                           .build();
        dataDescriptor.setPostScaling(scaling);

        // overwrite sample type when scaling is present
        dataDescriptor.setSampleType(convertScaledToSampleType(scaling.getOutputSampleType()));
    }
}

nlohmann::json SignalDescriptorConverter::DictToJson(const DictPtr<IString, IBaseObject>& dict)
{
    nlohmann::json json;

    for (const auto& [key, value] : dict)
    {
        if (value.asPtrOrNull<IList>().assigned())
            json[key.getCharPtr()] = value.asPtr<IList>().toVector();
        else if (value.asPtrOrNull<IDict>().assigned())
            json[key.getCharPtr()] = DictToJson(value);
        else if (value.asPtrOrNull<IFloat>().assigned())
            json[key.getCharPtr()] = (Float) value;
        else if (value.asPtrOrNull<IInteger>().assigned())
            json[key.getCharPtr()] = (Int) value;
        else
            json[key.getCharPtr()] = value;
    }

    return json;
}

DictPtr<IString, IBaseObject> SignalDescriptorConverter::JsonToDict(const nlohmann::json& json)
{
    auto dict = Dict<IString, IBaseObject>();
    auto items = json.items();

    for (const auto& entry : items)
    {
        if (entry.value().is_array())
        {
            auto vect = entry.value().get<std::vector<BaseObjectPtr>>();
            dict[entry.key()] = ListPtr<IBaseObject>::FromVector(vect);
        }
        else if (entry.value().is_object())
            dict[entry.key()] = JsonToDict(entry.value());
        else if (entry.value().is_number_float())
            dict[entry.key()] = entry.value().get<Float>();
        else if (entry.value().is_number_integer())
            dict[entry.key()] = entry.value().get<Int>();
        else
            dict[entry.key()] = entry.value();
    }

    return dict;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
