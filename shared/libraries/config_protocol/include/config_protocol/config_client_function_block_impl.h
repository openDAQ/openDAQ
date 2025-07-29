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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/function_block_impl.h>

namespace daq::config_protocol
{

template <class Impl>
class ConfigClientBaseFunctionBlockImpl;

using ConfigClientFunctionBlockImpl = ConfigClientBaseFunctionBlockImpl<FunctionBlockImpl<IFunctionBlock, IConfigClientObject>>;

template <class Impl>
class ConfigClientBaseFunctionBlockImpl : public ConfigClientComponentBaseImpl<Impl>
{
public:
    using Super = ConfigClientComponentBaseImpl<Impl>;

    ConfigClientBaseFunctionBlockImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                      const std::string& remoteGlobalId,
                                      const FunctionBlockTypePtr& functionBlockType,
                                      const ContextPtr& ctx,
                                      const ComponentPtr& parent,
                                      const StringPtr& localId,
                                      const StringPtr& className = nullptr);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;

protected:
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;
    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;
};

class ConfigClientRecorderFunctionBlockImpl : public ConfigClientBaseFunctionBlockImpl<FunctionBlockImpl<IFunctionBlock, IRecorder, IConfigClientObject>>
{
public:
    using Super = ConfigClientBaseFunctionBlockImpl<FunctionBlockImpl<IFunctionBlock, IRecorder, IConfigClientObject>>;

    ConfigClientRecorderFunctionBlockImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                          const std::string& remoteGlobalId,
                                          const FunctionBlockTypePtr& functionBlockType,
                                          const ContextPtr& ctx,
                                          const ComponentPtr& parent,
                                          const StringPtr& localId,
                                          const StringPtr& className = nullptr);

    // IRecorder
    ErrCode INTERFACE_FUNC startRecording() override;
    ErrCode INTERFACE_FUNC stopRecording() override;
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;
};

template <class Impl>
ConfigClientBaseFunctionBlockImpl<Impl>::ConfigClientBaseFunctionBlockImpl(
    const ConfigProtocolClientCommPtr& configProtocolClientComm,
    const std::string& remoteGlobalId,
    const FunctionBlockTypePtr& type,
    const ContextPtr& ctx,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, type, ctx, parent, localId, className)
{
}

template <class Impl>
ErrCode ConfigClientBaseFunctionBlockImpl<Impl>::Deserialize(ISerializedObject* serialized,
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

                            const auto typeId = serialized.readString("typeId");

                            const auto fbType = FunctionBlockType(typeId, typeId, "", nullptr);

                            bool isRecorder = false;
                            if (serialized.hasKey("isRecorder"))
                                isRecorder = serialized.readBool("isRecorder");

                            if (isRecorder)
                                return createWithImplementation<IFunctionBlock, ConfigClientRecorderFunctionBlockImpl>(
                                    configDeserializeContext->getClientComm(),
                                    configDeserializeContext->getRemoteGlobalId(),
                                    fbType,
                                    deserializeContext.getContext(),
                                    deserializeContext.getParent(),
                                    deserializeContext.getLocalId(),
                                    className);
                            else
                                return createWithImplementation<IFunctionBlock, ConfigClientFunctionBlockImpl>(
                                    configDeserializeContext->getClientComm(),
                                    configDeserializeContext->getRemoteGlobalId(),
                                    fbType,
                                    deserializeContext.getContext(),
                                    deserializeContext.getParent(),
                                    deserializeContext.getLocalId(),
                                    className);
                       })
                       .detach();
        });
}

template <class Impl>
DictPtr<IString, IFunctionBlockType> ConfigClientBaseFunctionBlockImpl<Impl>::onGetAvailableFunctionBlockTypes()
{
    return this->clientComm->getAvailableFunctionBlockTypes(this->remoteGlobalId, true);
}

template <class Impl>
FunctionBlockPtr ConfigClientBaseFunctionBlockImpl<Impl>::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    const ComponentHolderPtr fbHolder = this->clientComm->addFunctionBlock(this->remoteGlobalId, typeId, config, this->functionBlocks, true);

    FunctionBlockPtr fb = fbHolder.getComponent();
    if (!this->functionBlocks.hasItem(fb.getLocalId()))
    {
        this->clientComm->connectDomainSignals(fb);
        this->addNestedFunctionBlock(fb);
        this->clientComm->connectInputPorts(fb);
        return fb;
    }

    return this->functionBlocks.getItem(fb.getLocalId());
}

template <class Impl>
void ConfigClientBaseFunctionBlockImpl<Impl>::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    if (!functionBlock.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException);
    this->clientComm->removeFunctionBlock(this->remoteGlobalId, functionBlock.getLocalId(), true);

    if (this->functionBlocks.hasItem(functionBlock.getLocalId()))
    {
        this->removeNestedFunctionBlock(functionBlock);
    }
}

template <class Impl>
void ConfigClientBaseFunctionBlockImpl<Impl>::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientComponentBaseImpl<Impl>::onRemoteUpdate(serialized);

    for (const auto& comp : this->components)
    {
        const auto id = comp.getLocalId();
        if (serialized.hasKey(id))
        {
            const auto serObj = serialized.readSerializedObject(id);
            comp.template asPtr<IConfigClientObject>()->remoteUpdate(serObj);
        }
    }
}

template <class Impl>
void ConfigClientBaseFunctionBlockImpl<Impl>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                            const BaseObjectPtr& context,
                                                                            const FunctionPtr& factoryCallback)
{
    Impl::deserializeCustomObjectValues(serializedObject, context, factoryCallback);
    if (serializedObject.hasKey("ComponentConfig"))
        this->componentConfig = serializedObject.readObject("ComponentConfig");
}

inline ConfigClientRecorderFunctionBlockImpl::ConfigClientRecorderFunctionBlockImpl(
    const ConfigProtocolClientCommPtr& configProtocolClientComm,
    const std::string& remoteGlobalId,
    const FunctionBlockTypePtr& type,
    const ContextPtr& ctx,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, type, ctx, parent, localId, className)
{
}

inline ErrCode ConfigClientRecorderFunctionBlockImpl::startRecording()
{
    return daqTry([this] { this->clientComm->startRecording(this->remoteGlobalId); });
}

inline ErrCode ConfigClientRecorderFunctionBlockImpl::stopRecording()
{
    return daqTry([this] { this->clientComm->stopRecording(this->remoteGlobalId); });
}

inline ErrCode ConfigClientRecorderFunctionBlockImpl::getIsRecording(Bool* isRecording)
{
    return daqTry(
        [this, &isRecording]
        {
            BooleanPtr isRecordingPtr = clientComm->getIsRecording(remoteGlobalId);
            *isRecording = isRecordingPtr.getValue(False);
            return OPENDAQ_SUCCESS;
        });
}

}
