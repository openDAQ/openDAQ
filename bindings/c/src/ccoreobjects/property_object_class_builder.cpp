//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:02:12.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoreobjects/property_object_class_builder.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID PROPERTY_OBJECT_CLASS_BUILDER_INTF_ID = { daq::IPropertyObjectClassBuilder::Id.Data1, daq::IPropertyObjectClassBuilder::Id.Data2, daq::IPropertyObjectClassBuilder::Id.Data3, daq::IPropertyObjectClassBuilder::Id.Data4_UInt64 };

ErrCode PropertyObjectClassBuilder_build(PropertyObjectClassBuilder* self, PropertyObjectClass** propertyObjectClass)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->build(reinterpret_cast<daq::IPropertyObjectClass**>(propertyObjectClass));
}

ErrCode PropertyObjectClassBuilder_setName(PropertyObjectClassBuilder* self, String* className)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->setName(reinterpret_cast<daq::IString*>(className));
}

ErrCode PropertyObjectClassBuilder_getName(PropertyObjectClassBuilder* self, String** className)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->getName(reinterpret_cast<daq::IString**>(className));
}

ErrCode PropertyObjectClassBuilder_setParentName(PropertyObjectClassBuilder* self, String* parentName)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->setParentName(reinterpret_cast<daq::IString*>(parentName));
}

ErrCode PropertyObjectClassBuilder_getParentName(PropertyObjectClassBuilder* self, String** parentName)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->getParentName(reinterpret_cast<daq::IString**>(parentName));
}

ErrCode PropertyObjectClassBuilder_addProperty(PropertyObjectClassBuilder* self, Property* property)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->addProperty(reinterpret_cast<daq::IProperty*>(property));
}

ErrCode PropertyObjectClassBuilder_getProperties(PropertyObjectClassBuilder* self, Dict** properties)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->getProperties(reinterpret_cast<daq::IDict**>(properties));
}

ErrCode PropertyObjectClassBuilder_removeProperty(PropertyObjectClassBuilder* self, String* propertyName)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->removeProperty(reinterpret_cast<daq::IString*>(propertyName));
}

ErrCode PropertyObjectClassBuilder_setPropertyOrder(PropertyObjectClassBuilder* self, List* orderedPropertyNames)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->setPropertyOrder(reinterpret_cast<daq::IList*>(orderedPropertyNames));
}

ErrCode PropertyObjectClassBuilder_getPropertyOrder(PropertyObjectClassBuilder* self, List** orderedPropertyNames)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->getPropertyOrder(reinterpret_cast<daq::IList**>(orderedPropertyNames));
}

ErrCode PropertyObjectClassBuilder_getManager(PropertyObjectClassBuilder* self, TypeManager** manager)
{
    return reinterpret_cast<daq::IPropertyObjectClassBuilder*>(self)->getManager(reinterpret_cast<daq::ITypeManager**>(manager));
}

ErrCode PropertyObjectClassBuilder_createPropertyObjectClassBuilder(PropertyObjectClassBuilder** obj, String* name)
{
    daq::IPropertyObjectClassBuilder* ptr = nullptr;
    ErrCode err = daq::createPropertyObjectClassBuilder(&ptr, reinterpret_cast<daq::IString*>(name));
    *obj = reinterpret_cast<PropertyObjectClassBuilder*>(ptr);
    return err;
}

ErrCode PropertyObjectClassBuilder_createPropertyObjectClassBuilderWithManager(PropertyObjectClassBuilder** obj, TypeManager* manager, String* name)
{
    daq::IPropertyObjectClassBuilder* ptr = nullptr;
    ErrCode err = daq::createPropertyObjectClassBuilderWithManager(&ptr, reinterpret_cast<daq::ITypeManager*>(manager), reinterpret_cast<daq::IString*>(name));
    *obj = reinterpret_cast<PropertyObjectClassBuilder*>(ptr);
    return err;
}
