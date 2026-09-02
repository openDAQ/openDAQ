/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <py_opendaq_module/python_module.h>
#include <py_opendaq_module/python_runtime.h>
#include <opendaq/context_factory.h>

namespace py = pybind11;
using namespace daq;

class PythonModuleTest : public testing::Test
{
protected:
    static ModulePtr loadMock(const std::string& fileName)
    {
        return createPythonModule(NullContext(), std::string(MOCK_MODULE_DIR) + "/" + fileName);
    }

    // Builds a MockModule instance directly, the way a Python host that already has the
    // instance in hand (rather than a file to load) would - see createPythonModule(context,
    // instance). Must be called with the GIL not held on the calling thread. `context` is
    // stored as-is by Module.__init__ and never used by it, so a real conversion to a Python
    // IContext isn't needed here - py::none() stands in for it.
    static py::object buildMockInstance()
    {
        return PythonRuntime::instance().run(
            []() -> py::object
            {
                py::dict globals;
                globals["opendaq"] = py::module_::import("opendaq");
                py::exec(R"(
class MockModule(opendaq.Module):
    def __init__(self, context):
        super().__init__(context, name="MockPythonModule", version=(1, 2, 3), id="MockPythonModuleId")
)",
                          globals);

                return globals["MockModule"](py::none());
            });
    }
};

TEST_F(PythonModuleTest, LoadsModuleInfo)
{
    const auto module = loadMock("mock_module.py");
    const auto info = module.getModuleInfo();

    ASSERT_EQ(info.getName(), "MockPythonModule");
    ASSERT_EQ(info.getId(), "MockPythonModuleId");

    const auto version = info.getVersionInfo();
    ASSERT_EQ(version.getMajor(), 1u);
    ASSERT_EQ(version.getMinor(), 2u);
    ASSERT_EQ(version.getPatch(), 3u);
}

TEST_F(PythonModuleTest, EmptyFunctionBlockTypesByDefault)
{
    const auto module = loadMock("mock_module.py");
    const auto types = module.getAvailableFunctionBlockTypes();

    ASSERT_TRUE(types.assigned());
    ASSERT_EQ(types.getCount(), 0u);
}

TEST_F(PythonModuleTest, CreateFunctionBlockThrowsWhenUnsupported)
{
    const auto module = loadMock("mock_module.py");
    ASSERT_THROW(module.createFunctionBlock("someId", nullptr, "localId"), NotFoundException);
}

TEST_F(PythonModuleTest, ThrowsWhenPluginHasNoCreateModule)
{
    ASSERT_THROW(loadMock("mock_module_no_create.py"), InvalidParameterException);
}

TEST_F(PythonModuleTest, ThrowsWhenPluginHasNoVersion)
{
    ASSERT_THROW(loadMock("mock_module_no_version.py"), InvalidParameterException);
}

TEST_F(PythonModuleTest, ThrowsWhenPluginFileDoesNotExist)
{
    ASSERT_ANY_THROW(loadMock("does_not_exist.py"));
}

TEST_F(PythonModuleTest, WrapsAlreadyConstructedInstance)
{
    py::object instance = buildMockInstance();
    const ModulePtr module = createPythonModule(NullContext(), std::move(instance));
    const auto info = module.getModuleInfo();

    ASSERT_EQ(info.getName(), "MockPythonModule");
    ASSERT_EQ(info.getId(), "MockPythonModuleId");

    const auto version = info.getVersionInfo();
    ASSERT_EQ(version.getMajor(), 1u);
    ASSERT_EQ(version.getMinor(), 2u);
    ASSERT_EQ(version.getPatch(), 3u);
}

TEST_F(PythonModuleTest, ThrowsWhenWrappedInstanceIsNone)
{
    // py::none() must itself be built under the GIL (via run()), same as any other py::object -
    // constructing it directly on this thread would trip pybind11's GIL-held assertion.
    py::object none = PythonRuntime::instance().run([]() -> py::object { return py::none(); });
    ASSERT_THROW(createPythonModule(NullContext(), std::move(none)), InvalidParameterException);
}
