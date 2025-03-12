#include <opendaq/scaling_factory.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/types_daqbsp_generated_handling.h>
#include <opcuatms/core_types_utils.h>
#include <opcuatms/extension_object.h>
#include <opcuatms/converters/list_conversion_utils.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Definitions

template class StructConverter<IScaling, UA_PostScalingStructure>;
template class StructConverter<IScaling, UA_LinearScalingDescriptionStructure>;
template class VariantConverter<IScaling>;

template <>
ScalingPtr StructConverter<IScaling, UA_LinearScalingDescriptionStructure>::ToDaqObject(
    const UA_LinearScalingDescriptionStructure& tmsStruct, const ContextPtr& /*context*/);
template <>
OpcUaObject<UA_LinearScalingDescriptionStructure> StructConverter<IScaling, UA_LinearScalingDescriptionStructure>::ToTmsType(
    const ScalingPtr& object, const ContextPtr& /*context*/);
template <>
ScalingPtr StructConverter<IScaling, UA_PostScalingStructure>::ToDaqObject(const UA_PostScalingStructure& tmsStruct,
                                                                           const ContextPtr& /*context*/);
template <>
OpcUaObject<UA_PostScalingStructure> StructConverter<IScaling, UA_PostScalingStructure>::ToTmsType(const ScalingPtr& object,
                                                                                                   const ContextPtr& /*context*/);
template <>
ScalingPtr VariantConverter<IScaling>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/);
template <>
OpcUaVariant VariantConverter<IScaling>::ToVariant(const ScalingPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/);

// UA_LinearScalingDescription

template <>
ScalingPtr StructConverter<IScaling, UA_LinearScalingDescriptionStructure>::ToDaqObject(
    const UA_LinearScalingDescriptionStructure& tmsStruct, const ContextPtr& /*context*/)
{
    auto scale = VariantConverter<INumber>::ToDaqObject(tmsStruct.scale);
    auto offset = VariantConverter<INumber>::ToDaqObject(tmsStruct.offset);
    return LinearScaling(scale, offset);
}

template <>
OpcUaObject<UA_LinearScalingDescriptionStructure> StructConverter<IScaling, UA_LinearScalingDescriptionStructure>::ToTmsType(
    const ScalingPtr& object, const ContextPtr& /*context*/)
{
    if (object.getType() != ScalingType::Linear)
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    OpcUaObject<UA_LinearScalingDescriptionStructure> scaling;
    scaling->type = UA_STRING_ALLOC("linear");

    auto scale = object.getParameters().get("scale");
    scaling->scale = VariantConverter<INumber>::ToVariant(scale).getDetachedValue();

    auto offset = object.getParameters().get("offset");
    scaling->offset = VariantConverter<INumber>::ToVariant(offset).getDetachedValue();

    return scaling;
}

// UA_PostScalingStructure

template <>
ScalingPtr StructConverter<IScaling, UA_PostScalingStructure>::ToDaqObject(const UA_PostScalingStructure& tmsStruct,
                                                                           const ContextPtr& /*context*/)
{
    auto scalingObject = ExtensionObject(tmsStruct.scalingDescription);
    const auto scalingVariant = scalingObject.getAsVariant();
    const auto scaling = VariantConverter<IScaling>::ToDaqObject(scalingVariant);

    auto postScaling = ScalingBuilderCopy(scaling)
                           .setInputDataType(SampleTypeFromTmsEnum(tmsStruct.inputSampleType))
                           .setOutputDataType(ScaledSampleTypeFromTmsEnum(tmsStruct.outputSampleType))
                           .build();
    return postScaling;
}

template <>
OpcUaObject<UA_PostScalingStructure> StructConverter<IScaling, UA_PostScalingStructure>::ToTmsType(const ScalingPtr& object,
                                                                                                   const ContextPtr& /*context*/)
{
    OpcUaObject<UA_PostScalingStructure> uaPostScaling;
    uaPostScaling->inputSampleType = SampleTypeToTmsEnum(object.getInputSampleType());
    uaPostScaling->outputSampleType = ScaledSampleTypeToTmsEnum(object.getOutputSampleType());

    OpcUaObject<UA_LinearScalingDescriptionStructure> uaLinearScalingDescription;
    const NumberPtr scale = object.getParameters().get("scale");
    const NumberPtr offset = object.getParameters().get("offset");
    
    uaLinearScalingDescription->type = UA_STRING_ALLOC("linear");
    uaLinearScalingDescription->scale = VariantConverter<INumber>::ToVariant(scale).getDetachedValue();
    uaLinearScalingDescription->offset = VariantConverter<INumber>::ToVariant(offset).getDetachedValue();

    uaPostScaling->scalingDescription.encoding = UA_EXTENSIONOBJECT_DECODED;
    uaPostScaling->scalingDescription.content.decoded.type = &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARSCALINGDESCRIPTIONSTRUCTURE];

    const auto uaLinearScalingDescriptionPtr = UA_LinearScalingDescriptionStructure_new();
    *uaLinearScalingDescriptionPtr = uaLinearScalingDescription.getDetachedValue();
    uaPostScaling->scalingDescription.content.decoded.data = uaLinearScalingDescriptionPtr;

    return uaPostScaling;
}

// Variant converters

template <>
ScalingPtr VariantConverter<IScaling>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_PostScalingStructure>())
    {
        const auto tmsStruct = static_cast<UA_PostScalingStructure*>(decodedVariant->data);
        return StructConverter<IScaling, UA_PostScalingStructure>::ToDaqObject(*tmsStruct);
    }

    if (decodedVariant.isType<UA_LinearScalingDescriptionStructure>())
    {
        const auto tmsStruct = static_cast<UA_LinearScalingDescriptionStructure*>(decodedVariant->data);
        return StructConverter<IScaling, UA_LinearScalingDescriptionStructure>::ToDaqObject(*tmsStruct);
    }
    
    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IScaling>::ToVariant(const ScalingPtr& object, const UA_DataType* targetType, const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType == nullptr || targetType ==&UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_POSTSCALINGSTRUCTURE])
        variant.setScalar(*StructConverter<IScaling, UA_PostScalingStructure>::ToTmsType(object));
    else if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARSCALINGDESCRIPTIONSTRUCTURE])
        variant.setScalar(*StructConverter<IScaling, UA_LinearScalingDescriptionStructure>::ToTmsType(object));
    else
        DAQ_THROW_EXCEPTION(ConversionFailedException);

    return variant;
}

template <>
ListPtr<IScaling> VariantConverter<IScaling>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IScaling>(variant);
    if (variant.isType<UA_PostScalingStructure>())
        return ListConversionUtils::VariantToList<IScaling, UA_PostScalingStructure>(variant);
    if (variant.isType<UA_LinearScalingDescriptionStructure>())
        return ListConversionUtils::VariantToList<IScaling, UA_LinearScalingDescriptionStructure>(variant);

    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

template <>
OpcUaVariant VariantConverter<IScaling>::ToArrayVariant(const ListPtr<IScaling>& list,
                                                        const UA_DataType* targetType,
                                                        const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_POSTSCALINGSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IScaling, UA_PostScalingStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_LINEARSCALINGDESCRIPTIONSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IScaling, UA_LinearScalingDescriptionStructure>(list);
    
    DAQ_THROW_EXCEPTION(ConversionFailedException);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
