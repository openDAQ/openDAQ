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

// Validation and parsing of the SFTP file transfer factory arguments, shared by every backend (libssh2, libssh, ...).
// Not meant for users of the file transfer objects: they only need a backend factory header.

#pragma once
#include <coretypes/string_ptr.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/logger_ptr.h>

#include <cstdint>

namespace daq::sftp
{

// Server address split into host and port.
struct Endpoint
{
    StringPtr host;
    std::uint16_t port = 22;
};

// Parses `host`, `host:port`, `[IPv6]` or `[IPv6]:port` (default port 22). Throws InvalidParameterException.
Endpoint parseEndpoint(const StringPtr& address);

// Validates the arguments of a backend factory. Throws InvalidParameterException if any of them is unassigned or the
// selected authentication method is not an SFTP one, CredentialsRequiredException if no credential is supplied.
// The address itself is parsed by parseEndpoint. Afterwards the required fields of the selected method are guaranteed to be non-empty: the
// config rejects a supplied credential that misses any of them.
void validateArguments(const StringPtr& address, const AuthenticationConfigPtr& authenticationConfig, const LoggerPtr& logger);

}
