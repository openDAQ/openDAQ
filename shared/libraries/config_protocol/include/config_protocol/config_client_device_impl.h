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
#include <opendaq/device_impl.h>
#include <config_protocol/config_client_component_impl.h>
#include <config_protocol/component_holder_ptr.h>

namespace daq::config_protocol
{

template <typename... Interfaces>
using ConfigClientDeviceBase = DeviceBase<IConfigClientObject, Interfaces...>;

using ConfigClientDevice = ConfigClientDeviceBase<>;

template <class TDeviceBase>
class GenericConfigClientDeviceImpl;

using ConfigClientDeviceImpl = GenericConfigClientDeviceImpl<ConfigClientDevice>;

template <class TDeviceBase>
class GenericConfigClientDeviceImpl : public ConfigClientComponentBaseImpl<TDeviceBase>
{
public:
    using Super = ConfigClientComponentBaseImpl<TDeviceBase>;

    explicit GenericConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                           const std::string& remoteGlobalId,
                                           const ContextPtr& ctx,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};

template <class TDeviceBase>
GenericConfigClientDeviceImpl<TDeviceBase>::GenericConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                          const std::string& remoteGlobalId,
                                                                          const ContextPtr& ctx,
                                                                          const ComponentPtr& parent,
                                                                          const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId)
{
}

template <class TDeviceBase>
DictPtr<IString, IFunctionBlockType> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableFunctionBlockTypes()
{
    return this->clientComm->sendComponentCommand(this->remoteGlobalId, "GetAvailableFunctionBlockTypes");
}

template <class TDeviceBase>
FunctionBlockPtr GenericConfigClientDeviceImpl<TDeviceBase>::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    auto params = Dict<IString, IBaseObject>({{"TypeId", typeId}, {"Config", config}});
    const ComponentHolderPtr fbHolder =
        this->clientComm->sendComponentCommand(this->remoteGlobalId, "AddFunctionBlock", params, this->functionBlocks);
    const FunctionBlockPtr fb = fbHolder.getComponent();
    this->addNestedFunctionBlock(fb);
    return fb;
}

template <class TDeviceBase>
ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::Deserialize(ISerializedObject* serialized,
                                                                IBaseObject* context,
                                                                IFunction* factoryCallback,
                                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::template DeserializeConfigComponent<IDevice, ConfigClientDeviceImpl>(serialized, context, factoryCallback).detach();
        });
}


}
