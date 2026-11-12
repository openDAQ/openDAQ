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

// Hand-written binding of MultiReader2 (spec 0007, decisions A14, A16 to A19, C8).
//
// Threading: every reader call releases the GIL. The binding's own state (the params copy and the layout predicted
// from the last status) is guarded by a mutex that is only ever taken with the GIL released, so a Python callable the
// reader calls on the caller's thread (acceptsDescriptor) or a handler on a scheduler thread can always take the GIL.
// acceptsDescriptor calling its own reader raises instead of locking the mutex twice (StateMutex).

#include <pybind11/gil.h>
#include <pybind11/numpy.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

#include <opendaq/multi_reader2_factory.h>
#include <opendaq/multi_reader2_input_status_ptr.h>
#include <opendaq/multi_reader2_params_ptr.h>
#include <opendaq/multi_reader2_ptr.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/reader_utils.h>

namespace py_mr2
{

bool interpreterGone()
{
#if PY_VERSION_HEX >= 0x030D0000
    return !Py_IsInitialized() || Py_IsFinalizing();
#else
    return !Py_IsInitialized() || _Py_IsFinalizing();
#endif
}

// The most specific Python wrapper for an object the reader hands out: signals, input ports and descriptors
py::object typedToPy(const daq::BaseObjectPtr& object)
{
    if (!object.assigned())
        return py::none();
    if (object.supportsInterface<daq::ISignal>())
        return castFrom<daq::ISignal>(object.getObject());
    if (object.supportsInterface<daq::IInputPortConfig>())
        return castFrom<daq::IInputPortConfig>(object.getObject());
    if (object.supportsInterface<daq::IInputPort>())
        return castFrom<daq::IInputPort>(object.getObject());
    if (object.supportsInterface<daq::IComponent>())
        return castFrom<daq::IComponent>(object.getObject());
    if (object.supportsInterface<daq::IDataDescriptor>())
        return castFrom<daq::IDataDescriptor>(object.getObject());
    return baseObjectToPyObject(object);
}

// A Python callable as an IFunction. The reader may call it and drop it on any thread, so the call and the release of
// the callable both take the GIL. An exception raised by the callable is returned as an error, which the reader counts
// as a rejection
class PyCallableFunction : public daq::ImplementationOf<daq::IFunction, daq::ICoreType>
{
public:
    explicit PyCallableFunction(py::object callable)
        : callable(std::move(callable))
    {
    }

    ~PyCallableFunction() override
    {
        if (interpreterGone())
        {
            callable.release();
            return;
        }
        py::gil_scoped_acquire gil;
        callable = py::object();
    }

    daq::ErrCode INTERFACE_FUNC call(daq::IBaseObject* params, daq::IBaseObject** result) override
    {
        OPENDAQ_PARAM_NOT_NULL(result);
        if (interpreterGone())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The Python interpreter is shutting down");

        std::string error;
        try
        {
            py::gil_scoped_acquire gil;
            const auto paramsPtr = daq::BaseObjectPtr::Borrow(params);
            py::tuple args;
            if (!paramsPtr.assigned())
            {
                args = py::tuple(0);
            }
            else if (const auto list = paramsPtr.asPtrOrNull<daq::IList>(true); list.assigned())
            {
                args = py::tuple(list.getCount());
                for (daq::SizeT i = 0; i < list.getCount(); ++i)
                    args[i] = typedToPy(list.getItemAt(i));
            }
            else
            {
                args = py::make_tuple(typedToPy(paramsPtr));
            }

            const py::object returned = callable(*args);
            const bool accepted = static_cast<bool>(py::bool_(returned));
            *result = daq::Boolean_Create(accepted);
            return OPENDAQ_SUCCESS;
        }
        catch (const py::error_already_set& e)
        {
            error = e.what();
        }
        catch (const std::exception& e)
        {
            error = e.what();
        }
        // No format arguments: DAQ_MAKE_ERROR_INFO formats into 1024 bytes with sprintf_s, and a longer message aborts the process
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "The Python callable raised: " + error);
    }

    daq::ErrCode INTERFACE_FUNC getCoreType(daq::CoreType* coreType) override
    {
        OPENDAQ_PARAM_NOT_NULL(coreType);
        *coreType = daq::ctFunc;
        return OPENDAQ_SUCCESS;
    }

private:
    py::object callable;
};

// The binding's mutex, which raises instead of locking twice. acceptsDescriptor runs on the thread that holds it, inside
// one of the reader's calls; if it calls the same reader again, a second lock would be undefined behaviour (a deadlock
// on most platforms). The raised error fails the callable, which the reader counts as a rejection.
class StateMutex
{
public:
    void lock()
    {
        if (owner.load() == std::this_thread::get_id())
            throw std::runtime_error("A MultiReader2 callable cannot call its own reader");
        mutex.lock();
        owner.store(std::this_thread::get_id());
    }

    void unlock()
    {
        owner.store(std::thread::id());
        mutex.unlock();
    }

private:
    std::mutex mutex;
    std::atomic<std::thread::id> owner{};
};

// The on_data_available callable. The reader object owns the slot; the handler registered on the reader's event only
// sees it weakly, so the only strong reference is the one the garbage collector can traverse
struct HandlerSlot
{
    py::object callable;
};

class DataAvailableHandler : public daq::ImplementationOf<daq::IEventHandler>
{
public:
    explicit DataAvailableHandler(std::weak_ptr<HandlerSlot> slot)
        : slot(std::move(slot))
    {
    }

    daq::ErrCode INTERFACE_FUNC handleEvent(daq::IBaseObject* /*sender*/, daq::IEventArgs* /*args*/) override
    {
        if (slot.expired() || interpreterGone())
            return OPENDAQ_SUCCESS;

        std::string error;
        try
        {
            py::gil_scoped_acquire gil;
            py::object callable;
            if (const auto owned = slot.lock())
                callable = owned->callable;
            if (callable && !callable.is_none())
                callable();
            return OPENDAQ_SUCCESS;
        }
        catch (const py::error_already_set& e)
        {
            error = e.what();
        }
        catch (const std::exception& e)
        {
            error = e.what();
        }
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "on_data_available raised: " + error);
    }

private:
    std::weak_ptr<HandlerSlot> slot;
};

// Sample types

bool isNumeric(daq::SampleType type)
{
    switch (type)
    {
        case daq::SampleType::Float32:
        case daq::SampleType::Float64:
        case daq::SampleType::UInt8:
        case daq::SampleType::Int8:
        case daq::SampleType::UInt16:
        case daq::SampleType::Int16:
        case daq::SampleType::UInt32:
        case daq::SampleType::Int32:
        case daq::SampleType::UInt64:
        case daq::SampleType::Int64:
            return true;
        default:
            return false;
    }
}

size_t itemSize(daq::SampleType type)
{
    switch (type)
    {
        case daq::SampleType::UInt8:
        case daq::SampleType::Int8:
            return 1;
        case daq::SampleType::UInt16:
        case daq::SampleType::Int16:
            return 2;
        case daq::SampleType::Float32:
        case daq::SampleType::UInt32:
        case daq::SampleType::Int32:
            return 4;
        default:
            return 8;
    }
}

py::dtype dtypeOf(daq::SampleType type)
{
    switch (type)
    {
        case daq::SampleType::Float32:
            return py::dtype::of<float>();
        case daq::SampleType::Float64:
            return py::dtype::of<double>();
        case daq::SampleType::UInt8:
            return py::dtype::of<uint8_t>();
        case daq::SampleType::Int8:
            return py::dtype::of<int8_t>();
        case daq::SampleType::UInt16:
            return py::dtype::of<uint16_t>();
        case daq::SampleType::Int16:
            return py::dtype::of<int16_t>();
        case daq::SampleType::UInt32:
            return py::dtype::of<uint32_t>();
        case daq::SampleType::Int32:
            return py::dtype::of<int32_t>();
        case daq::SampleType::UInt64:
            return py::dtype::of<uint64_t>();
        case daq::SampleType::Int64:
            return py::dtype::of<int64_t>();
        default:
            throw py::value_error("value_read_type must be a numeric sample type; Undefined is not available from Python");
    }
}

void requireNumeric(daq::SampleType type)
{
    if (!isNumeric(type))
        throw py::value_error("value_read_type must be a numeric sample type; Undefined is not available from Python");
}

// Conversions from Python

// An owning smart pointer to the interface a Python wrapper holds (the T*& constructor adds a reference)
template <class Ptr, class Interface>
Ptr ownedFrom(const py::handle& obj)
{
    Interface* raw = obj.cast<Interface*>();
    return Ptr(raw);
}

daq::ComponentPtr toComponent(const py::handle& obj)
{
    if (obj.is_none())
        return nullptr;
    if (!py::isinstance<daq::IComponent>(obj))
        throw py::type_error("Expected a signal or an input port");
    return ownedFrom<daq::ComponentPtr, daq::IComponent>(obj);
}

daq::ListPtr<daq::IComponent> toComponentList(const py::handle& obj)
{
    auto list = daq::List<daq::IComponent>();
    if (py::isinstance<daq::IList>(obj))
    {
        for (const auto& item : ownedFrom<daq::ListPtr<daq::IBaseObject>, daq::IList>(obj))
            list.pushBack(item.asPtr<daq::IComponent>());
        return list;
    }
    if (py::isinstance<py::str>(obj) || !py::isinstance<py::iterable>(obj))
        throw py::type_error("inputs must be a list of signals or input ports");
    for (const auto& item : obj)
    {
        const auto component = toComponent(item);
        if (!component.assigned())
            throw py::type_error("inputs must not contain None");
        list.pushBack(component);
    }
    return list;
}

daq::FunctionPtr toFunction(const py::handle& obj)
{
    if (obj.is_none())
        return nullptr;
    // Every Python callable, a daq.Function included, is wrapped: the reader may drop the function without the GIL, and
    // only the wrapper releases its Python object under the GIL
    if (!PyCallable_Check(obj.ptr()))
        throw py::type_error("accepts_descriptor must be callable: (input, value_descriptor, domain_descriptor) -> bool");
    return daq::createWithImplementation<daq::IFunction, PyCallableFunction>(py::reinterpret_borrow<py::object>(obj));
}

daq::MultiReader2ParamsPtr copyParams(const daq::MultiReader2ParamsPtr& source)
{
    auto copy = daq::MultiReader2Params();
    auto inputs = daq::List<daq::IComponent>();
    if (const auto sourceInputs = source.getInputs(); sourceInputs.assigned())
    {
        for (const auto& input : sourceInputs)
            inputs.pushBack(input);
    }
    copy.setInputs(inputs);
    copy.setMainInput(source.getMainInput());
    copy.setAcceptsDescriptor(source.getAcceptsDescriptor());
    copy.setValueReadType(source.getValueReadType());
    copy.setMinReadCount(source.getMinReadCount());
    copy.setErrorPolicy(source.getErrorPolicy());
    return copy;
}

// Applies keyword arguments named as the params' properties; on_data_available is taken out by the caller first
void applyKwargs(const daq::MultiReader2ParamsPtr& params, const py::kwargs& kwargs)
{
    for (const auto& [key, value] : kwargs)
    {
        const auto name = key.cast<std::string>();
        if (name == "inputs")
            params.setInputs(toComponentList(value));
        else if (name == "main_input")
            params.setMainInput(toComponent(value));
        else if (name == "accepts_descriptor")
            params.setAcceptsDescriptor(toFunction(value));
        else if (name == "value_read_type")
            params.setValueReadType(value.cast<daq::SampleType>());
        else if (name == "min_read_count")
            params.setMinReadCount(value.cast<daq::SizeT>());
        else if (name == "error_policy")
            params.setErrorPolicy(value.cast<daq::MultiReader2ErrorPolicy>());
        else
            throw py::type_error("MultiReader2: unexpected keyword argument '" + name + "'");
    }
}

// Conversions to Python

py::object inputToPy(const daq::ComponentPtr& input)
{
    return typedToPy(input);
}

py::object statusToPy(const daq::MultiReader2StatusPtr& status)
{
    return castFrom<daq::IMultiReader2Status>(status.getObject());
}

py::object paramsToPy(const daq::MultiReader2ParamsPtr& params)
{
    return castFrom<daq::IMultiReader2Params>(params.getObject());
}

py::list componentsToPy(const daq::ListPtr<daq::IComponent>& inputs)
{
    py::list list;
    if (inputs.assigned())
    {
        for (const auto& input : inputs)
            list.append(inputToPy(input));
    }
    return list;
}

std::string idOf(const daq::ComponentPtr& component)
{
    return component.assigned() ? component.getGlobalId().toStdString() : std::string();
}

// The layout of the next read, predicted from the last status: a read never crosses a status with changes, so the
// samples it returns follow the descriptors of the status before it

struct SlotLayout
{
    bool contributes = false;
    size_t sampleSize = 0;
    std::vector<py::ssize_t> elementShape;
};

struct Layout
{
    bool known = false;
    daq::SampleType readType = daq::SampleType::Float64;
    std::vector<SlotLayout> slots;
    bool hasDomain = false;
    int64_t start = 0;
    int64_t delta = 1;
    int64_t resolutionNum = 1;
    int64_t resolutionDen = 1;
    std::string origin;
};

Layout makeLayout(const daq::MultiReader2StatusPtr& status, daq::SampleType readType)
{
    Layout layout;
    layout.known = true;
    layout.readType = readType;
    const size_t item = itemSize(readType);
    const bool valid = status.getStatus() != daq::ComponentStatus::Error;

    for (const auto& inputStatus : status.getInputs())
    {
        SlotLayout slot;
        if (valid && inputStatus.getError() == daq::MultiReader2InputError::None)
        {
            slot.sampleSize = inputStatus.getSampleSize();
            slot.contributes = slot.sampleSize > 0;
            const auto elements = static_cast<py::ssize_t>(slot.sampleSize / item);

            std::vector<py::ssize_t> dims;
            py::ssize_t product = 1;
            if (const auto descriptor = inputStatus.getDescriptor(); descriptor.assigned())
            {
                if (const auto dimensions = descriptor.getDimensions(); dimensions.assigned())
                {
                    for (const auto& dimension : dimensions)
                    {
                        dims.push_back(static_cast<py::ssize_t>(dimension.getSize()));
                        product *= dims.back();
                    }
                }
            }
            if (!dims.empty() && product == elements)
                slot.elementShape = dims;
            else if (elements != 1)
                slot.elementShape = {elements};
        }
        layout.slots.push_back(std::move(slot));
    }

    if (valid)
    {
        if (const auto domain = status.getDomainDescriptor(); domain.assigned())
        {
            const auto rule = domain.getRule();
            if (rule.assigned() && rule.getType() == daq::DataRuleType::Linear)
            {
                const auto parameters = rule.getParameters();
                layout.delta = static_cast<daq::Int>(parameters.get("delta"));
                layout.start = static_cast<daq::Int>(parameters.get("start"));
                const auto resolution = domain.getTickResolution();
                if (resolution.assigned())
                {
                    layout.resolutionNum = resolution.getNumerator();
                    layout.resolutionDen = resolution.getDenominator();
                }
                if (const auto origin = domain.getOrigin(); origin.assigned())
                    layout.origin = origin.toStdString();
                layout.hasDomain = true;
            }
        }
    }
    return layout;
}

// Memory the binding allocates for a read: one buffer per contributing input, not zero-filled; numpy takes
// ownership through a capsule. Inputs get separate buffers because their sample counts can differ (dividers)
struct Allocation
{
    size_t slot = 0;
    std::unique_ptr<uint8_t[]> data;
};

// A caller's buffers for read_into, one per input, taken apart under the GIL
struct UserTarget
{
    std::vector<py::object> perSlot;
    std::vector<void*> pointers;
    std::vector<std::vector<py::ssize_t>> elementShapes;
    std::vector<bool> present;
};

struct ReadOutcome
{
    size_t requested = 0;
    daq::SizeT count = 0;
    daq::SizeT offset = 0;
    daq::MultiReader2StatusPtr status;
    Layout layout;
    std::vector<Allocation> allocations;
};

class PyReadResult
{
public:
    size_t count = 0;
    daq::SizeT offset = 0;
    py::object status;
    py::list inputs;
    std::vector<std::string> inputIds;
    py::list values;
    Layout layout;
    py::object ticksCache;
    py::object timesCache;

    size_t slotOf(const py::handle& key) const
    {
        if (py::isinstance<py::int_>(key))
        {
            auto index = key.cast<py::ssize_t>();
            const auto size = static_cast<py::ssize_t>(inputIds.size());
            if (index < 0)
                index += size;
            if (index < 0 || index >= size)
                throw py::index_error("input index out of range");
            return static_cast<size_t>(index);
        }
        const auto id = idOf(toComponent(key));
        for (size_t i = 0; i < inputIds.size(); ++i)
        {
            if (inputIds[i] == id)
                return i;
        }
        throw py::key_error("not an input of the reader: " + id);
    }

    py::object getTicks()
    {
        if (ticksCache)
            return ticksCache;
        py::array_t<int64_t> ticks(static_cast<py::ssize_t>(count));
        if (count > 0)
        {
            if (!layout.hasDomain)
                throw std::runtime_error("The main domain is not known for this read");
            auto* data = ticks.mutable_data();
            const int64_t first = layout.start + static_cast<int64_t>(offset);
            for (size_t i = 0; i < count; ++i)
                data[i] = first + static_cast<int64_t>(i) * layout.delta;
        }
        ticksCache = ticks;
        return ticksCache;
    }

    py::object getTimes()
    {
        if (timesCache)
            return timesCache;
        py::array_t<int64_t> ns(static_cast<py::ssize_t>(count));
        if (count > 0)
        {
            bool ok = false;
            const auto epoch = daq::reader::parseEpoch(layout.origin, &ok);
            if (!ok)
                throw py::value_error("The main domain's origin \"" + layout.origin + "\" is not an ISO 8601 time");
            const int64_t epochNs =
                std::chrono::duration_cast<std::chrono::nanoseconds>(epoch.time_since_epoch()).count();

            const auto ticks = getTicks().cast<py::array_t<int64_t>>();
            const auto* tickData = ticks.data();
            auto* out = ns.mutable_data();
            const int64_t num = layout.resolutionNum;
            const int64_t den = layout.resolutionDen;
            constexpr int64_t nsPerSecond = 1000000000;
            for (size_t i = 0; i < count; ++i)
            {
                // Split so that ticks * num * 1e9 does not overflow
                const int64_t tick = tickData[i];
                const int64_t whole = tick / den;
                const int64_t rest = tick % den;
                out[i] = epochNs + whole * num * nsPerSecond + (rest * num * nsPerSecond) / den;
            }
        }
        timesCache = ns.attr("view")("datetime64[ns]");
        return timesCache;
    }
};

class PyMultiReader2
{
public:
    daq::MultiReader2Ptr reader;
    std::shared_ptr<HandlerSlot> handlerSlot = std::make_shared<HandlerSlot>();
    py::list inputs; // Python objects of the params' inputs, slot order

    // Guarded by mutex, which is taken only with the GIL released
    StateMutex mutex;
    daq::MultiReader2ParamsPtr params; // the binding's own copy
    std::vector<std::string> inputIds;
    Layout layout;
    daq::ObjectPtr<daq::IEventHandler> handler;

    ~PyMultiReader2()
    {
        // The reader's destruction never waits for a handler; a handler in flight finds the slot gone
        handlerSlot.reset();
    }

    static std::unique_ptr<PyMultiReader2> create(const py::object& inputsOrParams, const py::kwargs& kwargs)
    {
        auto self = std::make_unique<PyMultiReader2>();

        py::kwargs rest;
        py::object onDataAvailable;
        bool handlerGiven = false;
        splitKwargs(kwargs, rest, onDataAvailable, handlerGiven);

        daq::MultiReader2ParamsPtr params;
        if (py::isinstance<daq::IMultiReader2Params>(inputsOrParams))
        {
            params = copyParams(ownedFrom<daq::MultiReader2ParamsPtr, daq::IMultiReader2Params>(inputsOrParams));
        }
        else
        {
            params = daq::MultiReader2Params();
            params.setInputs(toComponentList(inputsOrParams));
        }
        applyKwargs(params, rest);
        requireNumeric(params.getValueReadType());

        self->inputs = componentsToPy(params.getInputs());
        std::vector<std::string> ids;
        for (const auto& input : params.getInputs())
            ids.push_back(idOf(input));

        {
            py::gil_scoped_release release;
            self->reader = daq::MultiReader2(params);
            self->params = params;
            self->inputIds = std::move(ids);
            self->layout = Layout{};
        }

        if (handlerGiven)
            self->setHandler(onDataAvailable);
        return self;
    }

    py::object configure(const py::object& paramsArg, const py::kwargs& kwargs)
    {
        py::kwargs rest;
        py::object onDataAvailable;
        bool handlerGiven = false;
        splitKwargs(kwargs, rest, onDataAvailable, handlerGiven);

        daq::MultiReader2ParamsPtr next;
        if (paramsArg.is_none())
        {
            daq::MultiReader2ParamsPtr current;
            {
                py::gil_scoped_release release;
                std::scoped_lock lock(mutex);
                current = params;
            }
            next = copyParams(current);
        }
        else if (py::isinstance<daq::IMultiReader2Params>(paramsArg))
        {
            next = copyParams(ownedFrom<daq::MultiReader2ParamsPtr, daq::IMultiReader2Params>(paramsArg));
        }
        else
        {
            throw py::type_error("configure takes a MultiReader2Params object and keyword arguments");
        }
        applyKwargs(next, rest);
        requireNumeric(next.getValueReadType());

        py::list nextInputs = componentsToPy(next.getInputs());
        std::vector<std::string> ids;
        for (const auto& input : next.getInputs())
            ids.push_back(idOf(input));

        daq::MultiReader2StatusPtr status;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            status = reader.configure(next); // a rejected object throws and leaves everything untouched
            layout = makeLayout(status, next.getValueReadType());
            params = next;
            inputIds = std::move(ids);
        }
        inputs = nextInputs;
        if (handlerGiven)
            setHandler(onDataAvailable);
        return statusToPy(status);
    }

    py::object reevaluate()
    {
        daq::MultiReader2StatusPtr status;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            status = reader.reevaluate();
            layout = makeLayout(status, params.getValueReadType());
        }
        return statusToPy(status);
    }

    py::object getParams()
    {
        daq::MultiReader2ParamsPtr current;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            current = params;
        }
        return paramsToPy(copyParams(current));
    }

    daq::SizeT availableCount()
    {
        py::gil_scoped_release release;
        return reader.getAvailableCount();
    }

    // Runs with the GIL released and the mutex held. Reads at most `count`, capped at what is available so the
    // binding's buffers are exact; with no layout yet, only the status can come back
    ReadOutcome readLocked(size_t count, UserTarget* target)
    {
        ReadOutcome outcome;
        outcome.layout = layout;
        const Layout& current = outcome.layout;

        size_t n = 0;
        if (current.known && count > 0)
            n = std::min<size_t>(count, reader.getAvailableCount());
        outcome.requested = n;

        std::vector<void*> pointers(current.slots.size(), nullptr);
        if (n > 0)
        {
            if (target)
            {
                for (size_t i = 0; i < current.slots.size(); ++i)
                {
                    const auto& slot = current.slots[i];
                    if (!slot.contributes)
                        continue;
                    if (i >= target->present.size() || !target->present[i])
                        throw py::value_error("read_into: no buffer for healthy input " + std::to_string(i));
                    if (target->elementShapes[i] != slot.elementShape)
                        throw py::value_error("read_into: the buffer of input " + std::to_string(i) +
                                              " does not match the input's sample shape");
                    pointers[i] = target->pointers[i];
                }
            }
            else
            {
                for (size_t i = 0; i < current.slots.size(); ++i)
                {
                    if (!current.slots[i].contributes)
                        continue;
                    Allocation allocation;
                    allocation.slot = i;
                    allocation.data.reset(new uint8_t[n * current.slots[i].sampleSize]);
                    pointers[i] = allocation.data.get();
                    outcome.allocations.push_back(std::move(allocation));
                }
            }
        }

        daq::SizeT read = n;
        daq::SizeT offset = 0;
        outcome.status = reader.read(n > 0 ? pointers.data() : nullptr, nullptr, &read, 0, offset);
        outcome.count = read;
        outcome.offset = offset;
        // A status without changes has nothing that differs from the previous one, so the layout still holds
        if (!current.known || outcome.status.getHasChanges())
            layout = makeLayout(outcome.status, current.readType);
        return outcome;
    }

    py::object buildResult(ReadOutcome&& outcome, UserTarget* target, const py::list& resultInputs, const std::vector<std::string>& ids)
    {
        auto result = std::make_unique<PyReadResult>();
        result->count = outcome.count;
        result->offset = outcome.offset;
        result->status = statusToPy(outcome.status);
        result->inputs = resultInputs;
        result->inputIds = ids;
        result->layout = outcome.layout;

        const auto& slots = outcome.layout.slots;
        const py::dtype dtype = dtypeOf(outcome.layout.readType);
        const auto count = static_cast<py::ssize_t>(outcome.count);
        const auto rows = py::slice(0, count, 1);

        // One array per input; None for an input that does not contribute
        std::vector<py::object> values(slots.size(), py::none());
        if (target)
        {
            for (size_t i = 0; i < slots.size(); ++i)
            {
                if (slots[i].contributes)
                    values[i] = target->perSlot[i][rows];
            }
        }
        else if (outcome.requested == 0)
        {
            for (size_t i = 0; i < slots.size(); ++i)
            {
                if (!slots[i].contributes)
                    continue;
                std::vector<py::ssize_t> shape{0};
                shape.insert(shape.end(), slots[i].elementShape.begin(), slots[i].elementShape.end());
                values[i] = py::array(dtype, shape);
            }
        }
        else
        {
            for (auto& allocation : outcome.allocations)
            {
                const auto& slot = slots[allocation.slot];
                std::vector<py::ssize_t> shape{static_cast<py::ssize_t>(outcome.requested)};
                shape.insert(shape.end(), slot.elementShape.begin(), slot.elementShape.end());
                auto* raw = allocation.data.release();
                py::capsule owner(raw, [](void* p) { delete[] static_cast<uint8_t*>(p); });
                py::array full(dtype, shape, raw, owner);
                values[allocation.slot] = outcome.count == outcome.requested ? py::object(full) : py::object(full[rows]);
            }
        }

        for (auto& value : values)
            result->values.append(value);
        return py::cast(std::move(result));
    }

    py::object read(size_t count)
    {
        ReadOutcome outcome;
        py::list resultInputs = inputs;
        std::vector<std::string> ids;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            outcome = readLocked(count, nullptr);
            ids = inputIds;
        }
        return buildResult(std::move(outcome), nullptr, resultInputs, ids);
    }

    py::object readInto(const py::object& buffers, size_t count)
    {
        UserTarget target;
        daq::SampleType readType;
        size_t slotCount;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            readType = params.getValueReadType();
            slotCount = inputIds.size();
        }
        const py::dtype dtype = dtypeOf(readType);

        auto checkArray = [&](const py::array& a, const char* what)
        {
            if (!a.dtype().is(dtype) && !a.dtype().equal(dtype))
                throw py::type_error(std::string("read_into: ") + what + " has dtype " + py::str(a.dtype()).cast<std::string>() +
                                     ", the read type needs " + py::str(dtype).cast<std::string>());
            if (!(a.flags() & py::array::c_style))
                throw py::value_error(std::string("read_into: ") + what + " must be C-contiguous");
            if (!a.writeable())
                throw py::value_error(std::string("read_into: ") + what + " must be writeable");
        };

        target.pointers.assign(slotCount, nullptr);
        target.elementShapes.assign(slotCount, {});
        target.present.assign(slotCount, false);

        // One buffer per input: a list of arrays (None for an input not read), or a 2D array, whose rows are the buffers
        if (!py::isinstance<py::sequence>(buffers) || py::isinstance<py::str>(buffers))
            throw py::type_error("read_into: buffers must hold one numpy array (or None) per input");
        const auto sequence = py::reinterpret_borrow<py::sequence>(buffers);
        if (sequence.size() != slotCount)
            throw py::value_error("read_into: buffers must hold one numpy array (or None) per input; got " +
                                  std::to_string(sequence.size()) + " for " + std::to_string(slotCount) + " inputs");
        target.perSlot.assign(slotCount, py::none());
        for (size_t i = 0; i < slotCount; ++i)
        {
            py::object item = sequence[i];
            if (item.is_none())
                continue;
            if (!py::isinstance<py::array>(item))
                throw py::type_error("read_into: the buffer of input " + std::to_string(i) + " is not a numpy array");
            auto a = py::reinterpret_borrow<py::array>(item);
            const std::string what = "the buffer of input " + std::to_string(i);
            checkArray(a, what.c_str());
            if (a.ndim() < 1 || static_cast<size_t>(a.shape(0)) < count)
                throw py::value_error("read_into: count is larger than " + what);
            target.perSlot[i] = a;
            target.pointers[i] = a.mutable_data();
            target.elementShapes[i] = std::vector<py::ssize_t>(a.shape() + 1, a.shape() + a.ndim());
            target.present[i] = true;
        }

        ReadOutcome outcome;
        py::list resultInputs = inputs;
        std::vector<std::string> ids;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            if (layout.known && layout.readType != readType)
                throw py::value_error("read_into: the reader was configured meanwhile; try again");
            outcome = readLocked(count, &target);
            ids = inputIds;
        }
        return buildResult(std::move(outcome), &target, resultInputs, ids);
    }

    py::tuple skipSamples(size_t count)
    {
        daq::SizeT skipped = count;
        daq::SizeT offset = 0;
        daq::MultiReader2StatusPtr status;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            status = reader.skipSamples(&skipped, 0, offset);
            if (!layout.known || status.getHasChanges())
                layout = makeLayout(status, params.getValueReadType());
        }
        return py::make_tuple(skipped, offset, statusToPy(status));
    }

    // The callable was checked by splitKwargs
    void setHandler(const py::object& callable)
    {
        handlerSlot->callable = callable.is_none() ? py::object() : callable;

        if (!callable.is_none())
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(mutex);
            if (!handler.assigned())
            {
                handler = daq::createWithImplementation<daq::IEventHandler, DataAvailableHandler>(std::weak_ptr<HandlerSlot>(handlerSlot));
                daq::IEvent* rawEvent = nullptr;
                daq::checkErrorInfo(reader->getOnDataAvailable(&rawEvent));
                const daq::ObjectPtr<daq::IEvent> event(std::move(rawEvent));
                daq::checkErrorInfo(event->addHandler(handler));
            }
        }
    }

private:
    static void splitKwargs(const py::kwargs& kwargs, py::kwargs& rest, py::object& onDataAvailable, bool& handlerGiven)
    {
        for (const auto& [key, value] : kwargs)
        {
            if (key.cast<std::string>() == "on_data_available")
            {
                // Checked here, before anything is applied
                if (!value.is_none() && !PyCallable_Check(value.ptr()))
                    throw py::type_error("on_data_available must be callable without arguments, or None");
                onDataAvailable = py::reinterpret_borrow<py::object>(value);
                handlerGiven = true;
            }
            else
            {
                rest[key] = value;
            }
        }
    }
};

// drain(): one result per read until nothing is deliverable and nothing changed
class PyDrain
{
public:
    explicit PyDrain(py::object owner)
        : owner(std::move(owner))
    {
    }

    py::object next()
    {
        if (done)
            throw py::stop_iteration();
        auto& self = owner.cast<PyMultiReader2&>();
        ReadOutcome outcome;
        py::list resultInputs = self.inputs;
        std::vector<std::string> ids;
        {
            py::gil_scoped_release release;
            std::scoped_lock lock(self.mutex);
            outcome = self.readLocked(std::numeric_limits<size_t>::max(), nullptr);
            ids = self.inputIds;
        }
        if (outcome.count == 0 && !outcome.status.getHasChanges())
        {
            done = true;
            throw py::stop_iteration();
        }
        return self.buildResult(std::move(outcome), nullptr, resultInputs, ids);
    }

private:
    py::object owner;
    bool done = false;
};

std::string localIdOf(const daq::ComponentPtr& component)
{
    return component.assigned() ? component.getLocalId().toStdString() : std::string("?");
}

const char* errorName(daq::MultiReader2InputError error)
{
    switch (error)
    {
        case daq::MultiReader2InputError::None:
            return "None";
        case daq::MultiReader2InputError::Disconnected:
            return "Disconnected";
        case daq::MultiReader2InputError::ValueDescriptorInvalid:
            return "ValueDescriptorInvalid";
        case daq::MultiReader2InputError::DomainDescriptorInvalid:
            return "DomainDescriptorInvalid";
        case daq::MultiReader2InputError::SyncFailed:
            return "SyncFailed";
    }
    return "?";
}

const char* healthName(daq::ComponentStatus status)
{
    switch (status)
    {
        case daq::ComponentStatus::Ok:
            return "Ok";
        case daq::ComponentStatus::Warning:
            return "Warning";
        case daq::ComponentStatus::Error:
            return "Error";
    }
    return "?";
}

std::string statusRepr(const daq::MultiReader2StatusPtr& status)
{
    std::ostringstream out;
    out << "<MultiReader2Status " << healthName(status.getStatus());
    const auto message = status.getStatusMessage();
    if (message.assigned() && !message.toStdString().empty())
        out << ": " << message.toStdString();

    std::vector<std::string> changes;
    if (status.getDomainDescriptorChanged())
        changes.emplace_back("domain descriptor");
    std::string descriptors;
    for (const auto& input : status.getInputs())
    {
        if (input.getDescriptorChanged())
            descriptors += (descriptors.empty() ? "" : ", ") + localIdOf(input.getInput());
    }
    if (!descriptors.empty())
        changes.push_back("descriptors of " + descriptors);
    if (status.getResynchronized())
        changes.emplace_back("resynchronized");

    if (!status.getHasChanges())
        out << "; no changes";
    else if (!changes.empty())
    {
        out << "; changes: ";
        for (size_t i = 0; i < changes.size(); ++i)
            out << (i ? ", " : "") << changes[i];
    }
    else
        out << "; changes";
    out << ">";
    return out.str();
}

} // namespace py_mr2

void wrapMultiReader2(pybind11::module_ m)
{
    using namespace py_mr2;

    py::enum_<daq::MultiReader2ErrorPolicy>(m, "MultiReader2ErrorPolicy", "What an erroring input does to the reader.")
        .value("Invalidate", daq::MultiReader2ErrorPolicy::Invalidate)
        .value("Exclude", daq::MultiReader2ErrorPolicy::Exclude);

    py::enum_<daq::MultiReader2InputError>(m, "MultiReader2InputError", "Why an input is in error.")
        .value("None", daq::MultiReader2InputError::None)
        .value("Disconnected", daq::MultiReader2InputError::Disconnected)
        .value("ValueDescriptorInvalid", daq::MultiReader2InputError::ValueDescriptorInvalid)
        .value("DomainDescriptorInvalid", daq::MultiReader2InputError::DomainDescriptorInvalid)
        .value("SyncFailed", daq::MultiReader2InputError::SyncFailed);

    auto paramsCls = wrapInterface<daq::IMultiReader2Params, daq::IBaseObject>(m, "IMultiReader2Params");
    auto statusCls = wrapInterface<daq::IMultiReader2Status, daq::IBaseObject>(m, "IMultiReader2Status");
    auto inputStatusCls = wrapInterface<daq::IMultiReader2InputStatus, daq::IBaseObject>(m, "IMultiReader2InputStatus");

    // Params

    paramsCls.doc() = "Parameters applied to a MultiReader2 via configure. Read at configure time and not retained.";

    m.def(
        "MultiReader2Params",
        [](const py::kwargs& kwargs)
        {
            auto params = daq::MultiReader2Params();
            for (const auto& [key, value] : kwargs)
            {
                if (key.cast<std::string>() == "on_data_available")
                    throw py::type_error("on_data_available is not a params field; pass it to MultiReader2 or configure");
            }
            applyKwargs(params, kwargs);
            return paramsToPy(params);
        },
        "Creates params with the defaults (Float64, MinReadCount 1, Invalidate, automatic main input); keyword arguments set fields.");

    paramsCls.def_property(
        "inputs",
        [](daq::IMultiReader2Params* object) { return componentsToPy(daq::MultiReader2ParamsPtr::Borrow(object).getInputs()); },
        [](daq::IMultiReader2Params* object, const py::object& inputs)
        { daq::MultiReader2ParamsPtr::Borrow(object).setInputs(toComponentList(inputs)); },
        "The inputs in slot order: all signals or all input ports.");
    paramsCls.def_property(
        "main_input",
        [](daq::IMultiReader2Params* object) { return inputToPy(daq::MultiReader2ParamsPtr::Borrow(object).getMainInput()); },
        [](daq::IMultiReader2Params* object, const py::object& input)
        { daq::MultiReader2ParamsPtr::Borrow(object).setMainInput(toComponent(input)); },
        "What everything else aligns against; None lets the reader choose.");
    paramsCls.def_property(
        "accepts_descriptor",
        [](daq::IMultiReader2Params* object) -> py::object
        {
            const auto function = daq::MultiReader2ParamsPtr::Borrow(object).getAcceptsDescriptor();
            if (!function.assigned())
                return py::none();
            return castFrom<daq::IFunction>(function.getObject());
        },
        [](daq::IMultiReader2Params* object, const py::object& function)
        { daq::MultiReader2ParamsPtr::Borrow(object).setAcceptsDescriptor(toFunction(function)); },
        "The owner's judgement: a callable (input, value_descriptor, domain_descriptor) -> bool, or None to accept all.");
    paramsCls.def_property(
        "value_read_type",
        [](daq::IMultiReader2Params* object) { return daq::MultiReader2ParamsPtr::Borrow(object).getValueReadType(); },
        [](daq::IMultiReader2Params* object, daq::SampleType type) { daq::MultiReader2ParamsPtr::Borrow(object).setValueReadType(type); },
        "The numeric type every input converts to; Float64 by default.");
    paramsCls.def_property(
        "min_read_count",
        [](daq::IMultiReader2Params* object) { return daq::MultiReader2ParamsPtr::Borrow(object).getMinReadCount(); },
        [](daq::IMultiReader2Params* object, daq::SizeT count) { daq::MultiReader2ParamsPtr::Borrow(object).setMinReadCount(count); },
        "The smallest number of samples a read returns; at least 1.");
    paramsCls.def_property(
        "error_policy",
        [](daq::IMultiReader2Params* object) { return daq::MultiReader2ParamsPtr::Borrow(object).getErrorPolicy(); },
        [](daq::IMultiReader2Params* object, daq::MultiReader2ErrorPolicy policy)
        { daq::MultiReader2ParamsPtr::Borrow(object).setErrorPolicy(policy); },
        "What an erroring input does to the reader; Invalidate by default.");

    // Input status

    inputStatusCls.doc() = "One input's state on a status.";
    inputStatusCls.def_property_readonly(
        "input",
        [](daq::IMultiReader2InputStatus* object) { return inputToPy(daq::MultiReader2InputStatusPtr::Borrow(object).getInput()); },
        "The signal or input port this entry describes.");
    inputStatusCls.def_property_readonly(
        "error",
        [](daq::IMultiReader2InputStatus* object) { return daq::MultiReader2InputStatusPtr::Borrow(object).getError(); },
        "The input's error; None while healthy.");
    inputStatusCls.def_property_readonly(
        "descriptor_changed",
        [](daq::IMultiReader2InputStatus* object) { return static_cast<bool>(daq::MultiReader2InputStatusPtr::Borrow(object).getDescriptorChanged()); },
        "Whether the value descriptor is new since the previous status.");
    inputStatusCls.def_property_readonly(
        "descriptor",
        [](daq::IMultiReader2InputStatus* object) -> py::object
        {
            const auto descriptor = daq::MultiReader2InputStatusPtr::Borrow(object).getDescriptor();
            if (!descriptor.assigned())
                return py::none();
            return castFrom<daq::IDataDescriptor>(descriptor.getObject());
        },
        "The input's value descriptor; None while the reader is invalid or the input is in error.");
    inputStatusCls.def_property_readonly(
        "sample_size",
        [](daq::IMultiReader2InputStatus* object) { return daq::MultiReader2InputStatusPtr::Borrow(object).getSampleSize(); },
        "The bytes one sample takes in its read buffer; 0 for an input in error.");
    inputStatusCls.def(
        "__str__",
        [](const py::object& self) { return py::repr(self); });
    inputStatusCls.def(
        "__repr__",
        [](daq::IMultiReader2InputStatus* object)
        {
            const auto status = daq::MultiReader2InputStatusPtr::Borrow(object);
            std::string repr = "<MultiReader2InputStatus " + localIdOf(status.getInput()) + ": " + errorName(status.getError());
            if (status.getDescriptorChanged())
                repr += ", descriptor changed";
            return repr + ">";
        });

    // Status

    statusCls.doc() = "Immutable result of one read: descriptor changes, the main domain, resynchronization and each input's error.";
    statusCls.def_property_readonly(
        "has_changes",
        [](daq::IMultiReader2Status* object) { return static_cast<bool>(daq::MultiReader2StatusPtr::Borrow(object).getHasChanges()); },
        "Whether anything differs from the previous status; False means the count says it all.");
    statusCls.def_property_readonly(
        "health",
        [](daq::IMultiReader2Status* object) { return daq::MultiReader2StatusPtr::Borrow(object).getStatus(); },
        "The reader's health: Ok, Warning (inputs set aside under Exclude) or Error (the reader is invalid).");
    statusCls.def_property_readonly(
        "ok",
        [](daq::IMultiReader2Status* object) { return daq::MultiReader2StatusPtr::Borrow(object).getStatus() == daq::ComponentStatus::Ok; },
        "True when health is Ok.");
    statusCls.def_property_readonly(
        "status_message",
        [](daq::IMultiReader2Status* object) { return daq::MultiReader2StatusPtr::Borrow(object).getStatusMessage().toStdString(); },
        "Empty under Ok, otherwise each input in error with its error kind.");
    statusCls.def_property_readonly(
        "main_input",
        [](daq::IMultiReader2Status* object) { return inputToPy(daq::MultiReader2StatusPtr::Borrow(object).getMainInput()); },
        "The main input in effect, or None.");
    statusCls.def_property_readonly(
        "domain_descriptor_changed",
        [](daq::IMultiReader2Status* object) { return static_cast<bool>(daq::MultiReader2StatusPtr::Borrow(object).getDomainDescriptorChanged()); },
        "Whether the main domain descriptor is new since the previous status.");
    statusCls.def_property_readonly(
        "domain_descriptor",
        [](daq::IMultiReader2Status* object) -> py::object
        {
            const auto descriptor = daq::MultiReader2StatusPtr::Borrow(object).getDomainDescriptor();
            if (!descriptor.assigned())
                return py::none();
            return castFrom<daq::IDataDescriptor>(descriptor.getObject());
        },
        "The main input's domain descriptor; None while the reader is invalid.");
    statusCls.def_property_readonly(
        "resynchronized",
        [](daq::IMultiReader2Status* object) { return static_cast<bool>(daq::MultiReader2StatusPtr::Borrow(object).getResynchronized()); },
        "Whether synchronization restarted, so the next offset is discontinuous.");
    statusCls.def_property_readonly(
        "inputs",
        [](daq::IMultiReader2Status* object)
        {
            py::list list;
            for (const auto& input : daq::MultiReader2StatusPtr::Borrow(object).getInputs())
                list.append(castFrom<daq::IMultiReader2InputStatus>(input.getObject()));
            return list;
        },
        "Every input's status, in slot order.");
    statusCls.def(
        "input_status",
        [](daq::IMultiReader2Status* object, const py::object& input)
        {
            const auto status = daq::MultiReader2StatusPtr::Borrow(object).getInputStatus(toComponent(input));
            return castFrom<daq::IMultiReader2InputStatus>(status.getObject());
        },
        py::arg("input"),
        "One input's status.");
    statusCls.def_property_readonly(
        "errors",
        [](daq::IMultiReader2Status* object)
        {
            py::dict errors;
            for (const auto& input : daq::MultiReader2StatusPtr::Borrow(object).getInputs())
            {
                if (input.getError() != daq::MultiReader2InputError::None)
                    errors[inputToPy(input.getInput())] = py::cast(input.getError());
            }
            return errors;
        },
        "{input: MultiReader2InputError} for the inputs in error; empty under Ok.");
    statusCls.def("__repr__", [](daq::IMultiReader2Status* object) { return statusRepr(daq::MultiReader2StatusPtr::Borrow(object)); });
    statusCls.def("__str__", [](daq::IMultiReader2Status* object) { return statusRepr(daq::MultiReader2StatusPtr::Borrow(object)); });

    // Read result

    py::class_<PyReadResult>(m, "MultiReader2ReadResult", "The samples, offset and status of one read.")
        .def("__len__", [](const PyReadResult& r) { return r.count; }, "The count read, in main ticks.")
        .def("__getitem__", [](const PyReadResult& r, const py::object& key) -> py::object { return r.values[r.slotOf(key)]; },
             py::arg("input"), "One input's array, by slot index or by input; None for an input in error.")
        .def_property_readonly("values", [](const PyReadResult& r) { return r.values; }, "The arrays in slot order; None for an input in error.")
        .def_property_readonly("inputs", [](const PyReadResult& r) { return r.inputs; }, "The inputs in slot order.")
        .def_property_readonly("offset", [](const PyReadResult& r) { return r.offset; }, "The main tick of the first sample, relative to the main rule's start.")
        .def_property_readonly("status", [](const PyReadResult& r) { return r.status; }, "The status of this read.")
        .def_property_readonly("ticks", [](PyReadResult& r) { return r.getTicks(); }, "The main input's domain ticks of this read, int64.")
        .def_property_readonly("times", [](PyReadResult& r) { return r.getTimes(); }, "The main input's domain of this read as datetime64[ns].")
        .def("__repr__",
             [](const PyReadResult& r)
             {
                 return "<MultiReader2ReadResult count=" + std::to_string(r.count) + " offset=" + std::to_string(r.offset) + " " +
                        py::repr(r.status).cast<std::string>() + ">";
             });

    py::class_<PyDrain>(m, "MultiReader2Drain")
        .def("__iter__", [](py::object self) { return self; })
        .def("__next__", &PyDrain::next);

    // Reader

    py::class_<PyMultiReader2> readerCls(
        m,
        "MultiReader2",
        py::custom_type_setup(
            [](PyHeapTypeObject* heapType)
            {
                auto* type = &heapType->ht_type;
                type->tp_flags |= Py_TPFLAGS_HAVE_GC;
                type->tp_traverse = [](PyObject* selfBase, visitproc visit, void* arg)
                {
#if PY_VERSION_HEX >= 0x03090000
                    Py_VISIT(Py_TYPE(selfBase));
#endif
                    if (py::detail::is_holder_constructed(selfBase))
                    {
                        auto& self = py::cast<PyMultiReader2&>(py::handle(selfBase));
                        if (self.handlerSlot && self.handlerSlot->callable)
                            Py_VISIT(self.handlerSlot->callable.ptr());
                        Py_VISIT(self.inputs.ptr());
                    }
                    return 0;
                };
                type->tp_clear = [](PyObject* selfBase)
                {
                    if (py::detail::is_holder_constructed(selfBase))
                    {
                        auto& self = py::cast<PyMultiReader2&>(py::handle(selfBase));
                        if (self.handlerSlot)
                            self.handlerSlot->callable = py::object();
                    }
                    return 0;
                };
            }));

    readerCls.doc() =
        "Reads several equal-rate signals at once, aligned onto the main input's tick grid.\n\n"
        "MultiReader2(inputs, **params) or MultiReader2(params, **changes). Keywords: value_read_type, main_input, "
        "accepts_descriptor, min_read_count, error_policy, on_data_available (a callable without arguments).";

    readerCls
        .def(py::init([](const py::object& inputs, const py::kwargs& kwargs) { return PyMultiReader2::create(inputs, kwargs); }),
             py::arg("inputs"))
        .def("configure", &PyMultiReader2::configure, py::arg("params") = py::none(),
             "Applies the reader's params with the named fields changed (or a params object) as if newly constructed; returns the status "
             "of a 0-count read. on_data_available=None removes the handler.")
        .def("reevaluate", &PyMultiReader2::reevaluate, "Judges every input again by accepts_descriptor; returns the status of a 0-count read.")
        .def_property_readonly("available_count", &PyMultiReader2::availableCount, "The aligned samples a read can return now.")
        .def_property_readonly("params", &PyMultiReader2::getParams, "A copy of the applied params.")
        .def("read", &PyMultiReader2::read, py::arg("count"), "Reads up to count aligned samples; returns a MultiReader2ReadResult.")
        .def("read_into", &PyMultiReader2::readInto, py::arg("buffers"), py::arg("count"),
             "Reads up to count samples into buffers, one numpy array (or None) per input; a 2D array (inputs, N[, *dims]) works row by row.")
        .def("skip_samples", &PyMultiReader2::skipSamples, py::arg("count"),
             "Behaves like read without copying; returns (count, offset, status).")
        .def("drain", [](py::object self) { return PyDrain(std::move(self)); },
             "Yields a read result per read until nothing is deliverable and nothing changed.");
}
