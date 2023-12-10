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

#include <coretypes/coretypes.h>
#include <opcuashared/opcuaobject.h>
#include <open62541/types_daqbsp_generated.h>
#include <open62541/types_daqdevice_generated.h>
#include <opcuashared/opcua.h>

#define BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS \
    namespace daq::opcua::tms          \
    {
#define END_NAMESPACE_OPENDAQ_OPCUA_TMS }
