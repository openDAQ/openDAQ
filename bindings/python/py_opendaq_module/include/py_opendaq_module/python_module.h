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

#pragma once
#include <opendaq/module_impl.h>
#include <py_opendaq_module/python_runtime.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief A Module backed by a Python file that defines a `create_module(context)` entry point
 * returning a `Module` subclass instance (see the Python-side base classes this mirrors).
 *
 * Discovery is explicit only: `PythonModule(context, path)` loads exactly one plugin file.
 * There is no folder scanning or manifest.
 *
 * Every call into the Python instance - including the initial load in the constructor - is
 * marshaled through PythonRuntime::run(), so it always executes on PythonRuntime's single
 * dispatch thread. `pyInstance` must only ever be touched from within a run() callable.
 */
class PythonModule final : public Module
{
public:
    PythonModule(const ContextPtr& context, const std::string& pathToPythonFile);
    ~PythonModule() override;

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id,
                                            const ComponentPtr& parent,
                                            const StringPtr& localId,
                                            const PropertyObjectPtr& config) override;

private:
    struct PluginInfo
    {
        StringPtr name;
        VersionInfoPtr version;
        StringPtr id;
        pybind11::object instance;
    };

    static PluginInfo loadPlugin(const ContextPtr& context, const std::string& path);
    static VersionInfoPtr toVersionInfo(const pybind11::object& pyVersion);

    PythonModule(const ContextPtr& context, PluginInfo&& info);

    pybind11::object pyInstance;
};

END_NAMESPACE_OPENDAQ
