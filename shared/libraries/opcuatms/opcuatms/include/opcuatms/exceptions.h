/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opcuatms/opcuatms.h>
#include <opcuatms/errors.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

DEFINE_EXCEPTION(OpcUaGeneral, OPENDAQ_ERR_OPCUA_GENERAL, "General OpcUa error")
DEFINE_EXCEPTION(OpcUaObjectNotDecoded, OPENDAQ_ERR_OPCUA_OBJECT_NOT_DECODED, "Extension object is not decoded.")
DEFINE_EXCEPTION(OpcUaClientCallNotAvailable, OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE, "This function call is not available/implemented for usage on connected-to OpcUa servers.")

END_NAMESPACE_OPENDAQ_OPCUA
