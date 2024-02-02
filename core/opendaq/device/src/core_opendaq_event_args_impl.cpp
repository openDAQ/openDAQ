#include <coreobjects/core_event_args_impl.h>
#include <opendaq/core_opendaq_event_args.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsComponentAdded(ICoreEventArgs** objTmp, IComponent* component)
{
    const auto dict = Dict<IString, IBaseObject>({{"Component", component}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::ComponentAdded,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsComponentRemoved(ICoreEventArgs** objTmp, IString* componentId)
{
    const auto dict = Dict<IString, IBaseObject>({{"Id", componentId}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::ComponentRemoved,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsSignalConnected(ICoreEventArgs** objTmp, ISignal* signal)
{
    const auto dict = Dict<IString, IBaseObject>({{"Signal", signal}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::SignalConnected,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsSignalDisconnected(ICoreEventArgs** objTmp)
{
    const auto dict = Dict<IString, IBaseObject>();
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::SignalDisconnected,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsDataDescriptorChanged(ICoreEventArgs** objTmp, IDataDescriptor* descriptor)
{
    const auto dict = Dict<IString, IBaseObject>({{"DataDescriptor", descriptor}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::DataDescriptorChanged,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsComponentUpdateEnd(ICoreEventArgs** objTmp)
{
    const auto dict = Dict<IString, IBaseObject>({});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::ComponentUpdateEnd,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsAttributeChanged(ICoreEventArgs** objTmp, IString* attributeName, IBaseObject* attributeValue)
{
    const auto dict = Dict<IString, IBaseObject>({{"AttributeName", attributeName}, {attributeName, attributeValue}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::AttributeChanged, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsTagsChanged(ICoreEventArgs** objTmp, IList* tags)
{
    const auto dict = Dict<IString, IBaseObject>({{"Tags", tags}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::TagsChanged, dict);
}

#endif

END_NAMESPACE_OPENDAQ
