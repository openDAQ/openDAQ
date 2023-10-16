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
#include <opendaq/device_type_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_device_type
 * @addtogroup opendaq_device_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a DeviceTypePtr pointer, with the id, name, description and
 * optional createDefaultConfigCallback.
 * @param id The unique type ID of the device.
 * @param name The name of the device type.
 * @param description A short description of the device type.
 * @param createDefaultConfigCallback The callback, which is called, when user want to create
 * new default configuration object.
 * Function needs to create and return property object. On each call, we need to create new object,
 * because we want that each instance of the device has its own configuration object.
 */
inline DeviceTypePtr DeviceType(const StringPtr& id,
                                const StringPtr& name,
                                const StringPtr& description,
                                const FunctionPtr& createDefaultConfigCallback = nullptr)
{
    DeviceTypePtr obj(DeviceType_Create(id, name, description, createDefaultConfigCallback));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Device type struct.
 */
inline StructTypePtr DeviceTypeStructType()
{
    return StructType("deviceType",
                      List<IString>("id", "name", "description"),
                      List<IString>("", "", ""),
                      List<IType>(SimpleType(ctInt), SimpleType(ctString), SimpleType(ctString)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
