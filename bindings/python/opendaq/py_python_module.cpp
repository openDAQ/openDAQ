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
#include "py_core_objects/py_variant_extractor.h"

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

class PythonModuleImpl : public Module
{
public:
    PythonModuleImpl(const ContextPtr& context,
                     const StringPtr& name,
                     const VersionInfoPtr& version,
                     const StringPtr& id,
                     py::object pyModule)
        : Module(name, version, context, id)
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
                    fbType->addRef();
                    dict.set(key, FunctionBlockTypePtr::Borrow(fbType));
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
        py::gil_scoped_acquire acquire;
        if (!py::hasattr(pyModule, "on_create_function_block"))
            return nullptr;

        py::object result = pyModule.attr("on_create_function_block")(
            py::cast(id),
            py::cast(parent, py::return_value_policy::reference),
            py::cast(localId),
            config.assigned() ? py::cast(config, py::return_value_policy::reference) : py::none());

        if (result.is_none())
            DAQ_THROW_EXCEPTION(NotFoundException);

        try
        {
            auto* raw = result.cast<daq::IFunctionBlock*>();
            if (raw)
            {
                raw->addRef();
                return FunctionBlockPtr::Borrow(raw);
            }
        }
        catch (const py::cast_error&)
        {
            return nullptr;
        }
        return nullptr;
    }

private:
    py::object pyModule;
};

END_NAMESPACE_OPENDAQ

void addPythonModuleToManager(daq::IModuleManager* manager,
                                     daq::IContext* context,
                                     py::object pyModule)
{
    if (!context || !manager)
        throw std::invalid_argument("module_manager and context must not be null");

    py::object nameObj = pyModule.attr("name");
    py::object versionObj = pyModule.attr("version");
    py::object idObj = pyModule.attr("id");

    std::string name = nameObj.cast<std::string>();
    std::string idStr = idObj.cast<std::string>();

    daq::VersionInfoPtr version;
    if (py::isinstance<daq::IVersionInfo>(versionObj))
    {
        version = versionObj.cast<daq::IVersionInfo*>();
    }
    else
    {
        py::tuple t = versionObj.cast<py::tuple>();
        if (t.size() >= 3)
        {
            version = daq::VersionInfo(
                t[0].cast<daq::SizeT>(),
                t[1].cast<daq::SizeT>(),
                t[2].cast<daq::SizeT>());
        }
        else
        {
            version = daq::VersionInfo(1, 0, 0);
        }
    }

    auto modulePtr = daq::createWithImplementation<daq::IModule, daq::PythonModuleImpl>(
        daq::ContextPtr::Borrow(context),
        daq::String(name),
        version,
        daq::String(idStr),
        std::move(pyModule));

    // Module manager's addModule() does not addRef – it only stores the pointer. We must addRef
    // so the module outlives this function; the manager and any list returned by getModules() hold it.
    (*modulePtr)->addRef();

    py::gil_scoped_release release;
    daq::ModuleManagerPtr::Borrow(manager).addModule(*modulePtr);
}
