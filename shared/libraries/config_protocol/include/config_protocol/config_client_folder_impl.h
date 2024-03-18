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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/folder_impl.h>
#include <opendaq/component_holder_ptr.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

template <class Impl>
class ConfigClientBaseFolderImpl;

using ConfigClientFolderImpl = ConfigClientBaseFolderImpl<FolderImpl<IFolderConfig, IConfigClientObject>>;

template <class Impl>
class ConfigClientBaseFolderImpl : public ConfigClientComponentBaseImpl<Impl>
{
public:
    ConfigClientBaseFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                               const std::string& remoteGlobalId,
                               const IntfID& intfID,
                               const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const StringPtr& className = nullptr);

    ConfigClientBaseFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                               const std::string& remoteGlobalId,
                               const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const StringPtr& className = nullptr);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    template <class Interface, class Implementation>
    static BaseObjectPtr DeserializeConfigFolder(const SerializedObjectPtr& serialized,
                                                 const BaseObjectPtr& context,
                                                 const FunctionPtr& factoryCallback);

    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;

private:
    void componentAdded(const CoreEventArgsPtr& args);
    void componentRemoved(const CoreEventArgsPtr& args);
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override; 
};

template <class Impl>
ConfigClientBaseFolderImpl<Impl>::ConfigClientBaseFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                             const std::string& remoteGlobalId,
                                                             const IntfID& intfID,
                                                             const ContextPtr& ctx,
                                                             const ComponentPtr& parent,
                                                             const StringPtr& localId,
                                                             const StringPtr& className)

    : ConfigClientComponentBaseImpl<Impl>(configProtocolClientComm, remoteGlobalId, intfID, ctx, parent, localId, className)
{
}

template <class Impl>
ConfigClientBaseFolderImpl<Impl>::ConfigClientBaseFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                             const std::string& remoteGlobalId,
                                                             const ContextPtr& ctx,
                                                             const ComponentPtr& parent,
                                                             const StringPtr& localId,
                                                             const StringPtr& className)

    : ConfigClientComponentBaseImpl<Impl>(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
{
}

template <class Impl>
ErrCode ConfigClientBaseFolderImpl<Impl>::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigFolder<IFolder, ConfigClientFolderImpl>(serialized, context, factoryCallback).detach();
        });
}

template <class Impl>
template <class Interface, class Implementation>
BaseObjectPtr ConfigClientBaseFolderImpl<Impl>::DeserializeConfigFolder(
    const SerializedObjectPtr& serialized,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    return Impl::DeserializeComponent(
        serialized,
        context,
        factoryCallback,
        [](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
        {
            IntfID intfID;
            const auto ctx = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();
            const auto errCode = ctx->getIntfID(&intfID);
            if (errCode == OPENDAQ_SUCCESS)
            {
                return createWithImplementation<Interface, Implementation>(ctx->getClientComm(),
                                                                           ctx->getRemoteGlobalId(),
                                                                           intfID,
                                                                           deserializeContext.getContext(),
                                                                           deserializeContext.getParent(),
                                                                           deserializeContext.getLocalId(),
                                                                           className);
            }
            if (errCode == OPENDAQ_NOTFOUND)
            {
                return createWithImplementation<Interface, Implementation>(ctx->getClientComm(),
                                                                           ctx->getRemoteGlobalId(),
                                                                           deserializeContext.getContext(),
                                                                           deserializeContext.getParent(),
                                                                           deserializeContext.getLocalId(),
                                                                           className);
            }
            checkErrorInfo(errCode);
            return typename InterfaceToSmartPtr<Interface>::SmartPtr();
        });
}

template <class Impl>
void ConfigClientBaseFolderImpl<Impl>::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(args);
            break;
        case CoreEventId::ComponentRemoved:
            componentRemoved(args);
            break;
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::SignalConnected:
        case CoreEventId::SignalDisconnected:
        case CoreEventId::DataDescriptorChanged:
        case CoreEventId::ComponentUpdateEnd:
        case CoreEventId::AttributeChanged:
        case CoreEventId::TagsChanged:
        case CoreEventId::StatusChanged:
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        case CoreEventId::DeviceDomainChanged:
        default:
            break;
    }

    ConfigClientComponentBaseImpl<Impl>::handleRemoteCoreObjectInternal(sender, args);
}

template <class Impl>
void ConfigClientBaseFolderImpl<Impl>::componentAdded(const CoreEventArgsPtr& args)
{
    const ComponentPtr comp = args.getParameters().get("Component");
    Bool hasItem{false};
    checkErrorInfo(Impl::hasItem(comp.getLocalId(), &hasItem));
    if (!hasItem)
    {
        this->clientComm->connectDomainSignals(comp);
        checkErrorInfo(Impl::addItem(comp));
    }
}

template <class Impl>
void ConfigClientBaseFolderImpl<Impl>::componentRemoved(const CoreEventArgsPtr& args)
{
    const StringPtr id = args.getParameters().get("Id");
    Bool hasItem{false};
    checkErrorInfo(Impl::hasItem(id, &hasItem));
    if (hasItem)
        checkErrorInfo(Impl::removeItemWithLocalId(id));
}

template <class Impl>
void ConfigClientBaseFolderImpl<Impl>::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientComponentBaseImpl<Impl>::onRemoteUpdate(serialized);

    const auto keyStr = String("items");
    const auto hasKey = serialized.hasKey(keyStr);

    if (!IsTrue(hasKey))
    {
        ListPtr<IComponent> itemsList = List<IComponent>();
        this->getItems(&itemsList, search::Any());
        for (const auto& item : itemsList)
            this->removeItem(item);

        return;
    }
    
    const auto serItems = serialized.readSerializedObject(keyStr);
    const auto keys = serItems.getKeys();

    for (const auto& key : keys)
    {
        Bool hasItem;
        this->hasItem(key, &hasItem);
        const auto obj = serItems.readSerializedObject(key);
        if (hasItem)
        {
            ComponentPtr child;
            this->getItem(key, &child);
            child.asPtr<IConfigClientObject>()->remoteUpdate(obj);
        }
        else
        {
            if (!obj.hasKey("__type"))
                continue;

            const StringPtr type = obj.readString("__type");
            const auto thisPtr = this->template borrowPtr<ComponentPtr>();
            const auto deserializeContext = createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
                this->clientComm, this->remoteGlobalId, this->context, nullptr, thisPtr, key, nullptr);

            const ComponentPtr deserializedObj = this->clientComm->deserializeConfigComponent(
                type,
                obj,
                deserializeContext,
                [&](const StringPtr& typeId,
                    const SerializedObjectPtr& object,
                    const BaseObjectPtr& context,
                    const FunctionPtr& factoryCallback)
                {
                    return this->clientComm->deserializeConfigComponent(typeId, object, context, factoryCallback, nullptr);
                },
                nullptr);

            if (deserializedObj.assigned())
                this->addItem(deserializedObj);
        }
    }

    ListPtr<IComponent> itemsList = List<IComponent>();
    this->getItems(&itemsList, search::Any());
    for (const auto& item : itemsList)
    {
        if (!serItems.hasKey(item.getName()))
            this->removeItem(item);
    }
}
}
