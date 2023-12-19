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

    // Component overrides
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getTags(ITagsConfig** tags) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

template <class... Args>
inline ConfigClientFolderImpl::ConfigClientFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                      const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const Args&... args)
    : ConfigClientComponentBaseImpl<FolderImpl<>>(configProtocolClientComm, ctx, parent, localId, args...)
{
}

inline ErrCode ConfigClientFolderImpl::getActive(Bool* active)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::setActive(Bool active)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::getTags(ITagsConfig** tags)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::getName(IString** name)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::setName(IString* name)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::getDescription(IString** description)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::setDescription(IString* description)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

inline ErrCode ConfigClientFolderImpl::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    return ComponentImpl::DeserializeComponent(
        serialized,
        context,
        factoryCallback,
        obj,
        [](const ComponentDeserializeContextPtr& context, const StringPtr& className)
        {
            const auto ctx = context.asPtr<IConfigProtocolDeserializeContext>();

            return createWithImplementation<IFolder, ConfigClientFolderImpl>(
                ctx->getClientComm(), context.getContext(), context.getParent(), context.getLocalId());
        });
}

}
