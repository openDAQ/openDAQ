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
                               const PropertyObjectPtr& defaultConfig)
    : Super(detail::serverTypeStructType, id, name, description, defaultConfig)
{
}

ServerTypeImpl::ServerTypeImpl(const ComponentTypeBuilderPtr& builder)
    : ServerTypeImpl(builder.getId(), builder.getName(), builder.getDescription(), builder.getDefaultConfig())
{
}

ErrCode ServerTypeImpl::createAuthenticationConfig(IAuthenticationConfig** /*config*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "Server types do not support authentication");
}

ErrCode ServerTypeImpl::getSupportedAuthenticationMethods(IDict** /*authenticationMethods*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "Server types do not support authentication");
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    ServerType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IPropertyObject*,
    defaultConfig
    )

END_NAMESPACE_OPENDAQ
