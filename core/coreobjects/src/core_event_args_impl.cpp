#include <coreobjects/core_event_args_impl.h>
#include <coreobjects/property_ptr.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

static std::string getCoreEventName(const Int id)
{
    switch(id)
    {
        case core_event_ids::PropertyValueChanged:
            return "PropertyValueChanged";
        case core_event_ids::UpdateEnd:
            return "UpdateEnd";
        case core_event_ids::PropertyAdded:
            return "PropertyAdded";
        case core_event_ids::PropertyRemoved:
            return "PropertyRemoved";
        default:
            break;
    }

    return "Unknown";
}

CoreEventArgsImpl::CoreEventArgsImpl(Int id, const DictPtr<IString, IBaseObject>& parameters)
    : EventArgsImplTemplate<ICoreEventArgs>(id, getCoreEventName(id))
    , parameters(parameters)
{
    if (!validateParameters())
        throw InvalidParameterException{"Core event parameters for event type \"{}\" are invalid", this->eventName};
}

ErrCode CoreEventArgsImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

bool CoreEventArgsImpl::validateParameters() const
{
    switch(eventId)
    {
        case core_event_ids::PropertyValueChanged:
            return parameters.hasKey("Name") && parameters.hasKey("Value") && parameters.getCount() == 2;
        case core_event_ids::UpdateEnd:
            return parameters.hasKey("UpdatedProperties") && parameters.get("UpdatedProperties").asPtrOrNull<IDict>().assigned() && parameters.getCount() == 1;
        case core_event_ids::PropertyAdded:
            return parameters.hasKey("Property") && parameters.getCount() == 1;
        case core_event_ids::PropertyRemoved:
            return parameters.hasKey("Name") && parameters.getCount() == 1;
        default:
            break;
    }

    return true;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, CoreEventArgs, Int, eventId, IDict*, parameters)

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyValueChanged(ICoreEventArgs** objTmp, IString* propName, IBaseObject* value)
{
    const auto dict = Dict<IString, IBaseObject>({{"Name", propName}, {"Value", value}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyValueChanged,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsUpdateEnd(ICoreEventArgs** objTmp, IDict* updatedProperties)
{
    const auto dict = Dict<IString, IBaseObject>({{"UpdatedProperties", updatedProperties}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::UpdateEnd,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyAdded(ICoreEventArgs** objTmp, IProperty* prop)
{
    const auto dict = Dict<IString, IBaseObject>({{"Property", prop}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyAdded,dict);
}

extern "C"
ErrCode PUBLIC_EXPORT createCoreEventArgsPropertyRemoved(ICoreEventArgs** objTmp, IString* propName)
{
    const auto dict = Dict<IString, IBaseObject>({{"Name", propName}});
    return daq::createObject<ICoreEventArgs, CoreEventArgsImpl, Int, IDict*>(objTmp, core_event_ids::PropertyRemoved,dict);
}

#endif

END_NAMESPACE_OPENDAQ
