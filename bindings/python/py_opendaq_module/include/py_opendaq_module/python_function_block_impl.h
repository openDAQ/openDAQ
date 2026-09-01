/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/function_block_impl.h>
#include <py_opendaq_module/python_function_block.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief A FunctionBlock backed by a Python delegate object (an instance of the plugin's own
 * `FunctionBlock` subclass, already constructed by the time this wraps it - see the lifecycle
 * note on the Python-side base class).
 *
 * Every entry point into `pyDelegate` runs through PythonRuntime, same as PythonModule:
 * onAcceptsSignal is a synchronous decision the caller needs immediately, so it uses run();
 * onConnected/onDisconnected/onPacketReceived are notifications nobody is waiting on, so they
 * use post() and log failures through this function block's own loggerComponent instead of
 * propagating them anywhere.
 *
 * Construction must happen from within a PythonRuntime::run() callable (GIL already held) -
 * the constructor calls into `pyDelegate` (setting `_cpp_fb`, calling `on_init()`) without
 * dispatching itself. createPythonFunctionBlock() is the only sanctioned way to build one.
 */
class PythonFunctionBlockImpl final : public FunctionBlockImpl<IFunctionBlock, IPythonFunctionBlock>
{
public:
    using Self = PythonFunctionBlockImpl;
    using Super = FunctionBlockImpl<IFunctionBlock, IPythonFunctionBlock>;

    PythonFunctionBlockImpl(const FunctionBlockTypePtr& type,
                             const ContextPtr& context,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             pybind11::object pyDelegate);
    ~PythonFunctionBlockImpl() override;

    bool onAcceptsSignal(const InputPortPtr& port, const SignalPtr& signal) override;
    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;
    void onPacketReceived(const InputPortPtr& port) override;
    SignalPtr onGetStatusSignal() override;

    // IPythonFunctionBlock
    ErrCode INTERFACE_FUNC signalsFolder(IComponent** folder) const override;
    ErrCode INTERFACE_FUNC inputPortsFolder(IComponent** folder) const override;
    ErrCode INTERFACE_FUNC addSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC removeSignal(ISignalConfig* signal) override;
    ErrCode INTERFACE_FUNC addInputPort(IInputPort* inputPort) override;
    ErrCode INTERFACE_FUNC removeInputPort(IInputPortConfig* inputPort) override;
    ErrCode INTERFACE_FUNC addNestedFunctionBlock(IFunctionBlock* functionBlock) override;
    ErrCode INTERFACE_FUNC removeNestedFunctionBlock(IFunctionBlock* functionBlock) override;
    ErrCode INTERFACE_FUNC setStatusMessage(ComponentStatus status, IString* message) override;

private:
    template <typename Fn>
    void dispatchAsync(const char* hookName, Fn&& fn);

    pybind11::object pyDelegate;
};

END_NAMESPACE_OPENDAQ
