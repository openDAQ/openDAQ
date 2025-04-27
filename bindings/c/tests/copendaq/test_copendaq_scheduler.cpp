#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqSchedulerTest = testing::Test;

static Bool taskGraphCalled = False;
static Bool taskCalled = False;
static Bool functionCalled = False;

ErrCode procedureTaskGraph(BaseObject*)
{
    taskGraphCalled = true;
    return 0;
}

ErrCode procedureTask(BaseObject*)
{
    taskCalled = true;
    return 0;
}

ErrCode functionCall(BaseObject*, BaseObject**)
{
    functionCalled = true;
    return 0;
}

TEST_F(COpendaqSchedulerTest, Scheduler)
{
    List* sinks = nullptr;
    List_createList(&sinks);

    LoggerSink* sink = nullptr;
    LoggerSink_createStdErrLoggerSink(&sink);
    List_pushBack(sinks, sink);

    Logger* logger = nullptr;
    Logger_createLogger(&logger, sinks, LogLevel::LogLevelDebug);

    Scheduler* scheduler = nullptr;
    Scheduler_createScheduler(&scheduler, logger, 1);

    Procedure* procGraph = nullptr;
    Procedure_createProcedure(&procGraph, procedureTaskGraph);

    Procedure* procTask = nullptr;
    Procedure_createProcedure(&procTask, procedureTask);

    String* nameGraph = nullptr;
    String_createString(&nameGraph, "taskGraph");

    String* nameTask = nullptr;
    String_createString(&nameTask, "task2");

    Task* task = nullptr;
    Task_createTask(&task, procGraph, nameGraph);

    Task* task2 = nullptr;
    Task_createTask(&task2, procTask, nameTask);

    Task_then(task, task2);

    TaskGraph* taskGraph = nullptr;
    BaseObject_borrowInterface(task, TASK_GRAPH_INTF_ID, reinterpret_cast<void**>(&taskGraph));

    Awaitable* awaitable = nullptr;
    Scheduler_scheduleGraph(scheduler, taskGraph, &awaitable);

    Awaitable_wait(awaitable);

    ASSERT_EQ(taskGraphCalled, True);
    ASSERT_EQ(taskCalled, True);

    Function* function = nullptr;
    Function_createFunction(&function, functionCall);

    Awaitable* awaitable2 = nullptr;
    Scheduler_scheduleFunction(scheduler, function, &awaitable2);

    Awaitable_wait(awaitable2);
    ASSERT_EQ(functionCalled, True);

    BaseObject_releaseRef(awaitable2);
    BaseObject_releaseRef(function);
    BaseObject_releaseRef(awaitable);
    BaseObject_releaseRef(task2);
    BaseObject_releaseRef(task);
    BaseObject_releaseRef(procTask);
    BaseObject_releaseRef(procGraph);
    BaseObject_releaseRef(scheduler);
    BaseObject_releaseRef(logger);
    BaseObject_releaseRef(sink);
    BaseObject_releaseRef(sinks);
    BaseObject_releaseRef(nameTask);
    BaseObject_releaseRef(nameGraph);
}