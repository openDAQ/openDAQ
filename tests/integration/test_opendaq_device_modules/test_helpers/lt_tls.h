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

// Helpers for connecting the LT streaming modules over the secure (TLS) channel
// The legacy LT streaming modules have no TLS support
#ifndef DAQMODULES_LT_LEGACY_MODULES

#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers::lt_tls
{
    inline constexpr const char* ServerTypeId = "OpenDAQLTStreaming";
    inline constexpr const char* SecureDeviceTypeId = "OpenDAQLTStreamingSecure";

    inline constexpr const char* CaCert = "secrets/ca.crt";
    inline constexpr const char* ServerCert = "secrets/server.crt";
    inline constexpr const char* ServerKey = "secrets/server.key";
    inline constexpr const char* ClientCert = "secrets/client.crt";
    inline constexpr const char* ClientKey = "secrets/client.key";

    [[maybe_unused]]
    inline PropertyObjectPtr secureServerConfig(const InstancePtr& serverInstance)
    {
        auto config = serverInstance.getAvailableServerTypes().get(ServerTypeId).createDefaultConfig();
        config.setPropertyValue("EnableTlsStreamingPort", true);
        config.setPropertyValue("EnableMutualTls", true);
        config.setPropertyValue("CertificateFilePath", ServerCert);
        config.setPropertyValue("KeyFilePath", ServerKey);
        config.setPropertyValue("CaCertificateFilePath", CaCert);
        return config;
    }

    [[maybe_unused]]
    inline PropertyObjectPtr secureDeviceConfig(const InstancePtr& clientInstance)
    {
        auto config = clientInstance.getAvailableDeviceTypes().get(SecureDeviceTypeId).createDefaultConfig();
        config.setPropertyValue("EnableMutualTls", true);
        config.setPropertyValue("CertificateFilePath", ClientCert);
        config.setPropertyValue("KeyFilePath", ClientKey);
        config.setPropertyValue("CaCertificateFilePath", CaCert);
        return config;
    }

    // The same values as secureDeviceConfig(), applied to the secure device type section of a general
    // add-device configuration (the object returned by createDefaultAddDeviceConfig())
    [[maybe_unused]]
    inline void applySecureDeviceConfig(const PropertyObjectPtr& addDeviceConfig)
    {
        const auto prefix = std::string("Device.") + SecureDeviceTypeId + ".";
        addDeviceConfig.setPropertyValue(prefix + "EnableMutualTls", true);
        addDeviceConfig.setPropertyValue(prefix + "CertificateFilePath", ClientCert);
        addDeviceConfig.setPropertyValue(prefix + "KeyFilePath", ClientKey);
        addDeviceConfig.setPropertyValue(prefix + "CaCertificateFilePath", CaCert);
    }
}

END_NAMESPACE_OPENDAQ

#endif
