set(BASE_NAME scheduler)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Scheduler scheduler.h)
rtgen(SRC_Awaitable awaitable.h)
rtgen(SRC_TaskGraph task_graph.h)
rtgen(SRC_Task task.h)

rtgen(SRC_GraphVisualization graph_visualization.h)

source_group("scheduler" FILES ${SDK_HEADERS_DIR}/scheduler.h
                               ${SDK_HEADERS_DIR}/scheduler_factory.h
                               ${SDK_HEADERS_DIR}/scheduler_impl.h
                               scheduler_impl.cpp
)

source_group("task" FILES ${SDK_HEADERS_DIR}/task.h
                          ${SDK_HEADERS_DIR}/task_graph.h
                          ${SDK_HEADERS_DIR}/task_impl.h
                          ${SDK_HEADERS_DIR}/task_factory.h
                          ${SDK_HEADERS_DIR}/task_ptr.custom.h
                          task_impl.cpp
)

source_group("awaitable" FILES ${SDK_HEADERS_DIR}/awaitable.h
                               ${SDK_HEADERS_DIR}/awaitable_impl.h
                               ${SDK_HEADERS_DIR}/awaitable_factory.h
                               awaitable_impl.cpp
)

set(SRC_Cpp scheduler_impl.cpp
            awaitable_impl.cpp
            task_graph_impl.cpp
            task_impl.cpp
)

set(SRC_PublicHeaders scheduler_factory.h
                      task_factory.h
                      scheduler_errors.h
                      scheduler_exceptions.h
                      task_ptr.custom.h
)

set(SRC_PrivateHeaders scheduler_impl.h
                       awaitable_impl.h

                       task_impl.h
                       task_internal.h
                       task_flow.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Scheduler_Cpp}
                    ${SRC_Task_Cpp}
                    ${SRC_Awaitable_Cpp}
                    ${SRC_Task_Cpp}
                    ${SRC_TaskGraph_Cpp}
                    ${SRC_GraphVisualization_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Scheduler_PublicHeaders}
                              ${SRC_Task_PublicHeaders}
                              ${SRC_Awaitable_PublicHeaders}
                              ${SRC_Task_PublicHeaders}
                              ${SRC_TaskGraph_PublicHeaders}
                              ${SRC_GraphVisualization_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Scheduler_PrivateHeaders}
                               ${SRC_Task_PrivateHeaders}
                               ${SRC_Awaitable_PrivateHeaders}
                               ${SRC_Task_PrivateHeaders}
                               ${SRC_TaskGraph_PrivateHeaders}
                               ${SRC_GraphVisualization_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coretypes
        daq::logger
    PRIVATE
        $<BUILD_INTERFACE:Taskflow::Taskflow>
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)

if (MSVC)
    opendaq_target_compile_options(${BASE_NAME} PRIVATE /wd4324)
endif()
