opendaq_dependency(
    NAME                pybind11
    REQUIRED_VERSION    3.0.1
    GIT_REPOSITORY      https://github.com/pybind/pybind11
    GIT_REF             v3.0.1
    EXPECT_COMMAND      pybind11_add_module
)
