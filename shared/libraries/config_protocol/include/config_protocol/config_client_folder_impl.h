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

class ConfigClientFolderImpl : public ConfigClientComponentBaseImpl<FolderImpl<>>
{
public:
    template <class... Args>
    ConfigClientFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                           const ContextPtr& ctx,
                           const ComponentPtr& parent,
                           const StringPtr& localId,
                           const Args&... args);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

template <class... Args>
ConfigClientFolderImpl::ConfigClientFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                      const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const Args&... args)
    : ConfigClientComponentBaseImpl<FolderImpl<>>(configProtocolClientComm, ctx, parent, localId, args...)
{
}

inline ErrCode ConfigClientFolderImpl::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr& serialized,
                          const ComponentDeserializeContextPtr& deserializeContext,
                          const StringPtr& className)
                       {
                            const auto configDeserializeContext = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();

                            return createWithImplementation<IFolder, ConfigClientFolderImpl>(
                                configDeserializeContext->getClientComm(),
                                deserializeContext.getContext(),
                                deserializeContext.getParent(),
                                deserializeContext.getLocalId());
                       })
                       .detach();
        });
}

}
