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
#include <coretypes/deserializer.h>
#include <coretypes/serialized_object.h>
#include <coretypes/serializer.h>
#include <coretypes/version_info.h>

BEGIN_NAMESPACE_OPENDAQ

class VersionInfoImpl : public GenericStructImpl<IVersionInfo, IStruct>
{
public:
    explicit VersionInfoImpl(SizeT major, SizeT minor, SizeT patch);

    ErrCode INTERFACE_FUNC getMajor(SizeT* major) override;
    ErrCode INTERFACE_FUNC getMinor(SizeT* minor) override;
    ErrCode INTERFACE_FUNC getPatch(SizeT* patch) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(VersionInfoImpl)

END_NAMESPACE_OPENDAQ
