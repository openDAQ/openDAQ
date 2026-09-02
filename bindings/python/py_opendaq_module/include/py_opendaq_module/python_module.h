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
 * @brief A Module backed by a `create_module(context)`-shaped `Module` subclass instance (see
 * the Python-side base classes this mirrors) - either loaded from a plugin file, or handed in
 * already constructed by a caller that lives in the same interpreter.
 *
 * Two ways to build one:
 *  - `PythonModule(context, path)` loads exactly one plugin file (via `create_module(context)`
 *    inside it). There is no folder scanning or manifest.
 *  - `PythonModule(context, instance)` wraps an instance the caller already built - e.g. a
 *    Python process that is itself the host, using the real pip package rather than loading a
 *    plugin file into PythonRuntime's embedded interpreter.
 *
 * Every call into the Python instance - including the initial load in the constructor - is
 * marshaled through PythonRuntime::run(), so it always executes on PythonRuntime's single
 * dispatch thread. `pyInstance` must only ever be touched from within a run() callable. That
 * also means the calling thread must not already hold the GIL when constructing a PythonModule
 * (e.g. from inside a pybind11 `.def()` without `gil_scoped_release`) - PythonRuntime::run()
 * blocks the caller until the dispatch thread acquires the GIL, which it never can if the
 * caller is holding it.
 */
class PythonModule final : public Module
{
public:
    PythonModule(const ContextPtr& context, const std::string& pathToPythonFile);
    PythonModule(const ContextPtr& context, pybind11::object instance);
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
    static PluginInfo wrapInstance(pybind11::object instance);

    // Reads name/version/id off an already-constructed instance into a PluginInfo. The GIL
    // must already be held - call only from within a PythonRuntime::run() callable (both
    // loadPlugin() and wrapInstance() do this themselves).
    static PluginInfo fromInstance(pybind11::object instance);

    static VersionInfoPtr toVersionInfo(const pybind11::object& pyVersion);

    PythonModule(const ContextPtr& context, PluginInfo&& info);

    pybind11::object pyInstance;
};

/*!
 * @brief Loads the Python plugin at `pathToPythonFile` and wraps it in a PythonModule.
 */
ModulePtr createPythonModule(const ContextPtr& context, const std::string& pathToPythonFile);

/*!
 * @brief Wraps an already-constructed `Module`-shaped instance in a PythonModule. See the
 * PythonModule class comment for the GIL caveat.
 */
ModulePtr createPythonModule(const ContextPtr& context, pybind11::object instance);

END_NAMESPACE_OPENDAQ
