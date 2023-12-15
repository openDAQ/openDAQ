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
#include <coretypes/simple_type.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/type_impl.h>
#include <string>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

class SimpleTypeImpl : public GenericTypeImpl<ISimpleType>
{
public:
    SimpleTypeImpl(CoreType coreType);
    
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();

    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static std::string coreTypeToString(CoreType coreType);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SimpleTypeImpl)

END_NAMESPACE_OPENDAQ
