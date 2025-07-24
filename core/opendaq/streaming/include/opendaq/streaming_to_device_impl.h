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
#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/streaming.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/object_keys.h>
#include <opendaq/mirrored_input_port_config_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/streaming_to_device_private.h>
#include <opendaq/mirrored_input_port_private_ptr.h>
#include <opendaq/ids_parser.h>
#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming_to_device.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class StreamingToDeviceImpl;

using StreamingToDevice = StreamingToDeviceImpl<>;

template <typename... Interfaces>
class StreamingToDeviceImpl : public StreamingImpl<IStreamingToDevice, IStreamingToDevicePrivate, Interfaces...>
{
public:
    using Super = StreamingImpl<IStreamingToDevice, IStreamingToDevicePrivate, Interfaces...>;
    using Self = StreamingToDeviceImpl<Interfaces...>;

    explicit StreamingToDeviceImpl(const StringPtr& connectionString,
                                   const ContextPtr& context,
                                   bool skipDomainSignalSubscribe,
                                   const StringPtr& protocolId);

    ~StreamingToDeviceImpl() override;

    // IStreamingToDevice
    ErrCode INTERFACE_FUNC addInputPorts(IList* inputPorts) override;
    ErrCode INTERFACE_FUNC removeInputPorts(IList* inputPorts) override;
    ErrCode INTERFACE_FUNC removeAllInputPorts() override;

    ErrCode INTERFACE_FUNC getOwnerDevice(IMirroredDevice** device) const override;
    ErrCode INTERFACE_FUNC getProtocolId(IString** protocolId) const override;

    // IStreamingToDevicePrivate
    virtual ErrCode INTERFACE_FUNC registerStreamedSignals(IList* signals) override;
    virtual ErrCode INTERFACE_FUNC unregisterStreamedSignals(IList* signals) override;
    virtual ErrCode INTERFACE_FUNC detachRemovedInputPort(IString* inputPortRemoteId) override;

private:
    ErrCode removeStreamingSourceForAllInputPorts();
    void removeAllInputPortsInternal();

    StringPtr protocolId;

    using InputPortItem = WeakRefPtr<IMirroredInputPortConfig>;
    std::unordered_map<StringPtr, InputPortItem, StringHash, StringEqualTo> streamingInputPortsItems;
};

template<typename... Interfaces>
StreamingToDeviceImpl<Interfaces...>::StreamingToDeviceImpl(const StringPtr& connectionString,
                                                            const ContextPtr& context,
                                                            bool skipDomainSignalSubscribe,
                                                            const StringPtr& protocolId)
    : Super(connectionString, context, skipDomainSignalSubscribe)
    , protocolId(protocolId)
{
}

template<typename... Interfaces>
StreamingToDeviceImpl<Interfaces...>::~StreamingToDeviceImpl()
{
    try
    {
        ErrCode errCode = removeStreamingSourceForAllInputPorts();
        removeAllInputPortsInternal();
        checkErrorInfo(errCode);
    }
    catch (const DaqException& e)
    {
        DAQLOGF_W(this->loggerComponent, "Failed to remove signals on streaming object destruction: {}", e.what());
    }
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::addInputPorts(IList* inputPorts)
{
    OPENDAQ_PARAM_NOT_NULL(inputPorts);

    const auto inputPortsPtr = ListPtr<IMirroredInputPortConfig>::Borrow(inputPorts);
    for (const auto& mirroredInputPort : inputPortsPtr)
    {
        auto inputPortRemoteId = mirroredInputPort.getRemoteId();
        auto inputPortGlobalId = mirroredInputPort.getGlobalId();
        {
            std::scoped_lock lock(this->sync);

            StringPtr inputPortIdKey = inputPortRemoteId;

            auto it = streamingInputPortsItems.find(inputPortIdKey);
            if (it != streamingInputPortsItems.end())
            {
                return DAQ_MAKE_ERROR_INFO(
                    OPENDAQ_ERR_DUPLICATEITEM,
                    fmt::format(
                        R"(Input port with Ids (global /// remote /// key) "{}" /// "{}" /// "{}" failed to add - input port already added to streaming "{}")",
                        inputPortGlobalId,
                        inputPortRemoteId,
                        inputPortIdKey,
                        this->connectionString
                    )
                );
            }

            auto inputPortItem = WeakRefPtr<IMirroredInputPortConfig>(mirroredInputPort);
            streamingInputPortsItems.insert({inputPortIdKey, inputPortItem});
        }

        ErrCode errCode =
            daqTry([&]()
                   {
                       auto thisPtr = this->template borrowPtr<StreamingToDevicePtr>();
                       mirroredInputPort.template asPtr<IMirroredInputPortPrivate>().addStreamingSource(thisPtr);
                   });
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::removeInputPorts(IList* inputPorts)
{
    OPENDAQ_PARAM_NOT_NULL(inputPorts);

    const auto inputPortsPtr = ListPtr<IMirroredInputPortConfig>::Borrow(inputPorts);
    for (const auto& mirroredInputPortToRemove : inputPortsPtr)
    {
        ErrCode errCode =
            daqTry([&]()
                   {
                       mirroredInputPortToRemove.template asPtr<IMirroredInputPortPrivate>().removeStreamingSource(this->connectionString);
                   });
        OPENDAQ_RETURN_IF_FAILED(errCode);

        auto inputPortRemoteId = mirroredInputPortToRemove.getRemoteId();
        auto inputPortGlobalId = mirroredInputPortToRemove.getGlobalId();
        {
            std::scoped_lock lock(this->sync);

            StringPtr inputPortIdKey = mirroredInputPortToRemove.getRemoteId();

            auto it = streamingInputPortsItems.find(inputPortIdKey);
            if (it != streamingInputPortsItems.end())
            {
                auto mirroredInputPortlRef = it->second;
                if (auto mirroredInputPort = mirroredInputPortlRef.getRef(); mirroredInputPort.assigned())
                {
                    streamingInputPortsItems.erase(it);
                }
            }
            else
            {
                return DAQ_MAKE_ERROR_INFO(
                    OPENDAQ_ERR_NOTFOUND,
                    fmt::format(
                        R"(Input port with Ids (global /// remote /// key) "{}" /// "{}" /// "{}" failed to remove - input port not found in streaming "{}" )",
                        inputPortGlobalId,
                        inputPortRemoteId,
                        inputPortIdKey,
                        this->connectionString
                    )
                );
            }
        }
    }

    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::removeAllInputPorts()
{
    ErrCode errCode = removeStreamingSourceForAllInputPorts();
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::scoped_lock lock(this->sync);

    removeAllInputPortsInternal();

    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::getOwnerDevice(IMirroredDevice** device) const
{
    OPENDAQ_PARAM_NOT_NULL(device);

    *device = this->ownerDeviceRef.getRef().template asPtr<IMirroredDevice>();
    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::getProtocolId(IString** protocolId) const
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);

    *protocolId = this->protocolId;
    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::registerStreamedSignals(IList* signals)
{
    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::unregisterStreamedSignals(IList* signals)
{
    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::detachRemovedInputPort(IString* inputPortRemoteId)
{
    std::scoped_lock lock(this->sync);

    StringPtr inputPortIdKey = inputPortRemoteId;
    if (auto it = streamingInputPortsItems.find(inputPortIdKey); it != streamingInputPortsItems.end())
    {
        streamingInputPortsItems.erase(it);
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Input port "{}" failed to remove - input port not found in streaming "{}" )",
                inputPortIdKey,
                this->connectionString
            )
        );
    }

    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
ErrCode StreamingToDeviceImpl<Interfaces...>::removeStreamingSourceForAllInputPorts()
{
    auto allInputPorts = List<IMirroredInputPortConfig>();

    {
        std::scoped_lock lock(this->sync);

        for (const auto& [_, inputPortItem] : streamingInputPortsItems)
        {
            if (auto mirroredInputPort = inputPortItem.getRef(); mirroredInputPort.assigned())
                allInputPorts.pushBack(mirroredInputPort);
        }
    }

    for (const auto& inputPort : allInputPorts)
    {
        ErrCode errCode =
            daqTry([&]()
                   {
                       inputPort.template asPtr<IMirroredInputPortPrivate>().removeStreamingSource(this->connectionString);
                   });
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    return OPENDAQ_SUCCESS;
}

template<typename... Interfaces>
void StreamingToDeviceImpl<Interfaces...>::removeAllInputPortsInternal()
{
    streamingInputPortsItems.clear();
}

END_NAMESPACE_OPENDAQ
