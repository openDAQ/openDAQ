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
#include <opendaq/component_update_context_ptr.h>
#include <opendaq/component_ptr.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/update_parameters_factory.h>
#include <opendaq/device_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentUpdateContextImpl : public ImplementationOf<IComponentUpdateContext, ISerializable>
{
public:

    ComponentUpdateContextImpl()
    {
    }

    ComponentUpdateContextImpl(const ComponentPtr& curComponent, const UpdateParametersPtr& config)
        : config(config.assigned() ? config : UpdateParameters())
        , deviceUpdateOptionsMapping(populateDeviceUpdateOptionsMapping(config))
        , deviceMapping(Dict<IString, IString>())
        , connections(Dict<IString, IBaseObject>())
        , signalDependencies(Dict<IString, IString>())
        , parentDependencies(List<IString>())
        , rootComponent(GetRootComponent(curComponent))
    {
    }

    ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) override;
    ErrCode INTERFACE_FUNC getInputPortConnections(IString* parentId, IDict** connections) override;
    ErrCode INTERFACE_FUNC removeInputPortConnection(IString* parentId) override;
    ErrCode INTERFACE_FUNC setRootComponent(IComponent* baseComponent) override;
    ErrCode INTERFACE_FUNC getRootComponent(IComponent** rootComponent) override;
    ErrCode INTERFACE_FUNC getSignal(IString* parentId, IString* portId, ISignal** signal) override;
    ErrCode INTERFACE_FUNC setSignalDependency(IString* signalId, IString* parentId) override;
    ErrCode INTERFACE_FUNC addDeviceRemapping(IString* originalDeviceId, IString* newDeviceId) override;
    ErrCode INTERFACE_FUNC remapInputPortConnections() override;
    ErrCode INTERFACE_FUNC getDeviceUpdateOptionsWithLocalIdOrNull(IString* localId, IDeviceUpdateOptions** options) override;
    ErrCode INTERFACE_FUNC getUpdateParameters(IUpdateParameters** updateParameters) override;

    ErrCode INTERFACE_FUNC resolveSignalDependency(IString* signalId, ISignal** signal);
    ErrCode INTERFACE_FUNC overrideState(IComponentUpdateContext* updateContext) override;
    ErrCode INTERFACE_FUNC getInternalState(IDict** state) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* /*factoryCallback*/, IBaseObject** obj);

    StringPtr remapDeviceLocalIds(const std::string& componentGlobalId) const;
    static ComponentPtr GetRootComponent(const ComponentPtr& curComponent);
    static DevicePtr GetDevice(const StringPtr& id, const DevicePtr& parentDevice);
    static std::string GetRootDeviceId(const std::string& id);
    static DictPtr<IString, IDeviceUpdateOptions> populateDeviceUpdateOptionsMapping(const UpdateParametersPtr& config);

    UpdateParametersPtr config;
    DictPtr<IString, IDeviceUpdateOptions> deviceUpdateOptionsMapping;

    DictPtr<IString, IString> deviceMapping;
    DictPtr<IString, IDict> connections;
    DictPtr<IString, IString> signalDependencies;
    ListPtr<IString> parentDependencies;

    ComponentPtr rootComponent;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ComponentUpdateContextImpl)

END_NAMESPACE_OPENDAQ
