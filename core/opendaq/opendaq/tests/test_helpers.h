/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/instance_factory.h>
#include <opendaq/instance_ptr.h>
#include <opendaq/device_ptr.h>
#include "opendaq/mock/mock_device_module.h"
#include "opendaq/mock/mock_fb_module.h"
#include "opendaq/mock/mock_physical_device.h"
#include "opendaq/mock/mock_server_module.h"

namespace test_helpers
{
    using namespace daq;

    inline InstancePtr setupInstance(const StringPtr& localId = nullptr)
    {
        const auto logger = Logger();
        const auto moduleManager = ModuleManager("[[none]]");
        const auto typeManager = TypeManager();
        const auto context = Context(nullptr, logger, typeManager, moduleManager);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        const ModulePtr serverModule(MockServerModule_Create(context, moduleManager));
        moduleManager.addModule(serverModule);

        auto instance = InstanceCustom(context, localId);
        return instance;
    }
}
