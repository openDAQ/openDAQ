/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coreobjects/unit.h>
#include <coretypes/ratio.h>
#include <opendaq/reference_domain_info.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device_domain Device domain
 * @{
 */

/*#
 * [interfaceLibrary(IUnit, CoreObjects)]
 */

/*!
 * @brief Contains information about the domain of the device.
 *
 * The device domain contains a general view into the device's domain data. While devices most often operate
 * in the time domain, this interface allows for description of any other domain commonly used in signal processing.
 * For example, common domains include the angle domain, frequency domain, the spatial domain, and the wavelet domain.
 *
 * The device domain allows for users to query a device for its current domain value via `getTicksSinceOrigin`
 * and convert that into its domain unit by multiplying the tick count with the resolution. To get the absolute
 * domain value, we can then also add the value to the Origin, which is most often provided
 * as a time epoch in the ISO 8601 format.
 *
 * Note that all devices might note provide a device domain implementation. Such devices cannot be directly queried
 * for their domain data. In such a case, the domain data can be obtained through the device's output signals.
 */
DECLARE_OPENDAQ_INTERFACE(IDeviceDomain, IBaseObject)
{
    /*!
     * @brief Gets domain (usually time) between two consecutive ticks. Resolution is provided in a domain unit.
     * @param[out] tickResolution The device's resolution.
     */
    virtual ErrCode INTERFACE_FUNC getTickResolution(IRatio** tickResolution) = 0;

    /*!
     * @brief Gets the device's absolute origin. Most often this is a time epoch in the ISO 8601 format.
     * @param[out] origin The origin.
     */
    virtual ErrCode INTERFACE_FUNC getOrigin(IString** origin) = 0;

    /*!
     * @brief Gets the domain unit (eg. seconds, hours, degrees...)
     * @param[out] unit The domain unit.
     */
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;

    /*!
     * @brief Gets the Reference Domain Info.
     * @param[out] referenceDomainInfo The Reference Domain Info.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceDomain,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, DeviceDomain, IDeviceDomain, createDeviceDomainWithReferenceDomainInfo,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit,
    IReferenceDomainInfo*, referenceDomainInfo
)

END_NAMESPACE_OPENDAQ
