/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/struct_type_ptr.h>
#include <coretypes/type_impl.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

class StructTypeImpl : public GenericTypeImpl<IStructType>
{
public:
    explicit StructTypeImpl(StringPtr name, ListPtr<IString> names, ListPtr<IBaseObject> defaultValues, ListPtr<IType> types);
    explicit StructTypeImpl(StringPtr name, ListPtr<IString> names, ListPtr<IType> types);

    ErrCode INTERFACE_FUNC getFieldNames(IList** names) override;
    ErrCode INTERFACE_FUNC getFieldDefaultValues(IList** defaultValues) override;
    ErrCode INTERFACE_FUNC getFieldTypes(IList** types) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();

    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    ListPtr<IString> names;
    ListPtr<IBaseObject> defaultValues;
    ListPtr<IType> types;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StructTypeImpl)

END_NAMESPACE_OPENDAQ
