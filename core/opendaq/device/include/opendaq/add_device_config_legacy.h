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
#include <coretypes/common.h>
#include <coretypes/listobject_factory.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace LegacyAddDeviceConfig
{
inline constexpr const char* DeviceSection = "Device";
inline constexpr const char* StreamingSection = "Streaming";
inline constexpr const char* GeneralSection = "General";

inline constexpr const char* StreamingConnectionHeuristic = "StreamingConnectionHeuristic";
inline constexpr const char* MinConnections = "MinConnections";
inline constexpr const char* MinHops = "MinHops";
inline constexpr const char* NotConnected = "NotConnected";

inline constexpr const char* PrioritizedStreamingProtocols = "PrioritizedStreamingProtocols";
inline constexpr const char* AllowedStreamingProtocols = "AllowedStreamingProtocols";
inline constexpr const char* AutomaticallyConnectStreaming = "AutomaticallyConnectStreaming";
inline constexpr const char* ScanOnAdd = "ScanOnAdd";
inline constexpr const char* Username = "Username";
inline constexpr const char* Password = "Password";
inline constexpr const char* ClientType = "ClientType";
inline constexpr const char* ExclusiveControlDropOthers = "ExclusiveControlDropOthers";
inline constexpr const char* PrimaryAddressType = "PrimaryAddressType";
inline constexpr const char* IPv4 = "IPv4";
inline constexpr const char* IPv6 = "IPv6";

inline constexpr const char* NativeStreamingProtocolId = "OpenDAQNativeStreaming";
inline constexpr const char* LtStreamingProtocolId = "OpenDAQLTStreaming";

inline ListPtr<IString> defaultPrioritizedStreamingProtocols()
{
    return List<IString>(NativeStreamingProtocolId, LtStreamingProtocolId);
}
}

/*!
 * @brief Creates the legacy `General` section with its default values.
 * @param[out] general The `General` section
 * @param scanOnAdd The default of `ScanOnAdd`
 */
extern "C" PUBLIC_EXPORT ErrCode daqCreateLegacyGeneralConfig(IPropertyObject** general, Bool scanOnAdd);

/*!
 * @brief Creates the legacy `General` section with its default values.
 * @param scanOnAdd The default of `ScanOnAdd`
 */
inline PropertyObjectPtr createLegacyGeneralConfig(Bool scanOnAdd)
{
    PropertyObjectPtr general;
    checkErrorInfo(daqCreateLegacyGeneralConfig(&general, scanOnAdd));
    return general;
}

END_NAMESPACE_OPENDAQ
