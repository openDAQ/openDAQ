#include <opendaq/task_impl.h>

BEGIN_NAMESPACE_OPENDAQ

TaskGraph::TaskGraph()
    : TaskGraph(nullptr, "")
{
}

TaskGraph::TaskGraph(ProcedurePtr callable, const StringPtr& name)
    : callable(std::move(callable))
    , flow(name)
    , task(flow.emplace([this] {
        // ZoneScopedN("RootTask");
        // TracyMessage(task.name().c_str(), task.name().size());

        // fmt::print("Root task executed\n");
        if (this->callable.assigned())
        {
            this->callable();
        }
    }))
{
    task.name(name);
}

ErrCode TaskGraph::getName(IString** name)
{
    if (name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return createStringN(name, flow.name().c_str(), flow.name().size());
}

ErrCode TaskGraph::setName(IString* name)
{
    if (name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::string n = toStdString(name);

    flow.name(n);
    task.name(n);

    return OPENDAQ_SUCCESS;
}

ErrCode TaskGraph::then(ITask* continuation)
{
    if (continuation == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    // ZoneScoped;

    auto predecessor = dynamic_cast<SubTask*>(continuation);
    if (predecessor == nullptr)
    {
        return OPENDAQ_ERR_NOT_SUPPORTED;
    }

    if (predecessor->getTask().empty())
    {
        predecessor->initialize(*this);
    }
    task.precede(predecessor->getTask());

    return OPENDAQ_SUCCESS;
}

ErrCode TaskGraph::dump(IString** dot)
{
    std::string dump = flow.dump();
    return createStringN(dot, dump.c_str(), dump.size());
}

tf::Task& TaskGraph::getTask() noexcept
{
    return task;
}

tf::Taskflow* TaskGraph::getFlow() noexcept
{
    return &flow;
}

void* TaskGraph::getGraph() noexcept
{
    return this;
}

void TaskGraph::addTask(TaskPtr newTask)
{
    tasks.insert(std::move(newTask));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, TaskGraph,
    ITaskGraph, createTaskGraph,
    IProcedure*, work,
    IString*, name
)

END_NAMESPACE_OPENDAQ
