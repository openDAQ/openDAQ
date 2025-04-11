//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:02:11.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoreobjects/property_object.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID PROPERTY_OBJECT_INTF_ID = { daq::IPropertyObject::Id.Data1, daq::IPropertyObject::Id.Data2, daq::IPropertyObject::Id.Data3, daq::IPropertyObject::Id.Data4_UInt64 };

ErrCode PropertyObject_getClassName(PropertyObject* self, String** className)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getClassName(reinterpret_cast<daq::IString**>(className));
}

ErrCode PropertyObject_setPropertyValue(PropertyObject* self, String* propertyName, BaseObject* value)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->setPropertyValue(reinterpret_cast<daq::IString*>(propertyName), reinterpret_cast<daq::IBaseObject*>(value));
}

ErrCode PropertyObject_getPropertyValue(PropertyObject* self, String* propertyName, BaseObject** value)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getPropertyValue(reinterpret_cast<daq::IString*>(propertyName), reinterpret_cast<daq::IBaseObject**>(value));
}

ErrCode PropertyObject_getPropertySelectionValue(PropertyObject* self, String* propertyName, BaseObject** value)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getPropertySelectionValue(reinterpret_cast<daq::IString*>(propertyName), reinterpret_cast<daq::IBaseObject**>(value));
}

ErrCode PropertyObject_clearPropertyValue(PropertyObject* self, String* propertyName)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->clearPropertyValue(reinterpret_cast<daq::IString*>(propertyName));
}

ErrCode PropertyObject_hasProperty(PropertyObject* self, String* propertyName, Bool* hasProperty)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->hasProperty(reinterpret_cast<daq::IString*>(propertyName), hasProperty);
}

ErrCode PropertyObject_getProperty(PropertyObject* self, String* propertyName, Property** property)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getProperty(reinterpret_cast<daq::IString*>(propertyName), reinterpret_cast<daq::IProperty**>(property));
}

ErrCode PropertyObject_addProperty(PropertyObject* self, Property* property)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->addProperty(reinterpret_cast<daq::IProperty*>(property));
}

ErrCode PropertyObject_removeProperty(PropertyObject* self, String* propertyName)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->removeProperty(reinterpret_cast<daq::IString*>(propertyName));
}

ErrCode PropertyObject_getOnPropertyValueWrite(PropertyObject* self, String* propertyName, Event** event)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getOnPropertyValueWrite(reinterpret_cast<daq::IString*>(propertyName), reinterpret_cast<daq::IEvent**>(event));
}

ErrCode PropertyObject_getOnPropertyValueRead(PropertyObject* self, String* propertyName, Event** event)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getOnPropertyValueRead(reinterpret_cast<daq::IString*>(propertyName), reinterpret_cast<daq::IEvent**>(event));
}

ErrCode PropertyObject_getOnAnyPropertyValueWrite(PropertyObject* self, Event** event)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getOnAnyPropertyValueWrite(reinterpret_cast<daq::IEvent**>(event));
}

ErrCode PropertyObject_getOnAnyPropertyValueRead(PropertyObject* self, Event** event)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getOnAnyPropertyValueRead(reinterpret_cast<daq::IEvent**>(event));
}

ErrCode PropertyObject_getVisibleProperties(PropertyObject* self, List** properties)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getVisibleProperties(reinterpret_cast<daq::IList**>(properties));
}

ErrCode PropertyObject_getAllProperties(PropertyObject* self, List** properties)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getAllProperties(reinterpret_cast<daq::IList**>(properties));
}

ErrCode PropertyObject_setPropertyOrder(PropertyObject* self, List* orderedPropertyNames)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->setPropertyOrder(reinterpret_cast<daq::IList*>(orderedPropertyNames));
}

ErrCode PropertyObject_beginUpdate(PropertyObject* self)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->beginUpdate();
}

ErrCode PropertyObject_endUpdate(PropertyObject* self)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->endUpdate();
}

ErrCode PropertyObject_getUpdating(PropertyObject* self, Bool* updating)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getUpdating(updating);
}

ErrCode PropertyObject_getOnEndUpdate(PropertyObject* self, Event** event)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getOnEndUpdate(reinterpret_cast<daq::IEvent**>(event));
}

ErrCode PropertyObject_getPermissionManager(PropertyObject* self, PermissionManager** permissionManager)
{
    return reinterpret_cast<daq::IPropertyObject*>(self)->getPermissionManager(reinterpret_cast<daq::IPermissionManager**>(permissionManager));
}

ErrCode PropertyObject_createPropertyObject(PropertyObject** obj)
{
    daq::IPropertyObject* ptr = nullptr;
    ErrCode err = daq::createPropertyObject(&ptr);
    *obj = reinterpret_cast<PropertyObject*>(ptr);
    return err;
}

ErrCode PropertyObject_createPropertyObjectWithClassAndManager(PropertyObject** obj, TypeManager* manager, String* className)
{
    daq::IPropertyObject* ptr = nullptr;
    ErrCode err = daq::createPropertyObjectWithClassAndManager(&ptr, reinterpret_cast<daq::ITypeManager*>(manager), reinterpret_cast<daq::IString*>(className));
    *obj = reinterpret_cast<PropertyObject*>(ptr);
    return err;
}
