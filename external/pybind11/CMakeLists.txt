opendaq_dependency(
    NAME                pybind11
    REQUIRED_VERSION    2.10.4
    GIT_REPOSITORY      https://github.com/pybind/pybind11
    GIT_REF             v2.10.4
    EXPECT_COMMAND      pybind11_add_module
)
