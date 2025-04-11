#include "ccoreobjects/property_value_event_args.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID PROPERTY_VALUE_EVENT_ARGS_INTF_ID = { daq::IPropertyValueEventArgs::Id.Data1, daq::IPropertyValueEventArgs::Id.Data2, daq::IPropertyValueEventArgs::Id.Data3, daq::IPropertyValueEventArgs::Id.Data4_UInt64 };

ErrCode PropertyValueEventArgs_getProperty(PropertyValueEventArgs* self, Property** property)
{
    return reinterpret_cast<daq::IPropertyValueEventArgs*>(self)->getProperty(reinterpret_cast<daq::IProperty**>(property));
}

ErrCode PropertyValueEventArgs_getValue(PropertyValueEventArgs* self, BaseObject** value)
{
    return reinterpret_cast<daq::IPropertyValueEventArgs*>(self)->getValue(reinterpret_cast<daq::IBaseObject**>(value));
}

ErrCode PropertyValueEventArgs_setValue(PropertyValueEventArgs* self, BaseObject* value)
{
    return reinterpret_cast<daq::IPropertyValueEventArgs*>(self)->setValue(reinterpret_cast<daq::IBaseObject*>(value));
}

ErrCode PropertyValueEventArgs_getPropertyEventType(PropertyValueEventArgs* self, PropertyEventType* changeType)
{
    daq::PropertyEventType t;
    ErrCode err = reinterpret_cast<daq::IPropertyValueEventArgs*>(self)->getPropertyEventType(&t);
    *changeType = copendaq::utils::toCPropertyEventType(t);
    return err;
}

ErrCode PropertyValueEventArgs_getIsUpdating(PropertyValueEventArgs* self, Bool* isUpdating)
{
    return reinterpret_cast<daq::IPropertyValueEventArgs*>(self)->getIsUpdating(isUpdating);
}

ErrCode PropertyValueEventArgs_getOldValue(PropertyValueEventArgs* self, BaseObject** value)
{
    return reinterpret_cast<daq::IPropertyValueEventArgs*>(self)->getOldValue(reinterpret_cast<daq::IBaseObject**>(value));
}

ErrCode PropertyValueEventArgs_createPropertyValueEventArgs(PropertyValueEventArgs** obj, Property* prop, BaseObject* value, BaseObject* oldValue, PropertyEventType type, Bool isUpdating)
{
    daq::IPropertyValueEventArgs* ptr = nullptr;
    ErrCode err = daq::createPropertyValueEventArgs(&ptr, reinterpret_cast<daq::IProperty*>(prop), reinterpret_cast<daq::IBaseObject*>(value), reinterpret_cast<daq::IBaseObject*>(oldValue), copendaq::utils::toDaqPropertyEventType(type), isUpdating);
    *obj = reinterpret_cast<PropertyValueEventArgs*>(ptr);
    return err;
}
