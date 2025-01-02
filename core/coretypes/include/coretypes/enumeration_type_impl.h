/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coretypes/enumeration_type_ptr.h>
#include <coretypes/type_impl.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

class EnumerationTypeImpl : public GenericTypeImpl<IEnumerationType>
{
public:
    explicit EnumerationTypeImpl(StringPtr typeName, DictPtr<IString, IInteger> enumerators);
    explicit EnumerationTypeImpl(StringPtr typeName, ListPtr<IString> enumeratorNames, Int firstEnumeratorIntValue);

    // IEnumerationType
    ErrCode INTERFACE_FUNC getEnumeratorNames(IList** names) override;
    ErrCode INTERFACE_FUNC getAsDictionary(IDict** dictionary) override;
    ErrCode INTERFACE_FUNC getEnumeratorIntValue(IString* name, Int* value) override;
    ErrCode INTERFACE_FUNC getCount(SizeT* count) override;

    // IBaseObject
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    DictPtr<IString, IInteger> enumerators;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(EnumerationTypeImpl)

END_NAMESPACE_OPENDAQ
