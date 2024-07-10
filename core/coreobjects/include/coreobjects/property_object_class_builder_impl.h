/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coreobjects/property_object_class_builder.h>
#include <coreobjects/property_ptr.h>
#include <coretypes/coretypes.h>
#include <coretypes/type_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

struct ISerializedObject;

class PropertyObjectClassBuilderImpl : public ImplementationOf<IPropertyObjectClassBuilder>
{
public:
    explicit PropertyObjectClassBuilderImpl(StringPtr name);
    PropertyObjectClassBuilderImpl(const TypeManagerPtr& manager, StringPtr name);

    ErrCode INTERFACE_FUNC build(IPropertyObjectClass** propertyObjectClass) override;

    ErrCode INTERFACE_FUNC setName(IString* className) override;
    ErrCode INTERFACE_FUNC getName(IString** className) override;

    ErrCode INTERFACE_FUNC setParentName(IString* parentName) override;
    ErrCode INTERFACE_FUNC getParentName(IString** parentName) override;

    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC getProperties(IDict** properties) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;

    ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) override;
    ErrCode INTERFACE_FUNC getPropertyOrder(IList** orderedPropertyNames) override;

    ErrCode INTERFACE_FUNC getManager(ITypeManager** manager) override;

private:
    StringPtr name;
    StringPtr parent;
    DictPtr<IString, IProperty> props;
    ListPtr<IString> customOrder;
    WeakRefPtr<ITypeManager> manager;

    bool hasDuplicateReferences(const PropertyPtr& prop) const;
    ListPtr<IProperty> getProperties() const;
};

END_NAMESPACE_OPENDAQ
