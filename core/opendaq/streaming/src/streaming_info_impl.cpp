#include <opendaq/streaming_info_impl.h>
#include <coretypes/validation.h>
#include "coretypes/impl.h"
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

StreamingInfoConfigImpl::StreamingInfoConfigImpl(const StringPtr& protocolId)
    : Super()
{
    Super::addProperty(StringPropertyBuilder("protocolId", protocolId).setReadOnly(true).build());
    Super::addProperty(StringProperty("address", ""));
}

ErrCode StreamingInfoConfigImpl::setPrimaryAddress(IString* address)
{
    return daqTry([&]() {
        setStringProperty("address", address);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode StreamingInfoConfigImpl::getPrimaryAddress(IString** address)
{
    return daqTry([&]() {
        *address = getStringProperty("address").detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode StreamingInfoConfigImpl::getProtocolId(IString** protocolId)
{
    return daqTry([&]() {
        *protocolId = getStringProperty("protocolId").detach();
        return OPENDAQ_SUCCESS;
    });
}

void StreamingInfoConfigImpl::setStringProperty(const StringPtr& name, const BaseObjectPtr& value)
{
    auto obj = this->template borrowPtr<PropertyObjectPtr>();
    obj.setPropertyValue(name, value);
}

StringPtr StreamingInfoConfigImpl::getStringProperty(const StringPtr& name)
{
    const auto obj = this->template borrowPtr<PropertyObjectPtr>();
    return obj.getPropertyValue(name).template asPtr<IString>();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, StreamingInfoConfigImpl,
    IStreamingInfoConfig, createStreamingInfoConfig,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
