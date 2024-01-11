#include <coreobjects/core_event_args_impl.h>
#include <coreobjects/property_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, CoreEventArgs, Int, eventId, IDict*, parameters)

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyValueChanged(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IString* propName, IBaseObject* value)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner}, {"Name", propName}, {"Value", value}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyValueChanged,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyObjectUpdateEnd(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IDict* updatedProperties)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner},{"UpdatedProperties", updatedProperties}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyObjectUpdateEnd,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyAdded(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IProperty* prop)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner}, {"Property", prop}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyAdded,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyRemoved(ICoreEventArgs** objTmp, IPropertyObject* propOwner, IString* propName)
{
    const auto dict = Dict<IString, IBaseObject>({{"Owner", propOwner}, {"Name", propName}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyRemoved,dict);
}

#endif

END_NAMESPACE_OPENDAQ
