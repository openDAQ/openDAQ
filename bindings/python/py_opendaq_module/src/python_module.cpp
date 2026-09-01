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

#include <py_opendaq_module/python_module.h>
#include <py_opendaq_module/python_function_block.h>

#include <py_core_types/py_opendaq_daq.h>
#include <coretypes/version_info_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/exceptions.h>

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

PythonModule::PythonModule(const ContextPtr& context, const std::string& pathToPythonFile)
    : PythonModule(context, loadPlugin(context, pathToPythonFile))
{
}

PythonModule::PythonModule(const ContextPtr& context, PluginInfo&& info)
    : Module(info.name, info.version, context, info.id)
    , pyInstance(std::move(info.instance))
{
}

PythonModule::~PythonModule()
{
    // Drop the reference under the GIL, on the dispatch thread - never touch a py::object from
    // any other thread.
    PythonRuntime::instance().run([this] { pyInstance = py::object(); });
}

PythonModule::PluginInfo PythonModule::loadPlugin(const ContextPtr& context, const std::string& path)
{
    auto& runtime = PythonRuntime::instance();

    return runtime.run(
        [&runtime, &context, &path]() -> PluginInfo
        {
            py::module_ importlibUtil = py::module_::import("importlib.util");
            py::module_ sys = py::module_::import("sys");

            // Namespaced so two plugin files sharing a basename cannot collide in sys.modules.
            const std::string moduleName = runtime.nextPluginModuleName();

            py::object spec = importlibUtil.attr("spec_from_file_location")(moduleName, path);
            if (spec.is_none())
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Failed to load Python plugin '{}': no import spec could be created", path);

            py::object pyModule = importlibUtil.attr("module_from_spec")(spec);
            // Registered before exec so the plugin's own relative imports resolve correctly.
            sys.attr("modules")[py::str(moduleName)] = pyModule;
            spec.attr("loader").attr("exec_module")(pyModule);

            if (!py::hasattr(pyModule, "create_module"))
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Python plugin '{}' does not define create_module()", path);

            py::object pyContext = baseObjectToPyObject(context, IContext::Id, false);
            py::object instance = pyModule.attr("create_module")(pyContext);
            if (instance.is_none())
                DAQ_THROW_EXCEPTION(InvalidParameterException, "create_module() in '{}' did not return a module instance", path);

            PluginInfo info;
            info.name = instance.attr("name").is_none() ? StringPtr("") : StringPtr(py::cast<std::string>(instance.attr("name")));
            info.id = instance.attr("id").is_none() ? StringPtr("") : StringPtr(py::cast<std::string>(instance.attr("id")));
            info.version = toVersionInfo(instance.attr("version"));
            info.instance = std::move(instance);
            return info;
        });
}

VersionInfoPtr PythonModule::toVersionInfo(const py::object& pyVersion)
{
    if (pyVersion.is_none())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Python module must set a version");

    if (py::isinstance<py::tuple>(pyVersion) || py::isinstance<py::list>(pyVersion))
    {
        auto seq = py::cast<py::sequence>(pyVersion);
        if (seq.size() != 3)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Python module version tuple must have exactly 3 elements (major, minor, patch)");

        return VersionInfo(py::cast<SizeT>(seq[0]), py::cast<SizeT>(seq[1]), py::cast<SizeT>(seq[2]));
    }

    return pyObjectToBaseObject(pyVersion, false).asPtr<IVersionInfo>();
}

DictPtr<IString, IFunctionBlockType> PythonModule::onGetAvailableFunctionBlockTypes()
{
    return PythonRuntime::instance().run(
        [this]() -> DictPtr<IString, IFunctionBlockType>
        {
            py::object result = pyInstance.attr("on_get_available_function_block_types")();

            auto types = Dict<IString, IFunctionBlockType>();
            if (result.is_none())
                return types;

            for (auto item : py::cast<py::dict>(result))
            {
                const auto id = py::cast<std::string>(item.first);
                const auto type =
                    pyObjectToBaseObject(py::reinterpret_borrow<py::object>(item.second), false).asPtr<IFunctionBlockType>();
                types.set(id, type);
            }
            return types;
        });
}

FunctionBlockPtr PythonModule::onCreateFunctionBlock(const StringPtr& id,
                                                       const ComponentPtr& parent,
                                                       const StringPtr& localId,
                                                       const PropertyObjectPtr& config)
{
    return PythonRuntime::instance().run(
        [this, &id, &parent, &localId, &config]() -> FunctionBlockPtr
        {
            // The delegate is a plain instance of the plugin's own FunctionBlock subclass -
            // __init__ has already run, but it is not yet backed by a PythonFunctionBlock
            // (_cpp_fb is unset). createPythonFunctionBlock() does that wrapping.
            py::object pyDelegate = pyInstance.attr("on_create_function_block")(
                baseObjectToPyObject(id, IString::Id, false),
                baseObjectToPyObject(parent, IComponent::Id, false),
                baseObjectToPyObject(localId, IString::Id, false),
                config.assigned() ? baseObjectToPyObject(config, IPropertyObject::Id, false) : py::none());

            if (pyDelegate.is_none())
                DAQ_THROW_EXCEPTION(NotFoundException);

            return createPythonFunctionBlock(context, parent, localId, std::move(pyDelegate));
        });
}

END_NAMESPACE_OPENDAQ
