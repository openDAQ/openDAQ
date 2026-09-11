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

#ifdef OPENDAQ_ENABLE_NATIVE_STREAMING_WITH_TLS

#include <native_streaming_protocol/native_streaming_constants.h>
#include <opendaq/opendaq.h>

#include <string>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers::native_tls
{
    using namespace daq::opendaq_native_streaming_protocol;

    inline constexpr const char* ServerTypeId = CONST_NATIVE_SERVER_TYPE_ID;

    inline constexpr const char* PlainStreamingId = CONST_NATIVE_STREAMING_ID;
    inline constexpr const char* PlainConfigId = CONST_NATIVE_CONFIG_ID;
    // the pseudo-device and the streaming of a channel share their protocol id
    inline constexpr const char* SecureStreamingId = CONST_NATIVE_STREAMING_SECURE_ID;
    inline constexpr const char* SecureConfigId = CONST_NATIVE_CONFIG_SECURE_ID;

    inline constexpr uint16_t PlainPort = DEFAULT_PORT;
    inline constexpr uint16_t TlsPort = DEFAULT_TLS_PORT;

    // kept apart from the LT fixtures: the file names coincide, the authorities do not
    inline constexpr const char* CaCert = "native_secrets/ca.crt";
    inline constexpr const char* OtherCaCert = "native_secrets/other-ca.crt";
    inline constexpr const char* ServerCert = "native_secrets/server.crt";
    inline constexpr const char* ServerKey = "native_secrets/server.key";
    inline constexpr const char* ClientCert = "native_secrets/client.crt";
    inline constexpr const char* ClientKey = "native_secrets/client.key";

    /// @brief server configuration with the TLS listener on
    /// @param mutualTls requires clients to present a certificate signed by CaCert
    /// @param plainPortEnabled keeps the plaintext listener alongside; false serves TLS only
    [[maybe_unused]]
    inline PropertyObjectPtr secureServerConfig(const InstancePtr& serverInstance,
                                                bool mutualTls = false,
                                                bool plainPortEnabled = true)
    {
        auto config = serverInstance.getAvailableServerTypes().get(ServerTypeId).createDefaultConfig();
        config.setPropertyValue("EnablePort", plainPortEnabled);
        config.setPropertyValue("EnableTlsPort", True);
        config.setPropertyValue("CertificateFilePath", ServerCert);
        config.setPropertyValue("KeyFilePath", ServerKey);
        config.setPropertyValue("EnableMutualTls", mutualTls);
        if (mutualTls)
            config.setPropertyValue("CaCertificateFilePath", CaCert);
        return config;
    }

    /// @brief client configuration which authenticates the server against the given authority
    /// @param deviceTypeId which of the secure types to configure, device or streaming
    /// @param mutualTls presents ClientCert / ClientKey to a server which asks for one
    /// @param caCert the authority to verify the server against
    [[maybe_unused]]
    inline PropertyObjectPtr secureClientConfig(const InstancePtr& clientInstance,
                                                const char* deviceTypeId,
                                                bool mutualTls = false,
                                                const char* caCert = CaCert)
    {
        auto config = clientInstance.getAvailableDeviceTypes().get(deviceTypeId).createDefaultConfig();
        config.setPropertyValue("CaCertificateFilePath", caCert);
        config.setPropertyValue("EnableMutualTls", mutualTls);
        if (mutualTls)
        {
            config.setPropertyValue("CertificateFilePath", ClientCert);
            config.setPropertyValue("KeyFilePath", ClientKey);
        }
        return config;
    }

    /// @brief client configuration which encrypts without authenticating the server
    [[maybe_unused]]
    inline PropertyObjectPtr unverifiedClientConfig(const InstancePtr& clientInstance, const char* deviceTypeId)
    {
        auto config = clientInstance.getAvailableDeviceTypes().get(deviceTypeId).createDefaultConfig();
        config.setPropertyValue("VerifyServerCertificate", False);
        return config;
    }

    /// @brief the same settings applied to the secure sections of a general add-device configuration,
    /// which is how a streaming connection made from a server capability is configured
    [[maybe_unused]]
    inline void applySecureClientConfig(const PropertyObjectPtr& addDeviceConfig,
                                        bool mutualTls = false,
                                        const char* caCert = CaCert)
    {
        for (const auto& prefix : {std::string("Device.") + SecureConfigId + ".",
                                   std::string("Device.") + SecureStreamingId + ".",
                                   std::string("Streaming.") + SecureStreamingId + "."})
        {
            addDeviceConfig.setPropertyValue(prefix + "CaCertificateFilePath", caCert);
            addDeviceConfig.setPropertyValue(prefix + "EnableMutualTls", mutualTls);
            if (mutualTls)
            {
                addDeviceConfig.setPropertyValue(prefix + "CertificateFilePath", ClientCert);
                addDeviceConfig.setPropertyValue(prefix + "KeyFilePath", ClientKey);
            }
        }
    }

    [[maybe_unused]]
    inline std::string connectionString(const char* prefix, uint16_t port)
    {
        return std::string(prefix) + "://127.0.0.1:" + std::to_string(port) + "/";
    }
}

END_NAMESPACE_OPENDAQ

#endif
