set(BASE_NAME scheduler)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Scheduler scheduler.h)
    rtgen(SRC_Awaitable awaitable.h)
    rtgen(SRC_TaskGraph task_graph.h)
    rtgen(SRC_Task task.h)
    rtgen(SRC_GraphVisualization graph_visualization.h)
    rtgen(SRC_Work work.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Scheduler_PublicHeaders}
        ${SRC_Task_PublicHeaders}
        ${SRC_Awaitable_PublicHeaders}
        ${SRC_Task_PublicHeaders}
        ${SRC_TaskGraph_PublicHeaders}
        ${SRC_GraphVisualization_PublicHeaders}
        ${SRC_Work_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Scheduler_PrivateHeaders}
        ${SRC_Task_PrivateHeaders}
        ${SRC_Awaitable_PrivateHeaders}
        ${SRC_Task_PrivateHeaders}
        ${SRC_TaskGraph_PrivateHeaders}
        ${SRC_GraphVisualization_PrivateHeaders}
        ${SRC_Work_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Scheduler_Cpp}
        ${SRC_Task_Cpp}
        ${SRC_Awaitable_Cpp}
        ${SRC_Task_Cpp}
        ${SRC_TaskGraph_Cpp}
        ${SRC_GraphVisualization_Cpp}
        ${SRC_Work_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "scheduler/include/opendaq")
    set(SDK_SRC_DIR "scheduler/src")
    
    source_group("scheduler//scheduler" FILES 
        ${SDK_HEADERS_DIR}/scheduler.h
        ${SDK_HEADERS_DIR}/scheduler_factory.h
        ${SDK_HEADERS_DIR}/scheduler_impl.h
        ${SDK_SRC_DIR}/scheduler_impl.cpp
    )
    
    source_group("scheduler//task" FILES 
        ${SDK_HEADERS_DIR}/task.h
        ${SDK_HEADERS_DIR}/task_graph.h
        ${SDK_HEADERS_DIR}/task_impl.h
        ${SDK_HEADERS_DIR}/task_factory.h
        ${SDK_HEADERS_DIR}/task_ptr.custom.h
        ${SDK_SRC_DIR}/task_impl.cpp
    )
    
    source_group("scheduler//awaitable" FILES 
        ${SDK_HEADERS_DIR}/awaitable.h
        ${SDK_HEADERS_DIR}/awaitable_impl.h
        ${SDK_HEADERS_DIR}/awaitable_factory.h
        ${SDK_SRC_DIR}/awaitable_impl.cpp
    )
    
    source_group("scheduler//work" FILES 
        ${SDK_HEADERS_DIR}/work.h
        ${SDK_HEADERS_DIR}/work_impl.h
        ${SDK_HEADERS_DIR}/work_factory.h
    )
endfunction()

set(SRC_PublicHeaders_Component 
    scheduler_factory.h
    task_factory.h
    scheduler_errors.h
    scheduler_exceptions.h
    task_ptr.custom.h
    work_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    scheduler_impl.h
    awaitable_impl.h
    task_impl.h
    task_internal.h
    task_flow.h
    work_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    scheduler_impl.cpp
    awaitable_impl.cpp
    task_graph_impl.cpp
    task_impl.cpp
    scheduler.natvis
    PARENT_SCOPE
)

set(SRC_PrivateLinkLibraries_Component
    $<BUILD_INTERFACE:Taskflow::Taskflow>
    PARENT_SCOPE
)
