/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <pybind11/pybind11.h>
#include <pybind11/gil.h>

#include <opendaq/module_impl.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_ptr.h>
#include <coretypes/version_info_factory.h>
#include <coretypes/objectptr.h>

#include "py_opendaq/py_opendaq.h"
#include "py_opendaq/py_python_function_block.h"

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

class PythonModuleImpl : public Module
{
public:
    PythonModuleImpl(const ContextPtr& context, py::object pyModule)
        : Module(extractName(pyModule), extractVersion(pyModule), context, extractId(pyModule))
        , pyModule(std::move(pyModule))
    {
    }

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override
    {
        py::gil_scoped_acquire acquire;
        if (!py::hasattr(pyModule, "on_get_available_function_block_types"))
            return Dict<IString, IFunctionBlockType>();

        py::object result = pyModule.attr("on_get_available_function_block_types")();
        if (result.is_none())
            return Dict<IString, IFunctionBlockType>();

        auto dict = Dict<IString, IFunctionBlockType>();
        py::object items = result.attr("items")();
        for (auto item : items)
        {
            py::tuple pair = item.cast<py::tuple>();
            py::object keyObj = pair[0];
            py::object valObj = pair[1];
            auto key = String(keyObj.cast<std::string>());
            try
            {
                auto* fbType = valObj.cast<daq::IFunctionBlockType*>();
                if (fbType)
                {
                    dict.set(key, fbType);
                }
            }
            catch (const py::cast_error&)
            {
                continue;
            }
        }
        return dict;
    }

    FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& config) override
    {
        const auto types = onGetAvailableFunctionBlockTypes();
        if (!types.hasKey(id))
            DAQ_THROW_EXCEPTION(NotFoundException, "Function block type not found: {}", id);

        py::gil_scoped_acquire acquire;
        if (!py::hasattr(pyModule, "on_create_function_block"))
            DAQ_THROW_EXCEPTION(NotFoundException, "Module does not implement on_create_function_block");

        // Python API: use plain strings + raw interface pointers (pybind can't convert SmartPtr wrappers).
        auto* rawParent = parent.assigned() ? parent.addRefAndReturn() : nullptr;

        py::object configArg = py::none();
        if (config.assigned())
        {
            auto* rawConfig = config.addRefAndReturn();
            configArg = py::cast(rawConfig, py::return_value_policy::reference);
        }

        py::object result = pyModule.attr("on_create_function_block")(
            py::cast(static_cast<std::string>(id)),
            rawParent ? py::cast(rawParent, py::return_value_policy::reference) : py::none(),
            py::cast(static_cast<std::string>(localId)),
            configArg);

        if (result.is_none())
            DAQ_THROW_EXCEPTION(NotFoundException, "Function block type not found: {}", id);

        const auto type = types.get(id);
        checkErrorInfo(type.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));

        return createPythonFunctionBlock(type, context, parent, localId, config, result);
    }

private:
    static py::object requireAttr(const py::object& module, const char* name)
    {
        py::gil_scoped_acquire acquire;
        if (!py::hasattr(module, name))
            throw std::invalid_argument(std::string("Python module must have attribute '") + name + "'");
        return module.attr(name);
    }

    static StringPtr extractName(const py::object& module)
    {
        py::object nameObj = requireAttr(module, "name");
        return String(nameObj.cast<std::string>());
    }

    static StringPtr extractId(const py::object& module)
    {
        py::object idObj = requireAttr(module, "id");
        return String(idObj.cast<std::string>());
    }

    static VersionInfoPtr extractVersion(const py::object& module)
    {
        py::object versionObj = requireAttr(module, "version");

        if (py::isinstance<daq::IVersionInfo>(versionObj))
            return versionObj.cast<daq::IVersionInfo*>();

        py::tuple t = versionObj.cast<py::tuple>();
        if (t.size() >= 3)
        {
            return daq::VersionInfo(
                t[0].cast<daq::SizeT>(),
                t[1].cast<daq::SizeT>(),
                t[2].cast<daq::SizeT>());
        }
        return daq::VersionInfo(1, 0, 0);
    }

    py::object pyModule;
};

END_NAMESPACE_OPENDAQ

void addPythonModuleToManager(daq::IModuleManager* manager,
                                     daq::IContext* context,
                                     py::object pyModule)
{
    if (!context || !manager)
        throw std::invalid_argument("module_manager and context must not be null");

    auto modulePtr = daq::createWithImplementation<daq::IModule, daq::PythonModuleImpl>(
        daq::ContextPtr::Borrow(context),
        std::move(pyModule));

    // Module manager's addModule() does not addRef – it only stores the pointer. We must addRef
    // so the module outlives this function; the manager and any list returned by getModules() hold it.
    (*modulePtr)->addRef();

    py::gil_scoped_release release;
    daq::ModuleManagerPtr::Borrow(manager).addModule(*modulePtr);
}
