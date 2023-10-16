#include <opendaq/device_type_impl.h>
#include <opendaq/device_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr deviceTypeStructType = DeviceTypeStructType();
}

DeviceTypeImpl::DeviceTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const FunctionPtr& createDefaultConfigCallback)
    : Super(detail::deviceTypeStructType, id, name, description, createDefaultConfigCallback)
{
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, DeviceType,
    IString*, id,
    IString*, name,
    IString*, description,
    IFunction*, createDefaultConfigCallback
)

END_NAMESPACE_OPENDAQ
