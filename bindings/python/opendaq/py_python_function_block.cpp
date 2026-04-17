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

#include <pybind11/pybind11.h>
#include <pybind11/gil.h>

#include <opendaq/function_block_impl.h>

#include "py_opendaq/py_python_function_block.h"

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

class PythonFunctionBlockImpl final : public FunctionBlock
{
public:
    using Self = PythonFunctionBlockImpl;
    using Super = FunctionBlock;

    PythonFunctionBlockImpl(const FunctionBlockTypePtr& type,
                            const ContextPtr& context,
                            const ComponentPtr& parent,
                            const StringPtr& localId,
                            const PropertyObjectPtr& /*config*/,
                            py::object pyFunctionBlock)
        : Super(type, context, parent, localId)
        , pyFunctionBlock(std::move(pyFunctionBlock))
    {
    }

    InputPortConfigPtr createAndAddInputPortPublic(const std::string& localId,
                                                   PacketReadyNotification notificationMethod,
                                                   BaseObjectPtr customData,
                                                   bool requestGapPackets,
                                                   const PermissionsPtr& permissions)
    {
        return this->createAndAddInputPort(localId,
                                           notificationMethod,
                                           std::move(customData),
                                           requestGapPackets,
                                           permissions);
    }

    SignalConfigPtr createAndAddSignalPublic(const std::string& localId,
                                             const DataDescriptorPtr& descriptor,
                                             bool visible,
                                             bool isPublic,
                                             const PermissionsPtr& permissions)
    {
        return this->createAndAddSignal(localId, descriptor, visible, isPublic, permissions);
    }

    void removeInputPortPublic(const InputPortConfigPtr& port)
    {
        this->removeInputPort(port);
    }

    void removeSignalPublic(const SignalConfigPtr& signal)
    {
        this->removeSignal(signal);
    }

protected:
    bool onAcceptsSignal(const InputPortPtr& port, const SignalPtr& signal) override
    {
        py::gil_scoped_acquire acquire;
        if (!py::hasattr(pyFunctionBlock, "on_accepts_signal"))
            return Super::onAcceptsSignal(port, signal);

        py::object result = pyFunctionBlock.attr("on_accepts_signal")(
            py::cast(port, py::return_value_policy::reference),
            py::cast(signal, py::return_value_policy::reference));

        if (result.is_none())
            return Super::onAcceptsSignal(port, signal);

        return result.cast<bool>();
    }

    void onConnected(const InputPortPtr& port) override
    {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(pyFunctionBlock, "on_connected"))
            pyFunctionBlock.attr("on_connected")(py::cast(port, py::return_value_policy::reference));
    }

    void onDisconnected(const InputPortPtr& port) override
    {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(pyFunctionBlock, "on_disconnected"))
            pyFunctionBlock.attr("on_disconnected")(py::cast(port, py::return_value_policy::reference));
    }

    void onPacketReceived(const InputPortPtr& port) override
    {
        py::gil_scoped_acquire acquire;
        if (py::hasattr(pyFunctionBlock, "on_packet_received"))
            pyFunctionBlock.attr("on_packet_received")(py::cast(port, py::return_value_policy::reference));
    }

private:
    py::object pyFunctionBlock;
};

static PythonFunctionBlockImpl* asPythonFunctionBlockImpl(IFunctionBlock* fb)
{
    if (!fb)
        throw std::invalid_argument("function_block must not be null");

    auto* impl = dynamic_cast<PythonFunctionBlockImpl*>(fb);
    if (!impl)
        throw std::invalid_argument("function_block is not a Python function block instance");
    return impl;
}

FunctionBlockPtr createPythonFunctionBlock(const FunctionBlockTypePtr& type,
                                           const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& config,
                                           py::object pyFunctionBlock)
{
    auto fb = createWithImplementation<IFunctionBlock, PythonFunctionBlockImpl>(
        type, context, parent, localId, config, std::move(pyFunctionBlock));

    auto* raw = *fb;
    return FunctionBlockPtr(raw);
}

InputPortConfigPtr pythonFunctionBlockCreateAndAddInputPort(IFunctionBlock* fb,
                                                            const std::string& localId,
                                                            PacketReadyNotification notificationMethod,
                                                            const BaseObjectPtr& customData,
                                                            bool requestGapPackets,
                                                            const PermissionsPtr& permissions)
{
    return asPythonFunctionBlockImpl(fb)->createAndAddInputPortPublic(localId,
                                                                     notificationMethod,
                                                                     customData,
                                                                     requestGapPackets,
                                                                     permissions);
}

SignalConfigPtr pythonFunctionBlockCreateAndAddSignal(IFunctionBlock* fb,
                                                      const std::string& localId,
                                                      const DataDescriptorPtr& descriptor,
                                                      bool visible,
                                                      bool isPublic,
                                                      const PermissionsPtr& permissions)
{
    return asPythonFunctionBlockImpl(fb)->createAndAddSignalPublic(localId, descriptor, visible, isPublic, permissions);
}

void pythonFunctionBlockRemoveInputPort(IFunctionBlock* fb, IInputPortConfig* port)
{
    asPythonFunctionBlockImpl(fb)->removeInputPortPublic(InputPortConfigPtr::Borrow(port));
}

void pythonFunctionBlockRemoveSignal(IFunctionBlock* fb, ISignalConfig* signal)
{
    asPythonFunctionBlockImpl(fb)->removeSignalPublic(SignalConfigPtr::Borrow(signal));
}

END_NAMESPACE_OPENDAQ

