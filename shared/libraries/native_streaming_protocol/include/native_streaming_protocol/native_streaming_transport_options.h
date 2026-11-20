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

#include <native_streaming_protocol/native_streaming_constants.h>
#include <opendaq/transport_option_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/exceptions.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

/// @brief Creates the native transport option with the given id at the given port.
/// - "ws": plaintext.
/// - "wss": TLS.
/// - "wss-mtls": TLS, and the client presents a certificate.
inline TransportOptionPtr NativeTransportOption(const StringPtr& id, Int port)
{
    if (id == CONST_NATIVE_TRANSPORT_WS)
        return TransportOption(id, port);
    if (id == CONST_NATIVE_TRANSPORT_WSS)
        return TransportOption(id, port, SecurityMode::Encrypted);
    if (id == CONST_NATIVE_TRANSPORT_WSS_MTLS)
        return TransportOption(id, port, SecurityMode::Encrypted, True);

    DAQ_THROW_EXCEPTION(NotFoundException, "There is no native transport option \"{}\"", id);
}

/// @brief Creates the transport options this build supports, at their default ports, plain first.
/// - "wss" needs a build with TLS.
/// - "wss-mtls" also needs the certificate settings, which NATIVE_STREAMING_TLS_WITHOUT_SIGN leaves out.
inline DictPtr<IString, ITransportOption> DefaultNativeTransportOptions()
{
    auto options = Dict<IString, ITransportOption>();
    options.set(CONST_NATIVE_TRANSPORT_WS, NativeTransportOption(CONST_NATIVE_TRANSPORT_WS, DEFAULT_PORT));
#if NATIVE_STREAMING_ENABLE_TLS
    options.set(CONST_NATIVE_TRANSPORT_WSS, NativeTransportOption(CONST_NATIVE_TRANSPORT_WSS, DEFAULT_TLS_PORT));
#ifndef NATIVE_STREAMING_TLS_WITHOUT_SIGN
    options.set(CONST_NATIVE_TRANSPORT_WSS_MTLS, NativeTransportOption(CONST_NATIVE_TRANSPORT_WSS_MTLS, DEFAULT_TLS_PORT));
#endif
#endif
    return options;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
