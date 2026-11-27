opendaq_dependency(
    NAME                pybind11
    REQUIRED_VERSION    3.1.0
    GIT_REPOSITORY      https://github.com/pybind/pybind11
    GIT_REF             v3.1.0
    EXPECT_COMMAND      pybind11_add_module
)
