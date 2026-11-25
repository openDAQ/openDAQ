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
#include <native_streaming_client_module/common.h>
#include <opendaq/add_device_config_ptr.h>
#include <opendaq/address_info_utils.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/listobject_factory.h>
#include <opendaq/transport_option_factory.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <string>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

/*!
 * Builds a list of reachable alternative addresses from the given server capability.
 *
 * Orders the addresses as `sortAddressInfos` does.
 * The primary address type is `PrimaryAddressType` of @p config when it is an add device config, and IPv4 otherwise.
 * Addresses with AddressReachabilityStatus::Unreachable are always excluded.
 *
 * @param config        Component config property object (may be nullptr).
 * @param deviceInfo    Device info to query the server capability from.
 * @param capabilityName  Protocol ID of the server capability to use.
 * @param outAddresses  Receives the ordered list of addresses on success.
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

    const AddDeviceConfigPtr addDeviceConfig = config.assigned() ? config.asPtrOrNull<IAddDeviceConfig>(true) : nullptr;
    const auto primaryAddressType = addDeviceConfig.assigned() ? addDeviceConfig.getPrimaryAddressType() : AddressType::IPv4;

    auto addresses = List<IString>();
    for (const auto& addressInfo : sortAddressInfos(addressInfos, primaryAddressType))
        if (addressInfo.getReachabilityStatus() != AddressReachabilityStatus::Unreachable)
            addresses.pushBack(addressInfo.getAddress());

    outAddresses = std::move(addresses);
    return OPENDAQ_SUCCESS;
}

/*!
 * Builds the message of the "Connected" connection status of a configuration or streaming channel,
 * which names the transport of the channel, e.g. "Transport: tls".
 *
 * @param transport  Name of the transport the channel is connected over.
 */
inline StringPtr buildConnectedStatusMessage(const std::string& transport)
{
    return String(fmt::format("Transport: {}", transport));
}

/*!
 * Builds the transport option of a native transport at a port: "tls" is encrypted, anything else ("plain")
 * unprotected. Neither needs a client certificate unless @p certificateRequired says so.
 *
 * @param transport  Name of the transport ("plain" or "tls").
 * @param port  The port the transport is served on.
 * @param certificateRequired  Whether the server asks for a client certificate.
 */
inline TransportOptionPtr NativeTransportOption(const StringPtr& transport, Int port, Bool certificateRequired = False)
{
    const auto protection = transport == opendaq_native_streaming_protocol::CONST_NATIVE_TRANSPORT_TLS ? SecurityMode::Encrypted
                                                                                                        : SecurityMode::None;
    return TransportOption(transport, port, protection, certificateRequired);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE