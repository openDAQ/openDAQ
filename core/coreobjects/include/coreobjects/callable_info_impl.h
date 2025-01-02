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

#include <coretypes/intfs.h>
#include <coreobjects/callable_info.h>
#include <coreobjects/argument_info_ptr.h>
#include <coretypes/list_factory.h>
#include <coretypes/struct_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class CallableInfoImpl : public GenericStructImpl<ICallableInfo, IStruct>
{
public:
    CallableInfoImpl(ListPtr<IArgumentInfo> arguments, CoreType returnType, Bool constFlag);

    ErrCode INTERFACE_FUNC getReturnType(CoreType* type) override;
    ErrCode INTERFACE_FUNC getArguments(IList** argumentInfo) override;
    ErrCode INTERFACE_FUNC isConst(Bool* constFlag) override;
    
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    CoreType returnType{};
    ListPtr<IArgumentInfo> arguments;
    Bool constFlag = false;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(CallableInfoImpl)

END_NAMESPACE_OPENDAQ
