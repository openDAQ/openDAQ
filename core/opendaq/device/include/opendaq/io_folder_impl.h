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
#include <opendaq/folder_impl.h>
#include <opendaq/io_folder_config.h>
#include <opendaq/channel.h>

BEGIN_NAMESPACE_OPENDAQ

class IoFolderImpl : public FolderImpl<IIoFolderConfig>
{
public:
    using Super = FolderImpl<IIoFolderConfig>;

    IoFolderImpl(const ContextPtr& context,
                 const ComponentPtr& parent,
                 const StringPtr& localId,
                 const StringPtr& className = nullptr,
                 ComponentStandardProps propsMode = ComponentStandardProps::Add)
        : Super(context, parent, localId, className, propsMode)
    {
    }

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override
    {
        OPENDAQ_PARAM_NOT_NULL(id);

        *id = SerializeId();

        return OPENDAQ_SUCCESS;
    }

    static ConstCharPtr SerializeId()
    {
        return "IoFolder";
    }

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj)
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }

protected:
    bool addItemInternal(const ComponentPtr& component) override
    {
        if (!component.supportsInterface<IIoFolderConfig>() && !component.supportsInterface<IChannel>())
            throw InvalidParameterException("Type of item not allowed in the folder");

        return Super::addItemInternal(component);
    }
};

END_NAMESPACE_OPENDAQ
