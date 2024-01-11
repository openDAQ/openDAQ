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
#include <coretypes/enumeration.h>
#include <coretypes/coretype.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/enumeration_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

class EnumerationImpl : public ImplementationOf<IEnumeration, IConvertible, ICoreType, ISerializable>
{
public:
    explicit EnumerationImpl(const StringPtr& name, const StringPtr& value, const TypeManagerPtr& typeManager);
    explicit EnumerationImpl(const EnumerationTypePtr& type, const StringPtr& value);

    // IBaseObject
    ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) override;
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    // IConvertible
    ErrCode INTERFACE_FUNC toFloat(Float* val) override;
    ErrCode INTERFACE_FUNC toInt(Int* val) override;
    ErrCode INTERFACE_FUNC toBool(Bool* val) override;

    // ICoreType
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

    // IEnumeration
    ErrCode INTERFACE_FUNC getEnumerationType(IEnumerationType** type) override;
    ErrCode INTERFACE_FUNC getValue(IString** value) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IBaseObject** obj);

protected:
    EnumerationTypePtr enumerationType;
    StringPtr value;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(EnumerationImpl)

END_NAMESPACE_OPENDAQ
