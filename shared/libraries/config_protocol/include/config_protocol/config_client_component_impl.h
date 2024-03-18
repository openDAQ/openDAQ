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
#include <config_protocol/config_client_property_object_impl.h>
#include <opendaq/component_impl.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <config_protocol/config_protocol_deserialize_context.h>

namespace daq::config_protocol
{

template <typename Impl>
class ConfigClientComponentBaseImpl;

using ConfigClientComponentImpl = ConfigClientComponentBaseImpl<ComponentImpl<IComponent, IConfigClientObject>>;

template <class Impl>
class ConfigClientComponentBaseImpl : public ConfigClientPropertyObjectBaseImpl<Impl>
{
public:
    template <class ... Args>
    ConfigClientComponentBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                  const std::string& remoteGlobalId,
                                  const Args& ... args);

    // Component overrides
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getTags(ITags** tags) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
protected:
    template <class Interface, class Implementation>
    static BaseObjectPtr DeserializeConfigComponent(const SerializedObjectPtr& serialized,
                                                    const BaseObjectPtr& context,
                                                    const FunctionPtr& factoryCallback);

    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;

private:
    void componentUpdateEnd(const CoreEventArgsPtr& args);
    void attributeChanged(const CoreEventArgsPtr& args);
    void tagsChanged(const CoreEventArgsPtr& args);
    void statusChanged(const CoreEventArgsPtr& args);
};

template <class Impl>
template <class ... Args>
ConfigClientComponentBaseImpl<Impl>::ConfigClientComponentBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                   const std::string& remoteGlobalId,
                                                                   const Args&... args)
    : ConfigClientPropertyObjectBaseImpl<Impl>(configProtocolClientComm, remoteGlobalId, args ...)
{
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getActive(Bool* active)
{
    return Impl::getActive(active);
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::setActive(Bool active)
{
    if (this->coreEventMuted)
        return Impl::setActive(active);

    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getTags(ITags** tags)
{
    return Impl::getTags(tags);
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getName(IString** name)
{
    return Impl::getName(name);
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::setName(IString* name)
{
    return daqTry([this, &name] { this->clientComm->setAttributeValue(this->remoteGlobalId, "Name", name); });
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getDescription(IString** description)
{
    return Impl::getDescription(description);
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::setDescription(IString* description)
{
    return daqTry([this, &description] { this->clientComm->setAttributeValue(this->remoteGlobalId, "Description", description); });
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigComponent<IComponent, ConfigClientComponentImpl>(serialized, context, factoryCallback).detach();
        });
}

template <class Impl>
template <class Interface, class Implementation>
BaseObjectPtr ConfigClientComponentBaseImpl<Impl>::DeserializeConfigComponent(const SerializedObjectPtr& serialized,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    return Impl::DeserializeComponent(
        serialized,
        context,
        factoryCallback,
        [](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
        {
            const auto ctx = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();

            return createWithImplementation<Interface, Implementation>(
                ctx->getClientComm(),
                ctx->getRemoteGlobalId(),
                deserializeContext.getContext(),
                deserializeContext.getParent(),
                deserializeContext.getLocalId(),
                className);
        });
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::ComponentUpdateEnd:
            componentUpdateEnd(args);
            break;
        case CoreEventId::AttributeChanged:
            attributeChanged(args);
            break;
        case CoreEventId::TagsChanged:
            tagsChanged(args);
            break;
        case CoreEventId::StatusChanged:
            statusChanged(args);
            break;
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::SignalConnected:
        case CoreEventId::SignalDisconnected:
        case CoreEventId::DataDescriptorChanged:
        case CoreEventId::ComponentAdded:
        case CoreEventId::ComponentRemoved:
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        case CoreEventId::DeviceDomainChanged:
        default:
            break;
    }

    ConfigClientPropertyObjectBaseImpl<Impl>::handleRemoteCoreObjectInternal(sender, args);
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientPropertyObjectBaseImpl<Impl>::onRemoteUpdate(serialized);

    if (serialized.hasKey("active"))
        this->active = serialized.readBool("active");

    if (serialized.hasKey("visible"))
        this->visible = serialized.readBool("visible");

    if (serialized.hasKey("description"))
        this->description = serialized.readString("description");

    if (serialized.hasKey("name"))
       this->name = serialized.readString("name");
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::componentUpdateEnd(const CoreEventArgsPtr& args)
{

    const StringPtr str = args.getParameters().get("SerializedComponent");

    const bool muted = this->coreEventMuted;
    const auto thisPtr = this->template borrowPtr<ComponentPtr>();
    const auto propInternalPtr = this->template borrowPtr<PropertyObjectInternalPtr>();
    if (!muted)
        propInternalPtr.disableCoreEventTrigger();
    
    this->deserializationComplete = false;

    const auto deserializer = JsonDeserializer();
    deserializer.callCustomProc([&](const SerializedObjectPtr& serialized) { onRemoteUpdate(serialized); }, str);
    this->clientComm->connectInputPorts(thisPtr);
    this->clientComm->connectDomainSignals(thisPtr);

    this->deserializationComplete = true;

    if (!muted && this->coreEvent.assigned())
    {
        const CoreEventArgsPtr argsNew = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(CoreEventId::ComponentUpdateEnd, Dict<IString, IBaseObject>());
        this->triggerCoreEvent(argsNew);
        propInternalPtr.enableCoreEventTrigger();
    }
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::attributeChanged(const CoreEventArgsPtr& args)
{
    const std::string attrName = args.getParameters().get("AttributeName");
    const bool relock = this->lockedAttributes.erase(attrName);

    if (attrName == "Active")
    {
        const Bool active = args.getParameters().get("Active");
        checkErrorInfo(Impl::setActive(active));
    }
    else if (attrName == "Name")
    {
        const StringPtr name = args.getParameters().get("Name");
        checkErrorInfo(Impl::setName(name));
    }
    else if (attrName == "Description")
    {
        const StringPtr description = args.getParameters().get("Description");
        checkErrorInfo(Impl::setDescription(description));
    }
    else if (attrName == "Visible")
    {
        const Bool visible = args.getParameters().get("Visible");
        checkErrorInfo(Impl::setVisible(visible));
    }

    if (relock)
        this->lockedAttributes.insert(attrName);
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::tagsChanged(const CoreEventArgsPtr& args)
{
    TagsPtr tags;
    checkErrorInfo(Impl::getTags(&tags));
    const TagsPtr newTags = args.getParameters().get("Tags");
    tags.asPtr<ITagsPrivate>().replace(newTags.getList());
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::statusChanged(const CoreEventArgsPtr& args)
{
    ComponentStatusContainerPtr statusContainer;
    checkErrorInfo(Impl::getStatusContainer(&statusContainer));
    DictPtr<IString, IEnumeration> changedStatuses = args.getParameters();
    for (const auto& st : changedStatuses)
        statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatus(st.first, st.second);
}
}
