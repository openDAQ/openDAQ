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
#include <opendaq/instance_builder_ptr.h>
#include <opendaq/instance_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_manager_factory.h>
#include <coretypes/type_manager_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_instance
 * @addtogroup opendaq_instance_factories Factories
 * @{
 */

/*!
 * @brief Creates a InstanceBuilder with no parameters configured.
 */
inline InstanceBuilderPtr InstanceBuilder()
{
    InstanceBuilderPtr obj(InstanceBuilder_Create());
    return obj;
}

/*!
 * @brief Creates a Instance with Builder
 * @param builder Instance Builder
 */
inline InstancePtr InstanceFromBuilder(const InstanceBuilderPtr& builder)
{
    InstancePtr obj(InstanceFromBuilder_Create(builder));
    return obj;
}

/*!
 * @brief Creates a new Instance, using the default logger and module manager. The module manager
 * searches for module shared libraries at the given module path, using the executable directory if left empty.
 * @param modulePath The module search path to be used by the Module manager.
 * @param localId The local id of the instance.
 *
 * If localId is empty, the local id will be set to the OPENDAQ_INSTANCE_ID environment variable if available. Otherwise
 * a random UUID will be generated for the local id.
 */
inline InstancePtr Instance(const std::string& modulePath = "", const std::string& localId = "")
{
    auto builder = InstanceBuilder().setModulePath(modulePath)
                                    .setDefaultRootDeviceLocalId(localId);
    return InstanceFromBuilder(builder);
}

/*!
 * @brief Creates a new Instance with the provided Context.
 * @param context The Context.
 * @param localId The instance local id.
 */
inline InstancePtr InstanceCustom(const ContextPtr& context,
                                  const StringPtr& localId)
{
    auto builder = InstanceBuilder().setContext(context)
                                    .setDefaultRootDeviceLocalId(localId);
    return InstanceFromBuilder(builder);
}

/*!
 * @brief Creates a Client implementation of a device.
 * @param context The Context to be used by the client.
 * @param localId The local id of the instance.
 * @param defaultDeviceInfo The DeviceInfo to be used by the client device.
 * @param parent The parent component of the client.
 *
 * When creating an Instance, it by default creates a Client and sets it as the root device.
 * The Client supports adding function blocks and connecting to devices. The array of function blocks
 * and supported devices depends on the modules loaded by the Module manager.
 */
inline DevicePtr Client(const ContextPtr& context, const StringPtr& localId, const DeviceInfoPtr& defaultDeviceInfo = nullptr, const ComponentPtr& parent = nullptr)
{
    DevicePtr obj(Client_Create(context, localId, defaultDeviceInfo, parent));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
