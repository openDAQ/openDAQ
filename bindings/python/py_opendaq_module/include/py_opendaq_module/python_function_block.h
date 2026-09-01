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
#include <opendaq/function_block.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/input_port.h>
#include <opendaq/input_port_config.h>
#include <opendaq/signal_config.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_ptr.h>

#include <pybind11/pybind11.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal interface exposing the handful of FunctionBlockImpl/SignalContainerImpl
 * helpers (add_signal/add_input_port/etc.) that a Python plugin delegate needs but that are not
 * part of the public IFunctionBlock interface. Implemented directly by PythonFunctionBlockImpl
 * (see python_function_block_impl.h), and never exposed through the normal auto-generated
 * `daq.IPythonFunctionBlock` Python binding.
 *
 * Python is instead handed a *weak* reference to it - see createHandleObject() in
 * python_function_block_impl.cpp - because PythonFunctionBlockImpl owns `pyDelegate`: if
 * `self._cpp_fb` held a normal owning reference back to the function block, that would form a
 * cycle openDAQ's refcounting can never collect (the function block would never reach a
 * refcount of zero, so it - and its Python delegate - would leak forever). Every call must
 * resolve the weak reference first and fail cleanly if the backing function block has already
 * been destroyed.
 */
DECLARE_OPENDAQ_INTERFACE(IPythonFunctionBlock, IBaseObject)
{
    // addSignal()/addInputPort() (below) reject a signal/port whose parent isn't exactly the
    // function block's own internal signals/inputPorts folder - the same folders
    // createAndAddSignal()/createAndAddInputPort() would have used, had we exposed those
    // instead. Since a plugin has no other way to construct a signal/port with that parent,
    // it needs these to construct one at all: opendaq.Signal(context, fb.signals_folder, ...).
    virtual ErrCode INTERFACE_FUNC signalsFolder(IComponent** folder) const = 0;
    virtual ErrCode INTERFACE_FUNC inputPortsFolder(IComponent** folder) const = 0;

    virtual ErrCode INTERFACE_FUNC addSignal(ISignal* signal) = 0;
    virtual ErrCode INTERFACE_FUNC removeSignal(ISignalConfig* signal) = 0;
    virtual ErrCode INTERFACE_FUNC addInputPort(IInputPort* inputPort) = 0;
    virtual ErrCode INTERFACE_FUNC removeInputPort(IInputPortConfig* inputPort) = 0;
    virtual ErrCode INTERFACE_FUNC addNestedFunctionBlock(IFunctionBlock* functionBlock) = 0;
    virtual ErrCode INTERFACE_FUNC removeNestedFunctionBlock(IFunctionBlock* functionBlock) = 0;

    // Updates the "ComponentStatus" entry the constructor seeds via initComponentStatus(), so a
    // plugin can surface its own health/error state the same way built-in components do.
    virtual ErrCode INTERFACE_FUNC setStatusMessage(ComponentStatus status, IString* message) = 0;
};

/*!
 * @brief Builds a PythonFunctionBlockImpl (see python_function_block_impl.h) around an
 * already-constructed Python delegate (the return value of the plugin's
 * `on_create_function_block`). Must be called from within a PythonRuntime::run() callable.
 */
FunctionBlockPtr createPythonFunctionBlock(const ContextPtr& context,
                                            const ComponentPtr& parent,
                                            const StringPtr& localId,
                                            pybind11::object pyDelegate);

END_NAMESPACE_OPENDAQ
