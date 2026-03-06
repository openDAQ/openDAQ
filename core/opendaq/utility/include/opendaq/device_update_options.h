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
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_device_options DeviceOptions
 * @{
 */
    
enum class DeviceUpdateMode : EnumType
{
    Load = 0,
    UpdateOnly,
    Skip,
    Remove,
    Remap
};

DECLARE_OPENDAQ_INTERFACE(IDeviceUpdateOptions, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getLocalId(IString** localId) = 0;
    virtual ErrCode INTERFACE_FUNC setLocalId(IString* localId) = 0;

    virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;
    virtual ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) = 0;

    virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;
    virtual ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) = 0;

    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;
    virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

    virtual ErrCode INTERFACE_FUNC getUpdateMode(DeviceUpdateMode* mode) = 0;
    virtual ErrCode INTERFACE_FUNC setUpdateMode(DeviceUpdateMode mode) = 0;

    // [elementType(childDeviceOptions, IDeviceUpdateOptions)]
    virtual ErrCode INTERFACE_FUNC getChildDeviceOptions(IList** childDeviceOptions) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceUpdateOptions, IString*, setupString);

END_NAMESPACE_OPENDAQ
