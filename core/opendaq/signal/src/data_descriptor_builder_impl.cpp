#include <opendaq/data_descriptor_builder_impl.h>
#include <coretypes/coretype_utils.h>
#include <coretypes/validation.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/data_descriptor_factory.h>

BEGIN_NAMESPACE_OPENDAQ

DataDescriptorBuilderImpl::DataDescriptorBuilderImpl()
    : dimensions(List<IDimension>())
    , name("")
    , sampleType(SampleType::Undefined)
    , unit(nullptr)
    , valueRange(nullptr)
    , dataRule(ExplicitDataRule())
    , scaling(nullptr)
    , origin("")
    , resolution(nullptr)
    , structFields(List<IDataDescriptor>())
    , metadata(Dict<IString, IString>())
{
}

static ListPtr<IDimension> copyDimensions(const ListPtr<IDimension>& toCopy)
{
    auto newDimensions = List<IDimension>();
    if (toCopy.assigned())
    {
        for (auto dim : toCopy)
        {
            newDimensions.pushBack(dim);
        }
    }
    return newDimensions;
}

static ListPtr<IDataDescriptor> copyStructFields(const ListPtr<IDataDescriptor>& toCopy)
{
    auto newStructFields = List<IDataDescriptor>();
    if (toCopy.assigned())
    {
        for (auto desc : toCopy)
        {
            newStructFields.pushBack(desc);
        }
    }
    return newStructFields;
}

static DictPtr<IString, IString> copyMetadata(const DictPtr<IString, IString>& toCopy)
{
    auto newMetaData = Dict<IString, IString>();

    if (toCopy.assigned())
    {
        for (const auto& [k, v] : toCopy)
        {
            newMetaData.set(k, v);
        }
    }
    return newMetaData;
}

DataDescriptorBuilderImpl::DataDescriptorBuilderImpl(const DataDescriptorPtr& descriptorCopy)
    : dimensions(copyDimensions(descriptorCopy.getDimensions()))
    , name(descriptorCopy.getName())
    , sampleType(descriptorCopy.getSampleType())
    , unit(descriptorCopy.getUnit())
    , valueRange(descriptorCopy.getValueRange())
    , dataRule(descriptorCopy.getRule())
    , scaling(descriptorCopy.getPostScaling())
    , origin(descriptorCopy.getOrigin())
    , resolution(descriptorCopy.getTickResolution())
    , structFields(copyStructFields(descriptorCopy.getStructFields()))
    , metadata(copyMetadata(descriptorCopy.getMetadata()))
{
}

ErrCode DataDescriptorBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}
ErrCode DataDescriptorBuilderImpl::setDimensions(IList* dimensions)
{
    this->dimensions = dimensions;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setSampleType(SampleType sampleType)
{
    this->sampleType = sampleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setUnit(IUnit* unit)
{
    this->unit = unit;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setValueRange(IRange* valueRange)
{
    this->valueRange = valueRange;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setRule(IDataRule* rule)
{
    this->dataRule = rule;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setOrigin(IString* origin)
{
    this->origin = origin;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setTickResolution(IRatio* tickResolution)
{
    this->resolution = tickResolution;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setPostScaling(IScaling* scaling)
{
    this->scaling = scaling;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setMetadata(IDict* metadata)
{
    this->metadata = metadata;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setStructFields(IList* structFields)
{
    this->structFields = structFields;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::build(IDataDescriptor** dataDescriptor)
{
    OPENDAQ_PARAM_NOT_NULL(dataDescriptor);

    return daqTry([&]()
    {
        auto descriptor = DataDescriptor(packBuildParams());
        *dataDescriptor = descriptor.detach();
        return OPENDAQ_SUCCESS;
    });
}

DictPtr<IString, IBaseObject> DataDescriptorBuilderImpl::packBuildParams()
{
    auto params = Dict<IString, IBaseObject>();
    params.set("dimensions", copyDimensions(dimensions));
    params.set("name", name);
    params.set("sampleType", static_cast<Int>(sampleType));
    params.set("unit", unit);
    params.set("valueRange", valueRange);
    params.set("dataRule", dataRule);
    params.set("scaling", scaling);
    params.set("origin", origin);
    params.set("tickResolution", resolution);
    params.set("structFields", copyStructFields(structFields));
    params.set("metadata", copyMetadata(metadata));

    return params;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    DataDescriptorBuilder,
    IDataDescriptorBuilder,
    createDataDescriptorBuilder,
    )

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, DataDescriptorBuilder, IDataDescriptorBuilder, createDataDescriptorBuilderFromExisting,
    IDataDescriptor*, descriptorToCopy
)

END_NAMESPACE_OPENDAQ
