set(TF_BUILD_TESTS    OFF CACHE BOOL "Enables TaskFlow tests")
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "Enables TaskFlow examples")
set(BUILD_TESTING     OFF CACHE BOOL "Enables TaskFlow CTest")

opendaq_dependency(
    NAME                Taskflow
    REQUIRED_VERSION    3.11.0
    GIT_REPOSITORY      https://github.com/taskflow/taskflow.git
    GIT_REF             v3.11.0
    ADD_FETCH_ALIAS     Taskflow::Taskflow=Taskflow
    EXPECT_TARGET       Taskflow::Taskflow
    PATCH_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0003-Ignore-False-Positive-Mem-Leak.patch
)

# Suppress C++20 extensions warning for Taskflow (it uses C++20 features but we compile with C++17)
if (TARGET Taskflow)
    if (CMAKE_COMPILER_IS_CLANGXX)
        target_compile_options(Taskflow INTERFACE -Wno-c++20-extensions)
    endif()
endif()