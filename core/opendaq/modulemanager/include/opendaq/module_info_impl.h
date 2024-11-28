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
#include <coretypes/struct_impl.h>
#include <opendaq/module_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
class ModuleInfoImpl : public GenericStructImpl<IModuleInfo, IStruct>
{
public:
    ModuleInfoImpl(const VersionInfoPtr& versionInfo, const StringPtr& name, const StringPtr& id);

    ErrCode INTERFACE_FUNC getVersionInfo(IVersionInfo** versionInfo) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getId(IString** id) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized,
                               IBaseObject* context,
                               IFunction* factoryCallback,
                               IBaseObject** obj);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ModuleInfoImpl)

END_NAMESPACE_OPENDAQ
