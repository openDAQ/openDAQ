#include <opendaq/add_streaming_config_impl.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_config
{
static const char* TypeConfig = "TypeConfig";
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType)
    : AddStreamingConfigImpl(streamingType, Partial())
{
    Super::addProperty(ObjectProperty(add_streaming_config::TypeConfig, streamingType.createDefaultConfig()));
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType, Partial)
    : Super()
    , streamingType(streamingType)
{
}

ErrCode AddStreamingConfigImpl::getStreamingType(IStreamingType** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = streamingType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::getTypeConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&] { *config = objPtr.getPropertyValue(add_streaming_config::TypeConfig).asPtr<IPropertyObject>().detach(); });
}

ErrCode AddStreamingConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddStreamingConfigImpl>(streamingType, Partial());
        dynamic_cast<AddStreamingConfigImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
        *cloned = obj.detach();
    });
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" ErrCode PUBLIC_EXPORT createAddStreamingConfig(IAddStreamingConfig** objTmp, IStreamingType* streamingType)
{
    return daq::createObject<IAddStreamingConfig, AddStreamingConfigImpl>(objTmp, streamingType);
}

#endif

END_NAMESPACE_OPENDAQ
