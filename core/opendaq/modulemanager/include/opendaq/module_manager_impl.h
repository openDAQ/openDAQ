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
#include <opendaq/module_manager_utils.h>
#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/device_info_ptr.h>
#include <coretypes/string_ptr.h>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

struct ModuleLibrary;

class ModuleManagerImpl : public ImplementationOfWeak<IModuleManager, IModuleManagerUtils>
{
public:
    explicit ModuleManagerImpl(const BaseObjectPtr& path);
    ~ModuleManagerImpl() override;

    ErrCode INTERFACE_FUNC getModules(IList** availableModules) override;
    ErrCode INTERFACE_FUNC addModule(IModule* module) override;
    ErrCode INTERFACE_FUNC loadModules(IContext* context) override;

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config = nullptr, IString* localId = nullptr) override;

private:
    bool modulesLoaded;
    std::vector<std::string> paths;
    std::vector<ModuleLibrary> libraries;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    DictPtr<IString, IDeviceInfo> availableDevicesGroup;
    std::unordered_map<std::string, size_t> functionBlockCountMap;
};

END_NAMESPACE_OPENDAQ
