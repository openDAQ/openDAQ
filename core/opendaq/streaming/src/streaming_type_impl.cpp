#include <opendaq/streaming_type_impl.h>

BEGIN_NAMESPACE_OPENDAQ

StreamingTypeImpl::StreamingTypeImpl(const StringPtr& id,
                                            const StringPtr& name,
                                            const StringPtr& description,
                                            const StringPtr& prefix,
                                            const PropertyObjectPtr& defaultConfig)
    : Super(StreamingTypeStructType(), id, name, description, prefix, defaultConfig)
{
}

StreamingTypeImpl::StreamingTypeImpl(const ComponentTypeBuilderPtr& builder)
    : StreamingTypeImpl(builder.getId(), builder.getName(), builder.getDescription(),builder.getConnectionStringPrefix(), builder.getDefaultConfig())
{
}

ErrCode StreamingTypeImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    StreamingType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IString*,
    prefix,
    IPropertyObject*,
    defaultConfig
    )

END_NAMESPACE_OPENDAQ
