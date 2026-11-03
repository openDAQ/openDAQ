/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/server_type.h>
#include <opendaq/component_type_impl.h>
#include <opendaq/component_type_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ServerTypeImpl : public GenericComponentTypeImpl<IServerType>
{
public:
    using Self = ServerTypeImpl;
    using Super = GenericComponentTypeImpl<IServerType>;

    explicit ServerTypeImpl(const StringPtr& id,
                            const StringPtr& name,
                            const StringPtr& description,
                            const PropertyObjectPtr& defaultConfig,
                            const DictPtr<IString, IInteger>& transports = Dict<IString, IInteger>());

    explicit ServerTypeImpl(const ComponentTypeBuilderPtr& builder);

    // IComponentType
    ErrCode INTERFACE_FUNC createAuthenticationConfig(IAuthenticationConfig** config) override;

    // IComponentTypePrivate
    ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** authenticationMethods) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* serializedId) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

END_NAMESPACE_OPENDAQ
