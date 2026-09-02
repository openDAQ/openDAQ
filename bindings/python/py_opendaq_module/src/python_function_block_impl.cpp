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

#include <py_opendaq_module/python_function_block_impl.h>
#include <py_opendaq_module/python_runtime.h>

#include <py_core_types/py_opendaq_daq.h>
#include <coretypes/exceptions.h>
#include <coretypes/weakrefptr.h>

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

// Resolves the weak reference handed to Python, throwing the same "already destroyed" error
// PythonFunctionBlockHandle::ensureValid() used to when the backing function block is gone.
static ObjectPtr<IPythonFunctionBlock> resolveWeakFb(const WeakRefPtr<IPythonFunctionBlock>& weakFb)
{
    auto fb = weakFb.getRef();
    if (!fb.assigned())
        DAQ_THROW_EXCEPTION(InvalidStateException, "The backing function block no longer exists");
    return fb;
}

// Registered once, lazily, the first time a PythonFunctionBlockImpl is constructed - always on
// PythonRuntime's dispatch thread, so a function-local static is enough (no extra locking).
static py::object createHandleObject(const WeakRefPtr<IPythonFunctionBlock>& weakFb)
{
    static bool registered = []
    {
        py::module_ opendaqModule = py::module_::import("opendaq");
        py::class_<WeakRefPtr<IPythonFunctionBlock>>(opendaqModule, "_PythonFunctionBlockHandle")
            .def_property_readonly(
                "signals_folder",
                [](const WeakRefPtr<IPythonFunctionBlock>& self) -> IComponent*
                {
                    IComponent* folder;
                    checkErrorInfo(resolveWeakFb(self)->signalsFolder(&folder));
                    return folder;
                },
                py::return_value_policy::take_ownership)
            .def_property_readonly(
                "input_ports_folder",
                [](const WeakRefPtr<IPythonFunctionBlock>& self) -> IComponent*
                {
                    IComponent* folder;
                    checkErrorInfo(resolveWeakFb(self)->inputPortsFolder(&folder));
                    return folder;
                },
                py::return_value_policy::take_ownership)
            .def("add_signal",
                 [](const WeakRefPtr<IPythonFunctionBlock>& self, ISignal* signal)
                 { checkErrorInfo(resolveWeakFb(self)->addSignal(signal)); })
            .def("remove_signal",
                 [](const WeakRefPtr<IPythonFunctionBlock>& self, ISignalConfig* signal)
                 { checkErrorInfo(resolveWeakFb(self)->removeSignal(signal)); })
            .def("add_input_port",
                 [](const WeakRefPtr<IPythonFunctionBlock>& self, IInputPort* inputPort)
                 { checkErrorInfo(resolveWeakFb(self)->addInputPort(inputPort)); })
            .def("remove_input_port",
                 [](const WeakRefPtr<IPythonFunctionBlock>& self, IInputPortConfig* inputPort)
                 { checkErrorInfo(resolveWeakFb(self)->removeInputPort(inputPort)); })
            .def("add_nested_function_block",
                 [](const WeakRefPtr<IPythonFunctionBlock>& self, IFunctionBlock* functionBlock)
                 { checkErrorInfo(resolveWeakFb(self)->addNestedFunctionBlock(functionBlock)); })
            .def("remove_nested_function_block",
                 [](const WeakRefPtr<IPythonFunctionBlock>& self, IFunctionBlock* functionBlock)
                 { checkErrorInfo(resolveWeakFb(self)->removeNestedFunctionBlock(functionBlock)); })
            .def(
                "set_status_message",
                [](const WeakRefPtr<IPythonFunctionBlock>& self, ComponentStatus status, const std::string& message)
                {
                    const StringPtr messagePtr(message);
                    checkErrorInfo(resolveWeakFb(self)->setStatusMessage(status, messagePtr));
                },
                py::arg("status"),
                py::arg("message") = "")
            .def_property_readonly(
                "ref",
                [](const WeakRefPtr<IPythonFunctionBlock>& self) -> IFunctionBlock*
                {
                    // Unlike every other method here, deliberately doesn't use resolveWeakFb()
                    // (which throws) - this is the one meant for a plain liveness check (e.g.
                    // `if self._cpp_fb.ref is None: return` at the top of a self-rescheduling
                    // callback), mirroring Python's own weakref.ref() convention of resolving to
                    // the referent or None rather than raising.
                    auto fb = self.getRef();
                    if (!fb.assigned())
                        return nullptr;
                    return fb.asPtr<IFunctionBlock>().detach();
                });
        return true;
    }();
    (void) registered;

    return py::cast(weakFb);
}

// PythonFunctionBlockImpl

PythonFunctionBlockImpl::PythonFunctionBlockImpl(const FunctionBlockTypePtr& type,
                                                  const ContextPtr& context,
                                                  const ComponentPtr& parent,
                                                  const StringPtr& localId,
                                                  py::object pyDelegateIn)
    : Super(type, context, parent, localId)
    , pyDelegate(std::move(pyDelegateIn))
{
    // Seeds the "ComponentStatus" entry with ComponentStatus::Ok, so setStatusMessage() below
    // has something to update - setComponentStatusWithMessage() throws otherwise.
    this->initComponentStatus();

    // Non-owning on purpose - see the class comment on IPythonFunctionBlock for why an owning
    // reference here would leak.
    const WeakRefPtr<IPythonFunctionBlock> weakSelf(this->borrowPtr<ObjectPtr<IPythonFunctionBlock>>());
    pyDelegate.attr("_cpp_fb") = createHandleObject(weakSelf);
    pyDelegate.attr("on_init")();
}

PythonFunctionBlockImpl::~PythonFunctionBlockImpl()
{
    // Same rule as PythonModule: never touch a py::object off the dispatch thread.
    PythonRuntime::instance().run(
        [this]
        {
            pyDelegate.attr("_cpp_fb") = py::none();
            pyDelegate = py::object();
        });
}

template <typename Fn>
void PythonFunctionBlockImpl::dispatchAsync(const char* hookName, Fn&& fn)
{
    PythonRuntime::instance().post(
        [this, hookName, fn = std::forward<Fn>(fn)]() mutable
        {
            try
            {
                fn();
            }
            catch (const std::exception& e)
            {
                LOG_E("Python {}() failed: {}", hookName, e.what())
            }
            catch (...)
            {
                LOG_E("Python {}() failed with a non-standard exception", hookName)
            }
        });
}

bool PythonFunctionBlockImpl::onAcceptsSignal(const InputPortPtr& port, const SignalPtr& signal)
{
    return PythonRuntime::instance().run(
        [this, &port, &signal]() -> bool
        {
            if (!py::hasattr(pyDelegate, "on_accepts_signal"))
                return true;

            py::object result = pyDelegate.attr("on_accepts_signal")(baseObjectToPyObject(port, IInputPort::Id, false),
                                                                       baseObjectToPyObject(signal, ISignal::Id, false));
            return result.is_none() ? true : py::cast<bool>(result);
        });
}

SignalPtr PythonFunctionBlockImpl::onGetStatusSignal()
{
    return PythonRuntime::instance().run(
        [this]() -> SignalPtr
        {
            if (!py::hasattr(pyDelegate, "on_get_status_signal"))
                return nullptr;

            py::object result = pyDelegate.attr("on_get_status_signal")();
            if (result.is_none())
                return nullptr;

            return pyObjectToBaseObject(result, false).asPtr<ISignal>();
        });
}

void PythonFunctionBlockImpl::onConnected(const InputPortPtr& port)
{
    dispatchAsync("on_connected", [this, port] { pyDelegate.attr("on_connected")(baseObjectToPyObject(port, IInputPort::Id, false)); });
}

void PythonFunctionBlockImpl::onDisconnected(const InputPortPtr& port)
{
    dispatchAsync("on_disconnected",
                   [this, port] { pyDelegate.attr("on_disconnected")(baseObjectToPyObject(port, IInputPort::Id, false)); });
}

void PythonFunctionBlockImpl::onPacketReceived(const InputPortPtr& port)
{
    dispatchAsync("on_packet_received",
                   [this, port] { pyDelegate.attr("on_packet_received")(baseObjectToPyObject(port, IInputPort::Id, false)); });
}

ErrCode PythonFunctionBlockImpl::signalsFolder(IComponent** folder) const
{
    OPENDAQ_PARAM_NOT_NULL(folder);

    *folder = this->signals.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PythonFunctionBlockImpl::inputPortsFolder(IComponent** folder) const
{
    OPENDAQ_PARAM_NOT_NULL(folder);

    *folder = this->inputPorts.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PythonFunctionBlockImpl::addSignal(ISignal* signal)
{
    return daqTry([&] { Super::addSignal(SignalPtr::Borrow(signal)); });
}

ErrCode PythonFunctionBlockImpl::removeSignal(ISignalConfig* signal)
{
    return daqTry([&] { Super::removeSignal(SignalConfigPtr::Borrow(signal)); });
}

ErrCode PythonFunctionBlockImpl::addInputPort(IInputPort* inputPort)
{
    return daqTry(
        [&]
        {
            // addInputPort() alone (unlike createAndAddInputPort(), which we deliberately don't
            // expose) does not register this function block as the port's
            // IInputPortNotifications listener - so without this, onAcceptsSignal/onConnected/
            // onDisconnected/onPacketReceived would silently never fire for a port added
            // through add_input_port(). SameThread is fine here regardless of which native
            // thread ends up calling in: onPacketReceived() always re-dispatches through
            // PythonRuntime itself.
            const auto portPtr = InputPortPtr::Borrow(inputPort);
            const auto config = portPtr.asPtr<IInputPortConfig>();
            config.setListener(this->borrowPtr<InputPortNotificationsPtr>());
            config.setNotificationMethod(PacketReadyNotification::SameThread);

            Super::addInputPort(portPtr);
        });
}

ErrCode PythonFunctionBlockImpl::removeInputPort(IInputPortConfig* inputPort)
{
    return daqTry([&] { Super::removeInputPort(InputPortConfigPtr::Borrow(inputPort)); });
}

ErrCode PythonFunctionBlockImpl::addNestedFunctionBlock(IFunctionBlock* functionBlock)
{
    return daqTry([&] { Super::addNestedFunctionBlock(FunctionBlockPtr::Borrow(functionBlock)); });
}

ErrCode PythonFunctionBlockImpl::removeNestedFunctionBlock(IFunctionBlock* functionBlock)
{
    return daqTry([&] { Super::removeNestedFunctionBlock(FunctionBlockPtr::Borrow(functionBlock)); });
}

ErrCode PythonFunctionBlockImpl::setStatusMessage(ComponentStatus status, IString* message)
{
    return daqTry([&] { this->setComponentStatusWithMessage(status, message ? StringPtr::Borrow(message) : StringPtr("")); });
}

FunctionBlockPtr createPythonFunctionBlock(const ContextPtr& context,
                                            const ComponentPtr& parent,
                                            const StringPtr& localId,
                                            py::object pyDelegate)
{
    py::object pyType = pyDelegate.attr("create_function_block_type")();
    const auto fbType = pyObjectToBaseObject(pyType, false).asPtr<IFunctionBlockType>();

    return createWithImplementation<IFunctionBlock, PythonFunctionBlockImpl>(fbType, context, parent, localId, std::move(pyDelegate));
}

END_NAMESPACE_OPENDAQ
