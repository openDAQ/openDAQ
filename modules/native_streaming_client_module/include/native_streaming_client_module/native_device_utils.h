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
#include <native_streaming_client_module/common.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

/*!
 * Builds a list of reachable alternative addresses from the given server capability.
 *
 * Reads the optional "General.PrimaryAddressType" field from @p config to filter
 * addresses by type. Addresses with AddressReachabilityStatus::Unreachable are
 * always excluded.
 *
 * @param config        Component config property object (may be nullptr).
 * @param deviceInfo    Device info to query the server capability from.
 * @param capabilityName  Protocol ID of the server capability to use.
 * @param outAddresses  Receives the filtered list of addresses on success.
 *
 * @returns OPENDAQ_SUCCESS with @p outAddresses populated,
 *          OPENDAQ_IGNORED when there is nothing to update (capability absent or
 *          no address infos), or an error code on failure.
 */
inline ErrCode collectAlternativeAddresses(const PropertyObjectPtr& config,
                                           const DeviceInfoPtr& deviceInfo,
                                           const StringPtr& capabilityName,
                                           ListPtr<IString>& outAddresses)
{
    if (!deviceInfo.assigned())
        return OPENDAQ_IGNORED;

    if (!deviceInfo.hasServerCapability(capabilityName))
        return OPENDAQ_IGNORED;

    const auto capability = deviceInfo.getServerCapability(capabilityName);
    if (!capability.assigned())
        return OPENDAQ_IGNORED;

    const auto addressInfos = capability.getAddressInfo();
    if (!addressInfos.assigned() || addressInfos.getCount() < 2)
        return OPENDAQ_IGNORED;

    StringPtr primaryAddressType;
    if (config.assigned() && config.hasProperty("General"))
    {
        const PropertyObjectPtr generalConfig = config.getPropertyValue("General");
        if (generalConfig.hasProperty("PrimaryAddressType"))
        {
            const StringPtr candidate = generalConfig.getPropertyValue("PrimaryAddressType");
            if (candidate.getLength())
                primaryAddressType = candidate;
        }
    }

    auto addresses = List<IString>();
    for (const auto& addressInfo : addressInfos)
    {
        if (primaryAddressType.assigned() && addressInfo.getType() != primaryAddressType)
            continue;

        if (addressInfo.getReachabilityStatus() == AddressReachabilityStatus::Unreachable)
            continue;

        addresses.pushBack(addressInfo.getAddress());
    }

    outAddresses = std::move(addresses);
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE