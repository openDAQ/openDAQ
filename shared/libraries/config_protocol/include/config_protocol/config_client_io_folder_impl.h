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
#include <config_protocol/config_client_folder_impl.h>
#include <opendaq/io_folder_impl.h>

namespace daq::config_protocol
{

class ConfigClientIoFolderImpl : public ConfigClientBaseFolderImpl<IoFolderImpl<IConfigClientObject>>
{
public:
    using Super = ConfigClientBaseFolderImpl<IoFolderImpl<IConfigClientObject>>;

    ConfigClientIoFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                             const std::string& remoteGlobalId,
                             const IntfID& intfID,
                             const ContextPtr& ctx,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             const StringPtr& className = nullptr);

    ConfigClientIoFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                             const std::string& remoteGlobalId,
                             const ContextPtr& ctx,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             const StringPtr& className = nullptr);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

inline ConfigClientIoFolderImpl::ConfigClientIoFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                          const std::string& remoteGlobalId,
                                                          const IntfID& intfID,
                                                          const ContextPtr& ctx,
                                                          const ComponentPtr& parent,
                                                          const StringPtr& localId,
                                                          const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, intfID, ctx, parent, localId, className)
{
}

inline ConfigClientIoFolderImpl::ConfigClientIoFolderImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                          const std::string& remoteGlobalId,
                                                          const ContextPtr& ctx,
                                                          const ComponentPtr& parent,
                                                          const StringPtr& localId,
                                                          const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
{
}

inline ErrCode ConfigClientIoFolderImpl::Deserialize(ISerializedObject* serialized,
                                                     IBaseObject* context,
                                                     IFunction* factoryCallback,
                                                     IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigFolder<IIoFolderConfig, ConfigClientIoFolderImpl>(serialized, context, factoryCallback).detach();
        });
}

}
