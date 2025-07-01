#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqSchedulerTest = testing::Test;

static daqBool taskCalled = False;
static daqBool taskGraphCalled = False;
static daqBool functionCalled = False;

daqErrCode procedureTask(daqBaseObject*)
{
    printf("task called\n");
    taskCalled = true;
    return 0;
}

daqErrCode procedureTaskGraph(daqBaseObject*)
{
    printf("taskGraph called\n");
    taskGraphCalled = true;
    return 0;
}

daqErrCode functionCall(daqBaseObject*, daqBaseObject**)
{
    printf("function called\n");
    functionCalled = true;
    return 0;
}

TEST_F(COpendaqSchedulerTest, Scheduler)
{
    daqList* sinks = nullptr;
    daqList_createList(&sinks);

    daqLoggerSink* sink = nullptr;
    daqLoggerSink_createStdErrLoggerSink(&sink);
    daqList_pushBack(sinks, sink);

    daqLogger* logger = nullptr;
    daqLogger_createLogger(&logger, sinks, daqLogLevel::daqLogLevelDebug);

    daqScheduler* scheduler = nullptr;
    daqScheduler_createScheduler(&scheduler, logger);
    ASSERT_NE(scheduler, nullptr);

    daqProcedure* procGraph = nullptr;
    daqProcedure_createProcedure(&procGraph, procedureTaskGraph);

    daqString* nameGraph = nullptr;
    daqString_createString(&nameGraph, "taskGraph");

    daqTaskGraph* taskGraph = nullptr;
    daqTaskGraph_createTaskGraph(&taskGraph, procGraph, nameGraph);

    daqTask* task = nullptr;
    daqBaseObject_borrowInterface(taskGraph, DAQ_TASK_INTF_ID, (void**) &task);
    ASSERT_NE(task, nullptr);

    daqProcedure* taskProc = nullptr;
    daqProcedure_createProcedure(&taskProc, procedureTask);

    daqString* name = nullptr;
    daqString_createString(&name, "task");

    daqTask* task2 = nullptr;
    daqTask_createTask(&task2, taskProc, name);

    daqTask_then(task, task2);

    daqAwaitable* awaitable = nullptr;
    daqScheduler_scheduleGraph(scheduler, taskGraph, &awaitable);
    ASSERT_NE(awaitable, nullptr);

    daqFunction* function = nullptr;
    daqFunction_createFunction(&function, functionCall);

    daqAwaitable* awaitable2 = nullptr;
    daqScheduler_scheduleFunction(scheduler, function, &awaitable2);
    ASSERT_NE(awaitable2, nullptr);

    daqScheduler_waitAll(scheduler);

    ASSERT_EQ(taskCalled, True);
    ASSERT_EQ(functionCalled, True);
    ASSERT_EQ(taskGraphCalled, True);

    daqBaseObject_releaseRef(awaitable2);
    daqBaseObject_releaseRef(function);

    daqBaseObject_releaseRef(awaitable);
    daqBaseObject_releaseRef(task2);
    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(taskProc);

    daqBaseObject_releaseRef(taskGraph);
    daqBaseObject_releaseRef(procGraph);
    daqBaseObject_releaseRef(nameGraph);

    daqBaseObject_releaseRef(scheduler);
    daqBaseObject_releaseRef(logger);
    daqBaseObject_releaseRef(sink);
    daqBaseObject_releaseRef(sinks);
}