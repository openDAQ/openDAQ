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

#include <coreobjects/argument_info.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class ArgumentInfoImpl : public GenericStructImpl<IArgumentInfo, IStruct>
{
public:
    ArgumentInfoImpl(StringPtr name, CoreType type);

    ErrCode INTERFACE_FUNC getName(IString** argName) override;
    ErrCode INTERFACE_FUNC getType(CoreType* type) override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);

private:
    StringPtr name;
    CoreType argType;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ArgumentInfoImpl)

END_NAMESPACE_OPENDAQ
