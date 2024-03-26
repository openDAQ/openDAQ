/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include "coreobjects/property_impl.h"
#include "opcuatms/opcuatms.h"
#include "opcuatms_client/objects/tms_client_object_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientPropertyImpl : public TmsClientObjectImpl, public PropertyImpl
{
public:
    explicit TmsClientPropertyImpl(const ContextPtr& daqContext, const TmsClientContextPtr& ctx, const opcua::OpcUaNodeId& nodeId);

protected:
    LoggerComponentPtr loggerComponent;

    void readBasicInfo();
    void configurePropertyFields();
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
