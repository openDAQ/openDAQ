/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/module_manager.h>
#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <coretypes/string_ptr.h>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

struct ModuleLibrary;

class ModuleManagerImpl : public ImplementationOfWeak<IModuleManager>
{
public:
    explicit ModuleManagerImpl(const BaseObjectPtr& path);
    ~ModuleManagerImpl() override;

    ErrCode INTERFACE_FUNC getModules(IList** availableModules) override;
    ErrCode INTERFACE_FUNC addModule(IModule* module) override;
    ErrCode INTERFACE_FUNC loadModules(IContext* context) override;

private:
    bool modulesLoaded;
    std::vector<std::string> paths;
    std::vector<ModuleLibrary> libraries;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_OPENDAQ
