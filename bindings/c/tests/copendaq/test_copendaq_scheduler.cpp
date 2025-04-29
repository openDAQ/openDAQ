#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqSchedulerTest = testing::Test;

static Bool taskCalled = False;
static Bool taskGraphCalled = False;
static Bool functionCalled = False;

ErrCode procedureTask(BaseObject*)
{
    printf("task called\n");
    taskCalled = true;
    return 0;
}

ErrCode procedureTaskGraph(BaseObject*)
{
    printf("taskGraph called\n");
    taskGraphCalled = true;
    return 0;
}

ErrCode functionCall(BaseObject*, BaseObject**)
{
    printf("function called\n");
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
    ASSERT_NE(scheduler, nullptr);

    Procedure* procGraph = nullptr;
    Procedure_createProcedure(&procGraph, procedureTaskGraph);

    String* nameGraph = nullptr;
    String_createString(&nameGraph, "taskGraph");

    TaskGraph* taskGraph = nullptr;
    TaskGraph_createTaskGraph(&taskGraph, procGraph, nameGraph);

    Task* task = nullptr;
    BaseObject_borrowInterface(taskGraph, TASK_INTF_ID, reinterpret_cast<void**>(&task));
    ASSERT_NE(task, nullptr);

    Procedure* taskProc = nullptr;
    Procedure_createProcedure(&taskProc, procedureTask);

    String* name = nullptr;
    String_createString(&name, "task");

    Task* task2 = nullptr;
    Task_createTask(&task2, taskProc, name);

    Task_then(task, task2);

    Awaitable* awaitable = nullptr;
    Scheduler_scheduleGraph(scheduler, taskGraph, &awaitable);
    ASSERT_NE(awaitable, nullptr);

    Function* function = nullptr;
    Function_createFunction(&function, functionCall);

    Awaitable* awaitable2 = nullptr;
    Scheduler_scheduleFunction(scheduler, function, &awaitable2);
    ASSERT_NE(awaitable2, nullptr);

    Scheduler_waitAll(scheduler);

    ASSERT_EQ(taskCalled, True);
    ASSERT_EQ(functionCalled, True);
    ASSERT_EQ(taskGraphCalled, True);

    BaseObject_releaseRef(awaitable2);
    BaseObject_releaseRef(function);

    BaseObject_releaseRef(awaitable);
    BaseObject_releaseRef(task2);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(taskProc);

    BaseObject_releaseRef(taskGraph);
    BaseObject_releaseRef(procGraph);
    BaseObject_releaseRef(nameGraph);

    BaseObject_releaseRef(scheduler);
    BaseObject_releaseRef(logger);
    BaseObject_releaseRef(sink);
    BaseObject_releaseRef(sinks);
}