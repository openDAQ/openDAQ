//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (PythonGenerator).
// </auto-generated>
//------------------------------------------------------------------------------

/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"


PyDaqIntf<daq::IScheduler, daq::IBaseObject> declareIScheduler(pybind11::module_ m)
{
    return wrapInterface<daq::IScheduler, daq::IBaseObject>(m, "IScheduler");
}

void defineIScheduler(pybind11::module_ m, PyDaqIntf<daq::IScheduler, daq::IBaseObject> cls)
{
    cls.doc() = "A thread-pool scheduler that supports scheduling one-off functions as well as dependency graphs.";

    m.def("Scheduler", &daq::Scheduler_Create);

    cls.def("schedule_function",
        [](daq::IScheduler *object, daq::IFunction* function)
        {
            const auto objectPtr = daq::SchedulerPtr::Borrow(object);
            return objectPtr.scheduleFunction(function).detach();
        },
        py::arg("function"),
        "Schedules the specified @p work function to run on the thread-pool. The call does not block but immediately returns an @p awaitable that represents the asynchronous execution. It can be waited upon and queried for status and result.");
    cls.def("schedule_work",
        [](daq::IScheduler *object, daq::IWork* work)
        {
            const auto objectPtr = daq::SchedulerPtr::Borrow(object);
            objectPtr.scheduleWork(work);
        },
        py::arg("work"),
        "Schedules the specified work callback to run on the thread-pool. The call does not block.");
    cls.def("schedule_graph",
        [](daq::IScheduler *object, daq::ITaskGraph* graph)
        {
            const auto objectPtr = daq::SchedulerPtr::Borrow(object);
            return objectPtr.scheduleGraph(graph).detach();
        },
        py::arg("graph"),
        "Schedules the specified dependency @p graph to run on the thread-pool. The call does not block but immediately returns an @p awaitable that represents the asynchronous execution. It can be waited upon and queried for status and result. <b>Any exceptions that occur during the graph execution are silently ignored.</b>");
    cls.def("stop",
        [](daq::IScheduler *object)
        {
            const auto objectPtr = daq::SchedulerPtr::Borrow(object);
            objectPtr.stop();
        },
        "Cancels all outstanding work and waits for the remaining to complete. After this point the scheduler does not allow any new work or graphs for scheduling.");
    cls.def("wait_all",
        [](daq::IScheduler *object)
        {
            const auto objectPtr = daq::SchedulerPtr::Borrow(object);
            objectPtr.waitAll();
        },
        "Waits fo all current scheduled work and tasks to complete.");
    cls.def_property_readonly("multi_threaded",
        [](daq::IScheduler *object)
        {
            const auto objectPtr = daq::SchedulerPtr::Borrow(object);
            return objectPtr.isMultiThreaded();
        },
        "Returns whether more than one worker thread is used.");
}
