#include <coreobjects/core_event_args_impl.h>
#include <opendaq/core_opendaq_event_args.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsComponentAdded(ICoreEventArgs** objTmp, IComponent* component)
{
    const auto dict = Dict<IString, IBaseObject>({{"Component", component}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::ComponentAdded, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsComponentRemoved(ICoreEventArgs** objTmp, IString* componentId)
{
    const auto dict = Dict<IString, IBaseObject>({{"Id", componentId}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::ComponentRemoved, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsSignalConnected(ICoreEventArgs** objTmp, ISignal* signal)
{
    const auto dict = Dict<IString, IBaseObject>({{"Signal", signal}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::SignalConnected, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsSignalDisconnected(ICoreEventArgs** objTmp)
{
    const auto dict = Dict<IString, IBaseObject>();
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::SignalDisconnected, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsDataDescriptorChanged(ICoreEventArgs** objTmp, IDataDescriptor* descriptor)
{
    const auto dict = Dict<IString, IBaseObject>({{"DataDescriptor", descriptor}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::DataDescriptorChanged, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsComponentUpdateEnd(ICoreEventArgs** objTmp)
{
    const auto dict = Dict<IString, IBaseObject>({});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::ComponentUpdateEnd, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsAttributeChanged(ICoreEventArgs** objTmp, IString* attributeName, IBaseObject* attributeValue)
{
    const auto dict = Dict<IString, IBaseObject>({{"AttributeName", attributeName}, {attributeName, attributeValue}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::AttributeChanged, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsTagsChanged(ICoreEventArgs** objTmp, IList* tags)
{
    const auto dict = Dict<IString, IBaseObject>({{"Tags", tags}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::TagsChanged, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsDeviceDomainChanged(ICoreEventArgs** objTmp, IDeviceDomain* deviceDomain)
{
    const auto dict = Dict<IString, IBaseObject>({{"DeviceDomain", deviceDomain}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::DeviceDomainChanged, dict);
}

#endif

END_NAMESPACE_OPENDAQ
