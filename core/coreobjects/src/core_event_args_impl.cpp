#include <coreobjects/core_event_args_impl.h>
#include <coreobjects/property_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, CoreEventArgs, CoreEventId, eventId, IString*, eventName, IDict*, parameters)

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyValueChanged(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IString* propName, IBaseObject* value, IString* path)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner}, {"Name", propName}, {"Value", value}, {"Path", path}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::PropertyValueChanged, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyObjectUpdateEnd(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IDict* updatedProperties, IString* path)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner},{"UpdatedProperties", updatedProperties}, {"Path", path}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::PropertyObjectUpdateEnd, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyAdded(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IProperty* prop, IString* path)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner}, {"Property", prop}, {"Path", path}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::PropertyAdded, dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyRemoved(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IString* propName, IString* path)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner}, {"Name", propName}, {"Path", path}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, CoreEventId, IDict*>(objTmp, CoreEventId::PropertyRemoved, dict);
}

#endif

END_NAMESPACE_OPENDAQ
