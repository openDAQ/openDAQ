#include <opendaq/server_type_impl.h>
#include <opendaq/server_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr serverTypeStructType = ServerTypeStructType();
}

ServerTypeImpl::ServerTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const FunctionPtr& createDefaultConfigCallback)
    : Super(detail::serverTypeStructType, id, name, description, createDefaultConfigCallback)
{
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, ServerType,
    IString*, id,
    IString*, name,
    IString*, description,
    IFunction*, createDefaultConfigCallback
)

END_NAMESPACE_OPENDAQ
