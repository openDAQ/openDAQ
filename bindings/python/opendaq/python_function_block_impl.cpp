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

#include <Python.h>

#include <functional>

#include <opendaq/function_block_impl.h>

#include <py_opendaq/python_function_block.h>

#include <py_core_types/py_opendaq_daq.h>

#include "py_core_types/py_event_queue.h"

#include "py_opendaq/py_python_function_block.h"
namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

namespace
{
inline void enqueuePythonCallback(std::function<void()> cb)
{
    if (!cb)
        return;

    if (auto queue = PyEventQueue::GetWeak().lock())
    {
        queue->enqueue(std::move(cb));
        return;
    }

    // Fallback: if the global queue isn't initialized for some reason, execute inline.
    if (!Py_IsInitialized())
        return;

    PyGILState_STATE gilState = PyGILState_Ensure();
    try
    {
        cb();
    }
    catch (...)
    {
    }
    PyGILState_Release(gilState);
}
} // namespace

class PythonFunctionBlockImpl final : public FunctionBlockImpl<IPythonFunctionBlock>
{
public:
    using Self = PythonFunctionBlockImpl;
    using Super = FunctionBlockImpl<IPythonFunctionBlock>;

    PythonFunctionBlockImpl(const FunctionBlockTypePtr& type,
                            const ContextPtr& context,
                            const ComponentPtr& parent,
                            const StringPtr& localId,
                            const PropertyObjectPtr& /*config*/,
                            py::object pyFunctionBlock)
        : Super(type, context, parent, localId)
        , pyFunctionBlock(std::move(pyFunctionBlock))
    {
        // Provide the backing C++ IFunctionBlock instance to the Python object so it can
        // call helper methods like create_and_add_input_port() implemented in opendaq/function_block.py.
        if (!Py_IsInitialized())
            return;

        PyGILState_STATE gilState = PyGILState_Ensure();
        try
        {
            // Add an extra ref that is owned by the Python _cpp_fb InterfaceWrapper.
            // InterfaceWrapper is always constructed WITHOUT addRef but always calls releaseRef
            // in its destructor — so we balance that here.  removed() will null _cpp_fb while
            // the component is still alive, consuming this extra ref safely.
            this->addRef();

            this->pyFunctionBlock.attr("_cpp_fb") =
                py::cast(static_cast<daq::IPythonFunctionBlock*>(this), py::return_value_policy::reference);

            // Optional hook: called after the C++ wrapper exists (and _cpp_fb is set).
            // This is the right time to create ports/signals from Python.
            if (py::hasattr(this->pyFunctionBlock, "on_init"))
                this->pyFunctionBlock.attr("on_init")();
        }
        catch (...)
        {
            // If setting the attribute fails, keep the function block alive without Python-side helpers.
        }
        PyGILState_Release(gilState);
    }

    ~PythonFunctionBlockImpl() override
    {
        // This object can be destroyed on a non-Python thread. Ensure any pybind refcount
        // operations happen while holding the GIL.
        // By the time we get here, removed() will have already nulled _cpp_fb, so releasing
        // pyFunctionBlock is safe — no reentrant releaseRef() from InterfaceWrapper.
        if (!Py_IsInitialized())
            return;

        PyGILState_STATE gilState = PyGILState_Ensure();
        pyFunctionBlock = py::none();
        PyGILState_Release(gilState);
    }

    // IPythonFunctionBlock

    ErrCode INTERFACE_FUNC createAndAddInputPort(IInputPortConfig** port,
                                                  IString* localId,
                                                  PacketReadyNotification notificationMethod,
                                                  IBaseObject* customData,
                                                  Bool requestGapPackets,
                                                  IPermissions* permissions) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);
        OPENDAQ_PARAM_NOT_NULL(localId);

        const ErrCode errCode = daqTry([&]
        {
            auto result = Super::createAndAddInputPort(StringPtr::Borrow(localId).toStdString(),
                                                       notificationMethod,
                                                       BaseObjectPtr::Borrow(customData),
                                                       requestGapPackets,
                                                       PermissionsPtr::Borrow(permissions));
            *port = result.detach();
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
        return errCode;
    }

    ErrCode INTERFACE_FUNC createAndAddSignal(ISignalConfig** signal,
                                               IString* localId,
                                               IDataDescriptor* descriptor,
                                               Bool visible,
                                               Bool isPublic,
                                               IPermissions* permissions) override
    {
        OPENDAQ_PARAM_NOT_NULL(signal);
        OPENDAQ_PARAM_NOT_NULL(localId);
        const ErrCode errCode = daqTry([&]
        {
            auto result = Super::createAndAddSignal(StringPtr::Borrow(localId).toStdString(),
                                                    DataDescriptorPtr::Borrow(descriptor),
                                                    visible,
                                                    isPublic,
                                                    PermissionsPtr::Borrow(permissions));
            *signal = result.detach();
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
        return errCode;
    }

    ErrCode INTERFACE_FUNC removeInputPort(IInputPortConfig* port) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);

        const ErrCode errCode = daqTry([&]
        {
            Super::removeInputPort(InputPortConfigPtr::Borrow(port));
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
        return errCode;
    }

    ErrCode INTERFACE_FUNC removeSignal(ISignalConfig* signal) override
    {
        OPENDAQ_PARAM_NOT_NULL(signal);

        const ErrCode errCode = daqTry([&]
        {
            Super::removeSignal(SignalConfigPtr::Borrow(signal));
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
        return errCode;
    }

protected:
    void removed() override
    {
        // Null out Python's _cpp_fb back-reference BEFORE the component is released from the
        // parent folder.  The InterfaceWrapper holding _cpp_fb always calls releaseRef() on
        // destruction; by doing this inside removed() (while refcount > 0) we consume the
        // extra addRef() we took in the constructor without triggering reentrant destruction.
        if (Py_IsInitialized())
        {
            PyGILState_STATE gilState = PyGILState_Ensure();
            try
            {
                if (py::hasattr(pyFunctionBlock, "_cpp_fb"))
                    pyFunctionBlock.attr("_cpp_fb") = py::none();
            }
            catch (...) {}
            PyGILState_Release(gilState);
        }

        Super::removed();
    }

    bool onAcceptsSignal(const InputPortPtr& port, const SignalPtr& signal) override
    {
        if (!Py_IsInitialized())
            return Super::onAcceptsSignal(port, signal);

        PyGILState_STATE gilState = PyGILState_Ensure();
        if (!py::hasattr(pyFunctionBlock, "on_accepts_signal"))
        {
            PyGILState_Release(gilState);
            return Super::onAcceptsSignal(port, signal);
        }

        // Pass interface pointers through InterfaceWrapper (pybind holder type).
        py::object result = pyFunctionBlock.attr("on_accepts_signal")(
            py::cast(InterfaceWrapper<daq::IInputPort>(port.addRefAndReturn()), py::return_value_policy::take_ownership),
            py::cast(InterfaceWrapper<daq::ISignal>(signal.addRefAndReturn()), py::return_value_policy::take_ownership));

        if (result.is_none())
        {
            PyGILState_Release(gilState);
            return Super::onAcceptsSignal(port, signal);
        }

        const bool accepted = result.cast<bool>();
        PyGILState_Release(gilState);
        return accepted;
    }

    void onConnected(const InputPortPtr& port) override
    {
        const auto portPtr = port;
        enqueuePythonCallback([this, portPtr]()
        {
            if (!Py_IsInitialized())
                return;

            PyGILState_STATE gilState = PyGILState_Ensure();
            try
            {
                if (py::hasattr(pyFunctionBlock, "on_connected"))
                    pyFunctionBlock.attr("on_connected")(
                        py::cast(InterfaceWrapper<daq::IInputPort>(portPtr.addRefAndReturn()),
                                 py::return_value_policy::take_ownership));
            }
            catch (...)
            {
            }
            PyGILState_Release(gilState);
        });
    }

    void onDisconnected(const InputPortPtr& port) override
    {
        const auto portPtr = port;
        enqueuePythonCallback([this, portPtr]()
        {
            if (!Py_IsInitialized())
                return;

            PyGILState_STATE gilState = PyGILState_Ensure();
            try
            {
                if (py::hasattr(pyFunctionBlock, "on_disconnected"))
                    pyFunctionBlock.attr("on_disconnected")(
                        py::cast(InterfaceWrapper<daq::IInputPort>(portPtr.addRefAndReturn()),
                                 py::return_value_policy::take_ownership));
            }
            catch (...)
            {
            }
            PyGILState_Release(gilState);
        });
    }

    void onPacketReceived(const InputPortPtr& port) override
    {
        const auto portPtr = port;
        enqueuePythonCallback([this, portPtr]()
        {
            if (!Py_IsInitialized())
                return;

            PyGILState_STATE gilState = PyGILState_Ensure();
            try
            {
                if (py::hasattr(pyFunctionBlock, "on_packet_received"))
                    pyFunctionBlock.attr("on_packet_received")(
                        py::cast(InterfaceWrapper<daq::IInputPort>(portPtr.addRefAndReturn()),
                                 py::return_value_policy::take_ownership));
            }
            catch (...)
            {
            }
            PyGILState_Release(gilState);
        });
    }

private:
    py::object pyFunctionBlock;
};

FunctionBlockPtr createPythonFunctionBlock(const FunctionBlockTypePtr& type,
                                           const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& config,
                                           py::object pyFunctionBlock)
{
    auto fb = createWithImplementation<IFunctionBlock, PythonFunctionBlockImpl>(
        type, context, parent, localId, config, std::move(pyFunctionBlock));

    return fb;
}

END_NAMESPACE_OPENDAQ

