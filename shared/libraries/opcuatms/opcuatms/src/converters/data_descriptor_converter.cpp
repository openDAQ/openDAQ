#include <open62541/types_daqbsp_generated_handling.h>
#include <opendaq/data_descriptor_factory.h>
#include "opcuatms/converters/struct_converter.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/core_types_utils.h"
#include "opcuatms/extension_object.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Definitions

template class StructConverter<IDataDescriptor, UA_StructDescriptorStructure>;
template class StructConverter<IDataDescriptor, UA_DataDescriptorStructure>;
template class VariantConverter<IDataDescriptor>;

template <>
DataDescriptorPtr StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToDaqObject(const UA_StructDescriptorStructure& tmsStruct,
                                                                                              const ContextPtr& /*context*/);

template <>
OpcUaObject<UA_StructDescriptorStructure> StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToTmsType(
    const DataDescriptorPtr& object, const ContextPtr& /*context*/);

template <>
DataDescriptorPtr StructConverter<IDataDescriptor, UA_DataDescriptorStructure>::ToDaqObject(const UA_DataDescriptorStructure& tmsStruct,
                                                                                            const ContextPtr& /*context*/);

template <>
DataDescriptorPtr VariantConverter<IDataDescriptor>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/);

template <>
OpcUaVariant VariantConverter<IDataDescriptor>::ToVariant(const DataDescriptorPtr& object,
                                                          const UA_DataType* targetType,
                                                          const ContextPtr& /*context*/);

template <>
ListPtr<IDataDescriptor> VariantConverter<IDataDescriptor>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/);

template <>
OpcUaVariant VariantConverter<IDataDescriptor>::ToArrayVariant(const ListPtr<IDataDescriptor>& list,
                                                               const UA_DataType* targetType,
                                                               const ContextPtr& /*context*/);

template <>
DataDescriptorPtr VariantConverter<IDataDescriptor>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/);

template <>
OpcUaVariant VariantConverter<IDataDescriptor>::ToVariant(const DataDescriptorPtr& object,
                                                          const UA_DataType* targetType,
                                                          const ContextPtr& /*context*/);

template <>
DataDescriptorPtr VariantConverter<IDataDescriptor>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/);

template <>
OpcUaVariant VariantConverter<IDataDescriptor>::ToVariant(const DataDescriptorPtr& object,
                                                          const UA_DataType* targetType,
                                                          const ContextPtr& /*context*/);

// Helper methods

static void WriteDimensions(const ListPtr<IDimension>& dimensions,
                            UA_DimensionDescriptorStructure*& dimensionsOut,
                            size_t& dimensionsSizeOut)
{
    if (!dimensions.assigned() || dimensions.getCount() == 0)
        return;

    dimensionsSizeOut = dimensions.getCount();
    dimensionsOut = static_cast<UA_DimensionDescriptorStructure*>(UA_Array_new(dimensions.getCount(), GetUaDataType<UA_DimensionDescriptorStructure>()));

    for (SizeT i = 0; i < dimensions.getCount(); i++)
    {
        auto tmsDimension = StructConverter<IDimension, UA_DimensionDescriptorStructure>::ToTmsType(dimensions[i]);
        dimensionsOut[i] = tmsDimension.getDetachedValue();
    }
}

static ListPtr<IDimension> ReadDimensions(const UA_DimensionDescriptorStructure* dimensions, size_t dimensionsSize)
{
    auto list = List<IDimension>();
    for (size_t i = 0; i < dimensionsSize; i++)
    {
        auto dimension = StructConverter<IDimension, UA_DimensionDescriptorStructure>::ToDaqObject(dimensions[i]);
        list.pushBack(dimension);
    }
    return list;
}

static void WriteMetadata(const DictPtr<IString, IString>& metadata,
                          UA_KeyValuePair*& metadataOut,
                          size_t& dimensionsSizeOut)
{
    metadataOut = static_cast<UA_KeyValuePair*>(UA_Array_new(metadata.getCount(), &UA_TYPES[UA_TYPES_KEYVALUEPAIR]));
    dimensionsSizeOut = metadata.getCount();
    size_t index = 0;

    for (const auto& [name, value] : metadata)
    {
        OpcUaObject<UA_KeyValuePair> pair;
        pair->key = UA_QUALIFIEDNAME_ALLOC(1, name.getCharPtr());
        pair->value = VariantConverter<IString>::ToVariant(value).getDetachedValue();
        metadataOut[index] = pair.getDetachedValue();

        index++;
    }
}

static DictPtr<IString, IString> ReadMetadata(const UA_KeyValuePair* metadata, size_t metadataSize)
{
    auto dict = Dict<IString, IString>();
    if (!metadata)
        return dict;
    
    for (size_t i = 0; i < metadataSize; i++)
    {
        const auto pair = &metadata[i];
        if (const auto variant = OpcUaVariant(pair->value); variant.isString())
            dict.set(ConvertToDaqCoreString(pair->key.name), variant.toString());
    }
    return dict;
}

// UA_StructDescriptorStructure

template <>
DataDescriptorPtr StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToDaqObject(const UA_StructDescriptorStructure& tmsStruct,
                                                                                              const ContextPtr& /*context*/)
{
    auto members = List<IDataDescriptor>();
    for (size_t i = 0; i < tmsStruct.fieldsSize; i++)
    {
        auto memberObject = ExtensionObject(tmsStruct.fields[i]);
        auto memberVariant = memberObject.getAsVariant();
        auto member = VariantConverter<IDataDescriptor>::ToDaqObject(memberVariant);
        members.pushBack(member);
    }

    const auto descriptor = DataDescriptorBuilder()
                            .setSampleType(SampleType::Undefined)
                            .setName(ConvertToDaqCoreString(tmsStruct.name))
                            .setDimensions(ReadDimensions(tmsStruct.dimensions, tmsStruct.dimensionsSize))
                            .setMetadata(ReadMetadata(tmsStruct.metadata, tmsStruct.metadataSize))
                            .setStructFields(members);

    return descriptor.build();
}

template <>
OpcUaObject<UA_StructDescriptorStructure> StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToTmsType(
    const DataDescriptorPtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_StructDescriptorStructure> tmsStruct;
    tmsStruct->name = ConvertToOpcUaString(object.getName()).getDetachedValue();

    WriteDimensions(object.getDimensions(), tmsStruct->dimensions, tmsStruct->dimensionsSize);
    WriteMetadata(object.getMetadata(), tmsStruct->metadata, tmsStruct->metadataSize);

    auto members = object.getStructFields();
    tmsStruct->fieldsSize = members.getCount();
    tmsStruct->fields = static_cast<UA_ExtensionObject*>(UA_Array_new(members.getCount(), GetUaDataType<UA_ExtensionObject>()));

    for (SizeT i = 0; i < members.getCount(); i++)
    {
        auto member = members[i];
        auto variant = VariantConverter<IDataDescriptor>::ToVariant(member);
        auto memberObject = ExtensionObject(variant);
        tmsStruct->fields[i] = memberObject.getDetachedValue();
    }

    return tmsStruct;
}

// UA_DataDescriptorStructure

template <>
DataDescriptorPtr StructConverter<IDataDescriptor, UA_DataDescriptorStructure>::ToDaqObject(const UA_DataDescriptorStructure& tmsStruct,
                                                                                            const ContextPtr& /*context*/)
{
    const auto descriptor = DataDescriptorBuilder()
                            .setSampleType(SampleTypeFromTmsEnum(tmsStruct.sampleType))
                            .setName(ConvertToDaqCoreString(tmsStruct.name))
                            .setDimensions(ReadDimensions(tmsStruct.dimensions, tmsStruct.dimensionsSize))
                            .setMetadata(ReadMetadata(tmsStruct.metadata, tmsStruct.metadataSize));

    if (tmsStruct.unit)
        descriptor.setUnit(StructConverter<IUnit, UA_EUInformationWithQuantity>::ToDaqObject(*tmsStruct.unit));

    if (tmsStruct.valueRange)
        descriptor.setValueRange(StructConverter<IRange, UA_Range>::ToDaqObject(*tmsStruct.valueRange));

    auto ruleObject = ExtensionObject(tmsStruct.rule);
    if (ruleObject.isDecoded())
        descriptor.setRule(VariantConverter<IDataRule>::ToDaqObject(ruleObject.getAsVariant()));

    if (tmsStruct.origin)
        descriptor.setOrigin(ConvertToDaqCoreString(*tmsStruct.origin));

    if (tmsStruct.tickResolution)
        descriptor.setTickResolution(StructConverter<IRatio, UA_RationalNumber64>::ToDaqObject(*tmsStruct.tickResolution));

    if (tmsStruct.postScaling)
        descriptor.setPostScaling(StructConverter<IScaling, UA_PostScalingStructure>::ToDaqObject(*tmsStruct.postScaling));

    return descriptor.build();
}

template <>
OpcUaObject<UA_DataDescriptorStructure> StructConverter<IDataDescriptor, UA_DataDescriptorStructure>::ToTmsType(
    const DataDescriptorPtr& object, const ContextPtr& /*context*/)
{
    OpcUaObject<UA_DataDescriptorStructure> tmsStruct;
    tmsStruct->sampleType = SampleTypeToTmsEnum(object.getSampleType());

    WriteDimensions(object.getDimensions(), tmsStruct->dimensions, tmsStruct->dimensionsSize);
    WriteMetadata(object.getMetadata(), tmsStruct->metadata, tmsStruct->metadataSize);

    tmsStruct->name = ConvertToOpcUaString(object.getName()).getDetachedValue();

    if (object.getUnit().assigned())
        tmsStruct->unit = StructConverter<IUnit, UA_EUInformationWithQuantity>::ToTmsType(object.getUnit()).newDetachedPointer();

    if (object.getValueRange().assigned())
        tmsStruct->valueRange = StructConverter<IRange, UA_Range>::ToTmsType(object.getValueRange()).newDetachedPointer();

    if (object.getRule().assigned())
    {
        auto ruleObject = ExtensionObject(VariantConverter<IDataRule>::ToVariant(object.getRule()));
        tmsStruct->rule = ruleObject.getDetachedValue();
    }

    if (object.getOrigin().assigned())
        tmsStruct->origin = ConvertToOpcUaString(object.getOrigin()).newDetachedPointer();

    if (object.getTickResolution().assigned())
        tmsStruct->tickResolution = StructConverter<IRatio, UA_RationalNumber64>::ToTmsType(object.getTickResolution()).newDetachedPointer();

    if (object.getPostScaling().assigned())
        tmsStruct->postScaling = StructConverter<IScaling, UA_PostScalingStructure>::ToTmsType(object.getPostScaling()).newDetachedPointer();

    return tmsStruct;
}

// Variant DataDescriptorPtr

template <>
DataDescriptorPtr VariantConverter<IDataDescriptor>::ToDaqObject(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    const auto decodedVariant = DecodeIfExtensionObject(variant);

    if (decodedVariant.isType<UA_StructDescriptorStructure>())
    {
        const auto tmsStruct = static_cast<UA_StructDescriptorStructure*>(variant->data);
        return StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToDaqObject(*tmsStruct);
    }

    if (decodedVariant.isType<UA_DataDescriptorStructure>())
    {
        const auto tmsStruct = static_cast<UA_DataDescriptorStructure*>(variant->data);
        return StructConverter<IDataDescriptor, UA_DataDescriptorStructure>::ToDaqObject(*tmsStruct);
    }

    throw ConversionFailedException();
}

template <>
OpcUaVariant VariantConverter<IDataDescriptor>::ToVariant(const DataDescriptorPtr& object,
                                                          const UA_DataType* targetType,
                                                          const ContextPtr& /*context*/)
{
    auto variant = OpcUaVariant();

    if (targetType ==nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_BASEDATADESCRIPTORSTRUCTURE])
    {
        if (object.isStructDescriptor())
            variant.setScalar(*StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToTmsType(object));
        else
            variant.setScalar(*StructConverter<IDataDescriptor, UA_DataDescriptorStructure>::ToTmsType(object));
    }
    else if (targetType ==&UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_DATADESCRIPTORSTRUCTURE])
        variant.setScalar(*StructConverter<IDataDescriptor, UA_DataDescriptorStructure>::ToTmsType(object));
    else if (targetType ==&UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_STRUCTDESCRIPTORSTRUCTURE])
        variant.setScalar(*StructConverter<IDataDescriptor, UA_StructDescriptorStructure>::ToTmsType(object));
    else
        throw ConversionFailedException{};

    return variant;
}

template <>
ListPtr<IDataDescriptor> VariantConverter<IDataDescriptor>::ToDaqList(const OpcUaVariant& variant, const ContextPtr& /*context*/)
{
    if (variant.isType<UA_ExtensionObject>())
        return ListConversionUtils::ExtensionObjectVariantToList<IDataDescriptor>(variant);
    if (variant.isType<UA_DataDescriptorStructure>())
        return ListConversionUtils::VariantToList<IDataDescriptor, UA_DataDescriptorStructure>(variant);
    if (variant.isType<UA_StructDescriptorStructure>())
        return ListConversionUtils::VariantToList<IDataDescriptor, UA_StructDescriptorStructure>(variant);
    
    throw ConversionFailedException{};
}

template <>
OpcUaVariant VariantConverter<IDataDescriptor>::ToArrayVariant(const ListPtr<IDataDescriptor>& list,
                                                               const UA_DataType* targetType,
                                                               const ContextPtr& /*context*/)
{
    if (targetType == nullptr || targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_BASEDATADESCRIPTORSTRUCTURE])
        return ListConversionUtils::ToExtensionObjectArrayVariant<IDataDescriptor>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_DATADESCRIPTORSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataDescriptor, UA_DataDescriptorStructure>(list);
    if (targetType == &UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_STRUCTDESCRIPTORSTRUCTURE])
        return ListConversionUtils::ToArrayVariant<IDataDescriptor, UA_StructDescriptorStructure>(list);

    throw ConversionFailedException{};
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
