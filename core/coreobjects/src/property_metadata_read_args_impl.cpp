#include <coreobjects/property_metadata_read_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

PropertyMetadataReadArgsImpl::PropertyMetadataReadArgsImpl(PropertyPtr property)
    : EventArgsImplTemplate(0, "PropertyMetadataRead")
    , property(std::move(property))
{
}

ErrCode PropertyMetadataReadArgsImpl::getProperty(IProperty** prop)
{
    OPENDAQ_PARAM_NOT_NULL(prop);

    *prop = property.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyMetadataReadArgsImpl::getValue(IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = this->value.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyMetadataReadArgsImpl::setValue(IBaseObject* value)
{
    this->value = value;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, PropertyMetadataReadArgs,
    IProperty*, prop
)

END_NAMESPACE_OPENDAQ
