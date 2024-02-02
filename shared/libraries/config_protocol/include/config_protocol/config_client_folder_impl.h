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

}
