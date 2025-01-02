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

#pragma once
#include <opendaq/module_manager_errors.h>
#include <coretypes/exceptions.h>

BEGIN_NAMESPACE_OPENDAQ

class ModuleManagerException : public DaqException
{
public:
    using DaqException::DaqException;
};

#define DEFINE_MODULE_MANAGER_EXCEPTION(excName, errCode, excMsg) DEFINE_EXCEPTION_BASE(daq::ModuleManagerException, excName, errCode, excMsg)

/*
 * Should be in the order of the error's numerical value
 */
DEFINE_MODULE_MANAGER_EXCEPTION(ModuleManagerUnknown, OPENDAQ_ERR_MODULE_MANAGER_UNKNOWN, "Unknown module-manager exception")
DEFINE_MODULE_MANAGER_EXCEPTION(ModuleLoadFailed, OPENDAQ_ERR_MODULE_LOAD_FAILED, "Module failed to load")
DEFINE_MODULE_MANAGER_EXCEPTION(ModuleNoEntryPoint, OPENDAQ_ERR_MODULE_NO_ENTRY_POINT, "Module has no entry-point function to call")
DEFINE_MODULE_MANAGER_EXCEPTION(ModuleEntryPointFailed, OPENDAQ_ERR_MODULE_ENTRY_POINT_FAILED, "Module entry-point function call failed")
DEFINE_MODULE_MANAGER_EXCEPTION(ModuleIncompatibleDependencies, OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES, "Module has incompatible dependencies")

END_NAMESPACE_OPENDAQ
