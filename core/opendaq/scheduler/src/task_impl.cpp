#include <opendaq/task_impl.h>
#include <atomic>

BEGIN_NAMESPACE_OPENDAQ

static std::atomic<SizeT> taskCounter{0};

SubTask::SubTask(): SubTask(nullptr)
{
}

SubTask::SubTask(ProcedurePtr callable, StringPtr name)
    : id(++taskCounter)
    , graph(nullptr)
    , name(std::move(name))
    , callable(std::move(callable))
{
}

SubTask::SubTask(const tf::Task& task, TaskGraph* graph)
    : id(++taskCounter)
    , task(task)
    , graph(graph)
{
}

ErrCode SubTask::setName(IString* taskName)
{
    OPENDAQ_PARAM_NOT_NULL(taskName);

    this->name = taskName;
    if (!task.empty())
    {
        task.name(name);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode SubTask::getName(IString** taskName)
{
    OPENDAQ_PARAM_NOT_NULL(taskName);

    if (task.empty())
    {
        *taskName = name.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    return createStringN(
        taskName,
        task.name().c_str(),
        task.name().size()
    );
}

ErrCode SubTask::then(ITask* continuation)
{
    OPENDAQ_PARAM_NOT_NULL(continuation);

    auto pre = dynamic_cast<SubTask*>(continuation);
    if (pre == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "");

    if (pre->getTask().empty())
    {
        pre->initialize(*graph);
    }
    else if (task.empty())
    {
        initialize(*pre->graph);
    }

    task.precede(pre->getTask());

    return OPENDAQ_SUCCESS;
}

ErrCode SubTask::getHashCode(SizeT* hashCode)
{
    OPENDAQ_PARAM_NOT_NULL(hashCode);

    *hashCode = id;
    return OPENDAQ_SUCCESS;
}

tf::Task& SubTask::getTask() noexcept
{
    return task;
}

tf::Taskflow* SubTask::getFlow() noexcept
{
    if (graph == nullptr)
        return nullptr;

    return graph->getFlow();
}

void* SubTask::getGraph() noexcept
{
    return graph;
}

void SubTask::initialize(TaskGraph& g)
{
    this->graph = &g;

    task = g.getFlow()->emplace([this]()
    {
        callable(nullptr);
    });

    if (!name.assigned() || name.getLength() == 0)
    {
        task.name(std::to_string(id));
    }
    else
    {
        task.name(name);
    }

    g.addTask(thisInterface());
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    SubTask, ITask, createTask,
    IProcedure*, work,
    IString*, name
)

END_NAMESPACE_OPENDAQ
