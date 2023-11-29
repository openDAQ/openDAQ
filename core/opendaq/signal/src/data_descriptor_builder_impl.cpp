#include <coretypes/coretype_utils.h>
#include <coretypes/validation.h>
#include <opendaq/data_descriptor_builder_impl.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/scaling_factory.h>

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

DataDescriptorBuilderImpl::DataDescriptorBuilderImpl(const DataDescriptorPtr& descriptorCopy)
    : dimensions(descriptorCopy.getDimensions())
    , name(descriptorCopy.getName())
    , sampleType(descriptorCopy.getSampleType())
    , unit(descriptorCopy.getUnit())
    , valueRange(descriptorCopy.getValueRange())
    , dataRule(descriptorCopy.getRule())
    , scaling(descriptorCopy.getPostScaling())
    , origin(descriptorCopy.getOrigin())
    , resolution(descriptorCopy.getTickResolution())
    , structFields(descriptorCopy.getStructFields())
    , metadata(descriptorCopy.getMetadata())
{
}

ErrCode DataDescriptorBuilderImpl::build(IDataDescriptor** dataDescriptor)
{
    OPENDAQ_PARAM_NOT_NULL(dataDescriptor);

    const auto builder = this->borrowPtr<DataDescriptorBuilderPtr>();

    return daqTry([&]()
    {
        *dataDescriptor = DataDescriptorFromBuilder(builder).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode DataDescriptorBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setDimensions(IList* dimensions)
{
    if (dimensions == nullptr)
        this->dimensions = List<IDimension>();
    else
        this->dimensions = dimensions;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getDimensions(IList** dimensions)
{
    OPENDAQ_PARAM_NOT_NULL(dimensions);
    *dimensions = this->dimensions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setSampleType(SampleType sampleType)
{
    this->sampleType = sampleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getSampleType(SampleType* sampleType)
{
    OPENDAQ_PARAM_NOT_NULL(sampleType);
    *sampleType = this->sampleType;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setUnit(IUnit* unit)
{
    this->unit = unit;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);
    *unit = this->unit.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setValueRange(IRange* valueRange)
{
    this->valueRange = valueRange;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getValueRange(IRange** valueRange)
{
    OPENDAQ_PARAM_NOT_NULL(valueRange);
    *valueRange = this->valueRange.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setRule(IDataRule* rule)
{
    this->dataRule = rule;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getRule(IDataRule** rule)
{
    OPENDAQ_PARAM_NOT_NULL(rule);
    *rule = this->dataRule.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setOrigin(IString* origin)
{
    this->origin = origin;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getOrigin(IString** origin)
{
    OPENDAQ_PARAM_NOT_NULL(origin);
    *origin = this->origin.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setTickResolution(IRatio* tickResolution)
{
    this->resolution = tickResolution;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getTickResolution(IRatio** tickResolution)
{
    OPENDAQ_PARAM_NOT_NULL(tickResolution);
    *tickResolution = this->resolution.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setPostScaling(IScaling* scaling)
{
    this->scaling = scaling;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getPostScaling(IScaling** scaling)
{
    OPENDAQ_PARAM_NOT_NULL(scaling);
    *scaling = this->scaling.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setMetadata(IDict* metadata)
{
    if (metadata == nullptr)
        this->metadata = Dict<IString, IString>();
    else
        this->metadata = metadata;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getMetadata(IDict** metadata)
{
    OPENDAQ_PARAM_NOT_NULL(metadata);
    *metadata = this->metadata.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::setStructFields(IList* structFields)
{
    if (structFields == nullptr)
        this->structFields = List<IDataDescriptor>();
    else
        this->structFields = structFields;
    return OPENDAQ_SUCCESS;
}

ErrCode DataDescriptorBuilderImpl::getStructFields(IList** structFields)
{
    OPENDAQ_PARAM_NOT_NULL(structFields);
    *structFields = this->structFields.addRefAndReturn();
    return OPENDAQ_SUCCESS;
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
