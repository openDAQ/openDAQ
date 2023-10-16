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

#include "opcuacommon.h"
#include "opcuavariant.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaCallMethodResult
{
public:
    OpcUaCallMethodResult(const UA_CallMethodResult& callMethodResult);
    virtual ~OpcUaCallMethodResult();

    const UA_StatusCode& getStatusCode() const;
    bool isStatusOK() const;

    size_t getOutputArgumentsSize() const;
    OpcUaVariant getOutputArgument(size_t i) const;

    const UA_CallMethodResult& getCallMethodResult() const;
    operator const UA_CallMethodResult&() const;

protected:
    const UA_CallMethodResult& callMethodResult;
};

END_NAMESPACE_OPENDAQ_OPCUA
