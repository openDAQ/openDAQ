/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coretypes/errors.h>

#define OPENDAQ_ERRTYPE_OPCUA                  0x09

#define OPENDAQ_ERR_OPCUA_GENERAL                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPCUA, 0x001)
#define OPENDAQ_ERR_OPCUA_OBJECT_NOT_DECODED        OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPCUA, 0x002)
#define OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPCUA, 0x003)
