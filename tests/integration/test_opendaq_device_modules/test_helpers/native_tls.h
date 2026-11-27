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

#if NATIVE_STREAMING_ENABLE_TLS

#include <native_streaming_protocol/native_streaming_constants.h>
#include <opendaq/opendaq.h>

#include <string>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers::native_tls
{
    using namespace daq::opendaq_native_streaming_protocol;

    inline constexpr const char* ServerTypeId = CONST_NATIVE_SERVER_TYPE_ID;

    // the pseudo-device and the streaming share their protocol id
    inline constexpr const char* PlainStreamingId = CONST_NATIVE_STREAMING_ID;
    inline constexpr const char* PlainConfigId = CONST_NATIVE_CONFIG_ID;

    inline constexpr const char* WsOption = CONST_NATIVE_TRANSPORT_WS;
    inline constexpr const char* WssOption = CONST_NATIVE_TRANSPORT_WSS;

    inline constexpr uint16_t PlainPort = DEFAULT_PORT;
    inline constexpr uint16_t TlsPort = DEFAULT_TLS_PORT;

    // kept apart from the LT fixtures: the file names coincide, the authorities do not
    inline constexpr const char* CaCert = "native_secrets/ca.crt";
    inline constexpr const char* OtherCaCert = "native_secrets/other-ca.crt";
    inline constexpr const char* OtherCaKey = "native_secrets/other-ca.key";
    inline constexpr const char* ServerCert = "native_secrets/server.crt";
    inline constexpr const char* ServerKey = "native_secrets/server.key";
    inline constexpr const char* ClientCert = "native_secrets/client.crt";
    inline constexpr const char* ClientKey = "native_secrets/client.key";

    /// @brief server configuration with the TLS listener on
    /// @param mutualTls requires clients to present a certificate signed by CaCert
    /// Under NATIVE_STREAMING_TLS_WITHOUT_SIGN the server has no such setting, and asking for it throws
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
#ifdef NATIVE_STREAMING_TLS_WITHOUT_SIGN
        if (mutualTls)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "The native server has no mutual TLS in this build");
#else
        config.setPropertyValue("EnableMutualTls", mutualTls);
        if (mutualTls)
            config.setPropertyValue("CaCertificateFilePath", CaCert);
#endif
        return config;
    }

    /// @brief the add device config for the device at the connection string, with the defaults of its type
    [[maybe_unused]]
    inline AddDeviceConfigPtr addDeviceConfig(const InstancePtr& clientInstance, const std::string& connectionString)
    {
        return clientInstance.createAddDeviceConfig(DeviceInfo(connectionString));
    }

    /// @brief the native streaming config of an add device config
    [[maybe_unused]]
    inline AddStreamingConfigPtr nativeStreamingConfig(const AddDeviceConfigPtr& config)
    {
        return config.getStreamingOptions().getAvailableStreamingConfigs().get(PlainStreamingId);
    }

#ifdef NATIVE_STREAMING_TLS_WITHOUT_SIGN

    // The client has no certificate settings, so the helpers below only select the TLS option.
    // They keep their signatures so that the tests which only need a working secure connection stay
    // as they are; the tests which depend on the certificate settings are left out of this build.

    /// @brief selects the TLS option on an add device or add streaming config; the client never verifies the server
    template <typename ConfigPtr>
    void selectTls(const ConfigPtr& config, bool /*mutualTls*/ = false, const char* /*caCert*/ = CaCert)
    {
        config.setTransportOption(WssOption);
    }

#else

    /// @brief selects the TLS option on an add device or add streaming config, which authenticates the server
    /// @param mutualTls presents ClientCert / ClientKey to a server which asks for one
    /// @param caCert the authority to verify the server against
    template <typename ConfigPtr>
    void selectTls(const ConfigPtr& config, bool mutualTls = false, const char* caCert = CaCert)
    {
        config.setTransportOption(mutualTls ? CONST_NATIVE_TRANSPORT_WSS_MTLS : WssOption);

        const PropertyObjectPtr typeConfig = config.getTypeConfig();
        typeConfig.setPropertyValue("CaCertificateFilePath", caCert);
        typeConfig.setPropertyValue("EnableMutualTls", mutualTls);
        if (mutualTls)
        {
            typeConfig.setPropertyValue("CertificateFilePath", ClientCert);
            typeConfig.setPropertyValue("KeyFilePath", ClientKey);
        }
    }

    /// @brief selects the TLS option, which encrypts without authenticating the server
    template <typename ConfigPtr>
    void selectUnverifiedTls(const ConfigPtr& config)
    {
        config.setTransportOption(WssOption);
        config.getTypeConfig().setPropertyValue("VerifyServerCertificate", False);
    }

#endif

    /// @brief the add device config for the device at the connection string, over the TLS option
    [[maybe_unused]]
    inline AddDeviceConfigPtr secureAddDeviceConfig(const InstancePtr& clientInstance,
                                                    const std::string& connectionString,
                                                    bool mutualTls = false,
                                                    const char* caCert = CaCert)
    {
        auto config = addDeviceConfig(clientInstance, connectionString);
        selectTls(config, mutualTls, caCert);
        return config;
    }

    [[maybe_unused]]
    inline std::string connectionString(const char* prefix, uint16_t port)
    {
        return std::string(prefix) + "://127.0.0.1:" + std::to_string(port) + "/";
    }
}

END_NAMESPACE_OPENDAQ

#endif
