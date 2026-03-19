opendaq_dependency(
    NAME                pybind11
    REQUIRED_VERSION    3.0.1
    GIT_REPOSITORY      https://github.com/pybind/pybind11
    GIT_REF             v3.0.1
    EXPECT_COMMAND      pybind11_add_module
    PATCH_FILES         ${CMAKE_CURRENT_SOURCE_DIR}/patches/001-py14-clear-managed-dict.patch
)

# Patch was created after PR https://github.com/pybind/pybind11/pull/5999
# When bumping the version, check if PR was merged and remove the patch.
# Check if test_py_opendaq passes on python >= 3.14.
