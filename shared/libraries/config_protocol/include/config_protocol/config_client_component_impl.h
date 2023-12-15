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

namespace daq::config_protocol
{

template <typename Impl>
class ConfigClientComponentBaseImpl;

using ConfigClientComponentImpl = ConfigClientComponentBaseImpl<ComponentImpl<>>;

template <class Impl>
class ConfigClientComponentBaseImpl : public ConfigClientPropertyObjectBaseImpl<Impl>
{
public:
    template <class ... Args>
    ConfigClientComponentBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                  const SerializedObjectPtr& serializedObject,
                                  const ContextPtr& ctx,
                                  const ComponentPtr& parent,
                                  const StringPtr& localId,
                                  const Args& ... args);

    // Component overrides
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getTags(ITagsConfig** tags) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;

private:
    void buildComponent(const SerializedObjectPtr& serializedObject);
};

template <class Impl>
template <class ... Args>
ConfigClientComponentBaseImpl<Impl>::ConfigClientComponentBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
    const SerializedObjectPtr& serializedObject,
    const ContextPtr& ctx,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const Args&... args)
    : ConfigClientPropertyObjectBaseImpl<Impl>(configProtocolClientComm, serializedObject, ctx, parent, localId, args ...)
{
    buildComponent(serializedObject);
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getActive(Bool* active)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::setActive(Bool active)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getTags(ITagsConfig** tags)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getName(IString** name)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::setName(IString* name)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::getDescription(IString** description)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientComponentBaseImpl<Impl>::setDescription(IString* description)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
void ConfigClientComponentBaseImpl<Impl>::buildComponent(const SerializedObjectPtr& serializedObject)
{
    // TODO
}

}
