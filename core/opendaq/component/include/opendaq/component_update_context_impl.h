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

inline ComponentPtr ComponentUpdateContextImpl::GetRootComponent(const ComponentPtr& curComponent)
{
    const auto parent = curComponent.getParent();
    if (parent.assigned())
        return GetRootComponent(parent);
    return curComponent;
}

inline DevicePtr ComponentUpdateContextImpl::GetDevice(const StringPtr& id, const DevicePtr& parentDevice)
{
    if (parentDevice.getLocalId() == id)
        return parentDevice;
    
    for (const auto& device: parentDevice.getDevices())
    {
        const auto devicePtr = GetDevice(id, device);
        if (devicePtr.assigned())
            return devicePtr;
    }
    return nullptr;
}

inline std::string ComponentUpdateContextImpl::GetRootDeviceId(const std::string& id)
{
    if (id.empty())
        return id;
    
    auto idx = id.find('/', 1);
    if (idx == std::string::npos)
        return id;

    return id.substr(1, idx - 1);
}

static void recurseDeviceUpdateOptions(DictPtr<IString, IDeviceUpdateOptions>& mapping, const DeviceUpdateOptionsPtr& options)
{
    if (!options.assigned())
        return;

    auto localId = options.getLocalId();
    if (localId == "")
        mapping.set("Root", options);
    else
        mapping.set(localId, options);

    for (const auto& childOptions : options.getChildDeviceOptions())
        recurseDeviceUpdateOptions(mapping, childOptions);
}

inline DictPtr<IString, IDeviceUpdateOptions> ComponentUpdateContextImpl::populateDeviceUpdateOptionsMapping(const UpdateParametersPtr& config)
{
    DictPtr<IString, IDeviceUpdateOptions> mapping = Dict<IString, IDeviceUpdateOptions>();
    if (config.assigned())
        recurseDeviceUpdateOptions(mapping, config.getDeviceUpdateOptions());

    return mapping;
}

inline ErrCode ComponentUpdateContextImpl::setInputPortConnection(IString* parentId, IString* portId, IString* signalId)
{
    OPENDAQ_PARAM_NOT_NULL(parentId);
    OPENDAQ_PARAM_NOT_NULL(portId);
    OPENDAQ_PARAM_NOT_NULL(signalId);

    DictPtr<IString, IString> ports;
    
    if (!connections.hasKey(parentId))
    {
        ports = Dict<IString, IString>();
        connections.set(parentId, ports);
    }
    else
    {
        ports = connections.get(parentId);
    }

    ports.set(portId, signalId);

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getInputPortConnections(IString* parentId, IDict** connections)
{
    OPENDAQ_PARAM_NOT_NULL(parentId);
    OPENDAQ_PARAM_NOT_NULL(connections);

    DictPtr<IString, IBaseObject> ports = this->connections.getOrDefault(parentId, Dict<IString, IBaseObject>());
    *connections = ports.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::removeInputPortConnection(IString* parentId)
{
    OPENDAQ_PARAM_NOT_NULL(parentId);

    return connections->deleteItem(parentId);
}

inline ErrCode ComponentUpdateContextImpl::setRootComponent(IComponent* baseComponent)
{
    OPENDAQ_PARAM_NOT_NULL(baseComponent);
    if (this->rootComponent.assigned())
        return OPENDAQ_ERR_ALREADYEXISTS;

    this->rootComponent = GetRootComponent(baseComponent);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getRootComponent(IComponent** rootComponent)
{
    OPENDAQ_PARAM_NOT_NULL(rootComponent);

    *rootComponent = this->rootComponent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getSignal(IString* parentId, IString* portId, ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(parentId);
    OPENDAQ_PARAM_NOT_NULL(portId);
    OPENDAQ_PARAM_NOT_NULL(signal);

    *signal = nullptr;

    DictPtr<IString, IString> signalConnections;
    getInputPortConnections(parentId, &signalConnections);
    if (!signalConnections.hasKey(portId))
        return OPENDAQ_NOTFOUND;
    
    auto signalId = signalConnections.get(portId);

    Bool isCircle = false;
    for (SizeT i = 0; i < parentDependencies.getCount(); i++)
    {
        const auto & parentDep = parentDependencies.getItemAt(i);
        if (parentDep == parentId)
        {
            std::ostringstream deps;
            for (SizeT j = i; j < parentDependencies.getCount(); j++)
            {
                deps << parentDependencies.getItemAt(j).toStdString() + " -> ";
            }
            auto loggerComponent = rootComponent.getContext().getLogger().getOrAddComponent("Component");
            LOG_W("Circular dependency detected: {}{}", deps.str(), parentDep);
            isCircle = true;
            break;
        }
    }

    if (isCircle == false)
    {
        parentDependencies.pushBack(parentId);
        ErrCode errCode = resolveSignalDependency(signalId, signal);
        parentDependencies.popBack();
        if (errCode == OPENDAQ_SUCCESS)
            return OPENDAQ_SUCCESS;
    }

    auto signalRootId = GetRootDeviceId(signalId);
    ComponentPtr signalRootComponent;
    if (rootComponent.supportsInterface<IDevice>())
        signalRootComponent = GetDevice(signalRootId, rootComponent);
    else if (rootComponent.getLocalId() == signalRootId)
        signalRootComponent = rootComponent;

    if (!signalRootComponent.assigned())
    {
        auto loggerComponent = rootComponent.getContext().getLogger().getOrAddComponent("Component");
        LOG_W("Root device for signal {} not found", signalId);
        return OPENDAQ_NOTFOUND;
    }

    ComponentPtr signalPtr;
    signalRootComponent->findComponent(signalId, &signalPtr);

    if (!signalPtr.assigned())
        return OPENDAQ_NOTFOUND;

    *signal = signalPtr.asPtrOrNull<ISignal>().detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::setSignalDependency(IString* signalId, IString* parentId)
{
    OPENDAQ_PARAM_NOT_NULL(signalId);
    OPENDAQ_PARAM_NOT_NULL(parentId);

    signalDependencies.set(signalId, parentId);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::addDeviceRemapping(IString* originalDeviceId, IString* newDeviceId)
{
    OPENDAQ_PARAM_NOT_NULL(originalDeviceId);
    OPENDAQ_PARAM_NOT_NULL(newDeviceId);

    deviceMapping.set(originalDeviceId, newDeviceId);
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::remapInputPortConnections()
{
    for (const auto& [parentId, ports] : connections)
    {
        DictPtr<IString, IString> newPorts = Dict<IString, IString>();
        for (const auto& [portId, signalId] : ports)
            newPorts.set(portId, remapDeviceLocalIds(signalId));

        connections.set(parentId, newPorts);
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getDeviceUpdateOptionsWithLocalIdOrNull(IString* localId, IDeviceUpdateOptions** options)
{
    OPENDAQ_PARAM_NOT_NULL(localId);
    OPENDAQ_PARAM_NOT_NULL(options);

    *options = deviceUpdateOptionsMapping.hasKey(localId) ? deviceUpdateOptionsMapping.get(localId).detach() : nullptr;
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getUpdateParameters(IUpdateParameters** updateParameters)
{
    OPENDAQ_PARAM_NOT_NULL(updateParameters);

    *updateParameters = config.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::resolveSignalDependency(IString* signalId, ISignal** signal)
{
    // Check that signal has parent
    if (!signalDependencies.hasKey(signalId))
        return OPENDAQ_NOTFOUND;
    
    auto parentId = signalDependencies.get(signalId);

    // Check that the parent is function block which is not finished with the update
    if (!connections.hasKey(parentId + "/IP"))
        return OPENDAQ_NOTFOUND;

    // Find the function block
    ComponentPtr parentComponent;
    rootComponent->findComponent(parentId, &parentComponent);

    if (!parentComponent.assigned())
        return OPENDAQ_NOTFOUND;

    // Call the parent component to finish the update
    parentComponent.as<IUpdatable>(true)->updateEnded(this->borrowInterface<IBaseObject>());

    // unregister dependency
    signalDependencies->deleteItem(signalId);
    
    auto signalIdPtr = StringPtr::Borrow(signalId);
    StringPtr signalLocalId = signalIdPtr.toStdString().substr(parentId.getLength());
    
    ComponentPtr signalComponent;
    parentComponent->findComponent(signalLocalId, &signalComponent);
    if (!signalComponent.assigned())
        return OPENDAQ_NOTFOUND;

    SignalPtr singalPtr = signalComponent.asPtrOrNull<ISignal>();
    if (!singalPtr.assigned())
        return OPENDAQ_NOTFOUND;

    *signal = singalPtr.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::overrideState(IComponentUpdateContext* updateContext)
{
    OPENDAQ_PARAM_NOT_NULL(updateContext);
    return daqTry([&updateContext, this]()
    {
        ComponentUpdateContextPtr updateContextPtr = ComponentUpdateContextPtr::Borrow(updateContext);
        auto state = updateContextPtr.getInternalState();
        deviceMapping = state.get("DeviceMapping");
        connections = state.get("Connections");
        signalDependencies = state.get("SignalDependencies");
        parentDependencies = state.get("ParentDependencies");
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ComponentUpdateContextImpl::getInternalState(IDict** state)
{
    OPENDAQ_PARAM_NOT_NULL(state);

    auto stateObj = Dict<IString, IBaseObject>();
    stateObj.set("DeviceMapping", deviceMapping);
    stateObj.set("Connections", connections);
    stateObj.set("SignalDependencies", signalDependencies);
    stateObj.set("ParentDependencies", parentDependencies);

    *state = stateObj.detach();
    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        if (deviceMapping.getCount())
        {
            serializer->key("DeviceMapping");
            OPENDAQ_RETURN_IF_FAILED(deviceMapping.asPtr<ISerializable>()->serialize(serializer));
        }

        if (connections.getCount())
        {
            serializer->key("Connections");
            OPENDAQ_RETURN_IF_FAILED(connections.asPtr<ISerializable>()->serialize(serializer));
        }

        if (signalDependencies.getCount())
        {
            serializer->key("SignalDependencies");
            OPENDAQ_RETURN_IF_FAILED(signalDependencies.asPtr<ISerializable>()->serialize(serializer));
        }

        if (parentDependencies.getCount())
        {
            serializer->key("ParentDependencies");
            OPENDAQ_RETURN_IF_FAILED(signalDependencies.asPtr<ISerializable>()->serialize(serializer));
        }

        if (config.assigned())
        {
            serializer->key("Config");
            OPENDAQ_RETURN_IF_FAILED(config.asPtr<ISerializable>()->serialize(serializer));
        }
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

inline ErrCode ComponentUpdateContextImpl::getSerializeId(ConstCharPtr* id) const
{    
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr ComponentUpdateContextImpl::SerializeId()
{
    return "ComponentUpdateContext";
}

inline ErrCode ComponentUpdateContextImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    auto deviceUpdateOptions = createWithImplementation<IComponentUpdateContext, ComponentUpdateContextImpl>();
    ComponentUpdateContextImpl* impl = dynamic_cast<ComponentUpdateContextImpl*>(deviceUpdateOptions.getObject());
    
    const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);
    if (serializedPtr.hasKey("DeviceMapping"))
        impl->deviceMapping = serializedPtr.readObject("DeviceMapping");
    else
        impl->deviceMapping = Dict<IString, IString>();

    if (serializedPtr.hasKey("Connections"))
        impl->connections = serializedPtr.readObject("Connections");
    else
        impl->connections = Dict<IString, IDict>();    
    
    if (serializedPtr.hasKey("SignalDependencies"))
        impl->signalDependencies = serializedPtr.readObject("SignalDependencies");
    else
        impl->signalDependencies = Dict<IString, IString>();

    if (serializedPtr.hasKey("ParentDependencies"))
        impl->parentDependencies = serializedPtr.readObject("ParentDependencies");
    else
        impl->parentDependencies = List<IString>();


    if (serializedPtr.hasKey("Config"))
    {
        impl->config = serializedPtr.readObject("Config");
        impl->deviceUpdateOptionsMapping = impl->populateDeviceUpdateOptionsMapping(impl->config);
    }
    else
        impl->config = UpdateParameters();

    *obj = deviceUpdateOptions.detach();
    return OPENDAQ_SUCCESS;
}

inline StringPtr ComponentUpdateContextImpl::remapDeviceLocalIds(const std::string& componentGlobalId) const
{
    std::string output;
    output.reserve(componentGlobalId.size());
    output = "/";

    size_t start = 1;
    while (start <= componentGlobalId.size())
    {
        const size_t end = componentGlobalId.find('/', start);
        auto seg = componentGlobalId.substr(start, end - start);
        
        if (deviceMapping.hasKey(seg))
            output += deviceMapping.get(seg).toStdString();
        else
            output += seg;

        if (end == std::string::npos)
            break;

        output += "/";
        start = end + 1;
    }
    
    return output;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ComponentUpdateContextImpl)

END_NAMESPACE_OPENDAQ
