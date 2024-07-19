#include <opendaq/device_type_impl.h>
#include <opendaq/device_type_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr deviceTypeStructType = DeviceTypeStructType();
}

DeviceTypeImpl::DeviceTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const PropertyObjectPtr& defaultConfig,
                               const StringPtr& prefix)
    : Super(detail::deviceTypeStructType, id, name, description, prefix, defaultConfig)
{
}

DeviceTypeImpl::DeviceTypeImpl(const ComponentTypeBuilderPtr& builder)
    : DeviceTypeImpl(builder.getId(), builder.getName(), builder.getDescription(), builder.getDefaultConfig(), builder.getConnectionStringPrefix())
{
}

ErrCode DeviceTypeImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    DeviceType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IPropertyObject*,
    defaultConfig,
    IString*,
    prefix
    )

END_NAMESPACE_OPENDAQ
