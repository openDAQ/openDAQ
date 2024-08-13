set(TF_BUILD_TESTS    OFF CACHE BOOL "Enables TaskFlow tests")
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "Enables TaskFlow examples")
set(BUILD_TESTING     OFF CACHE BOOL "Enables TaskFlow CTest")

opendaq_dependency(
    NAME                Taskflow
    REQUIRED_VERSION    3.4.0
    GIT_REPOSITORY      https://github.com/taskflow/taskflow.git
    GIT_REF             v3.4.0
    ADD_FETCH_ALIAS     Taskflow::Taskflow=Taskflow
    EXPECT_TARGET       Taskflow::Taskflow
    PATCH_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0001-TBBAS-280-Fix-Executor-async-functions-to-set-except.patch
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0002-Implement-fallback-uncaught-exception-handler.patch
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0003-Ignore-False-Positive-Mem-Leak.patch
)
