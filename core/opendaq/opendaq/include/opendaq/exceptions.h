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
#include <coretypes/exceptions.h>
#include <opendaq/errors.h>

BEGIN_NAMESPACE_OPENDAQ

DEFINE_EXCEPTION(ConnectionLost, OPENDAQ_ERR_CONNECTION_LOST, "Lost connection to the server.")
DEFINE_EXCEPTION(ConnectionLimitReached, OPENDAQ_ERR_CONNECTION_LIMIT_REACHED, "Connection rejected - connections limit reached")
DEFINE_EXCEPTION(ServerVersionTooLow, OPENDAQ_ERR_SERVER_VERSION_TOO_LOW, "The client attempted to call a function that requires a newer version of the openDAQ server")
DEFINE_EXCEPTION(ControlClientRejected, OPENDAQ_ERR_CONTROL_CLIENT_REJECTED, "Connection rejected - too many control clients")

END_NAMESPACE_OPENDAQ
