/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/graph_visualization.h>
#include <opendaq/task_flow.h>
#include <opendaq/task_graph.h>
#include <opendaq/task_internal.h>
#include <opendaq/task_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/procedure_ptr.h>
#include <coretypes/string_ptr.h>

#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

class TaskGraph;

class SubTask : public ImplementationOf<ITask, ITaskInternal>
{
public:
    SubTask();
    explicit SubTask(ProcedurePtr callable, StringPtr name = "");
    explicit SubTask(const tf::Task& task, TaskGraph* graph);

    ErrCode INTERFACE_FUNC setName(IString* taskName) override;
    ErrCode INTERFACE_FUNC getName(IString** taskName) override;
    ErrCode INTERFACE_FUNC then(ITask* continuation) override;

    ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) override;

    //
    // Implementation specific
    //

    tf::Task& INTERFACE_FUNC getTask() noexcept override;
    tf::Taskflow* INTERFACE_FUNC getFlow() noexcept override;
    void* INTERFACE_FUNC getGraph() noexcept override;

    void initialize(TaskGraph& graph);

private:
    SizeT id;
    tf::Task task;
    TaskGraph* graph;
    StringPtr name;
    ProcedurePtr callable;
};

END_NAMESPACE_OPENDAQ

template <>
struct std::hash<daq::TaskPtr>
{
    using argument_type = daq::TaskPtr;
    using result_type = daq::SizeT;

    result_type operator()(const daq::TaskPtr& task) const noexcept
    {
        return task.getHashCode();
    }
};

BEGIN_NAMESPACE_OPENDAQ

class TaskGraph : public ImplementationOf<ITaskGraph, ITaskInternal, IGraphVisualization>
{
public:
    TaskGraph();
    explicit TaskGraph(ProcedurePtr callable, const StringPtr& name);

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;

    ErrCode INTERFACE_FUNC then(ITask* continuation) override;
    ErrCode INTERFACE_FUNC dump(IString** dot) override;

    tf::Task& INTERFACE_FUNC getTask() noexcept override;
    tf::Taskflow* INTERFACE_FUNC getFlow() noexcept override;
    void* INTERFACE_FUNC getGraph() noexcept override;

    void addTask(TaskPtr newTask);

private:
    ProcedurePtr callable;

    tf::Taskflow flow;
    tf::Task task;
    std::unordered_set<TaskPtr> tasks;
};

using RootTaskImpl = daq::TaskGraph;

END_NAMESPACE_OPENDAQ
