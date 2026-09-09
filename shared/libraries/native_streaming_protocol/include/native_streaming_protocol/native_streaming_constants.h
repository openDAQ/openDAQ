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

#include <native_streaming_protocol/native_streaming_protocol.h>

#include <cstdint>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

static constexpr const char* CONST_NATIVE_STREAMING_PREFIX = "daq.ns";
static constexpr const char* CONST_NATIVE_STREAMING_SECURE_PREFIX = "daq.nss";
static constexpr const char* CONST_NATIVE_CONFIG_PREFIX = "daq.nd";
static constexpr const char* CONST_NATIVE_CONFIG_SECURE_PREFIX = "daq.nds";

static constexpr const char* CONST_NATIVE_STREAMING_ID = "OpenDAQNativeStreaming";
static constexpr const char* CONST_NATIVE_STREAMING_SECURE_ID = "OpenDAQNativeStreamingSecure";
static constexpr const char* CONST_NATIVE_CONFIG_ID = "OpenDAQNativeConfiguration";
static constexpr const char* CONST_NATIVE_CONFIG_SECURE_ID = "OpenDAQNativeConfigurationSecure";

static constexpr const char* CONST_NATIVE_SERVER_TYPE_ID = "OpenDAQNativeStreaming";

static constexpr const char* CONST_NATIVE_PROTOCOL_GROUP_ID = "NativeStreaming";
static constexpr const int64_t CONST_NATIVE_SECURITY_LVL = 0;
static constexpr const int64_t CONST_NATIVE_SECURE_SECURITY_LVL = 10;

static constexpr const char* CONST_NATIVE_SERVICE_NAME = "_opendaq-streaming-native._tcp.local.";
static constexpr const char* CONST_NATIVE_TLS_SERVICE_NAME = "_opendaq-streaming-native-tls._tcp.local.";
static constexpr const char* CONST_NATIVE_SERVICE_CAPABILITY = "OPENDAQ_NS";

static constexpr const char* PROPERTY_ENABLE_PORT_SERVER = "EnablePort";
static constexpr const char* PROPERTY_PORT_SERVER = "NativeStreamingPort";
static constexpr const char* PROPERTY_ENABLE_TLS_PORT_SERVER = "EnableTlsPort";
static constexpr const char* PROPERTY_TLS_PORT_SERVER = "NativeStreamingTlsPort";
static constexpr const char* PROPERTY_ENABLE_MTLS_SERVER = "EnableMutualTls";
static constexpr const char* PROPERTY_CERT_FILE_PATH_SERVER = "CertificateFilePath";
static constexpr const char* PROPERTY_KEY_FILE_PATH_SERVER = "KeyFilePath";
static constexpr const char* PROPERTY_CA_CERT_FILE_PATH_SERVER = "CaCertificateFilePath";
static constexpr const char* PROPERTY_PATH_SERVER = "Path";

static constexpr const char* PROPERTY_VERIFY_SERVER_CERT_CLIENT = "VerifyServerCertificate";
static constexpr const char* PROPERTY_ENABLE_MTLS_CLIENT = "EnableMutualTls";
static constexpr const char* PROPERTY_CERT_FILE_PATH_CLIENT = "CertificateFilePath";
static constexpr const char* PROPERTY_KEY_FILE_PATH_CLIENT = "KeyFilePath";
static constexpr const char* PROPERTY_CA_CERT_FILE_PATH_CLIENT = "CaCertificateFilePath";

static constexpr bool DEFAULT_ENABLE_PORT = true;
static constexpr bool DEFAULT_ENABLE_TLS_PORT = false;
static constexpr bool DEFAULT_ENABLE_MTLS = true;
static constexpr bool DEFAULT_VERIFY_SERVER_CERT = true;
static constexpr uint16_t DEFAULT_PORT = 7420;
static constexpr uint16_t DEFAULT_TLS_PORT = 7422;
static constexpr const char* DEFAULT_CERT_FILE_PATH = "";
static constexpr const char* DEFAULT_KEY_FILE_PATH = "";
static constexpr const char* DEFAULT_CA_CERT_FILE_PATH = "";

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
