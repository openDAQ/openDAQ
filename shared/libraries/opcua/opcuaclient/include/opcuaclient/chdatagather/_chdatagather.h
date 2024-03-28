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

#include "opcuashared/opcua.h"

BEGIN_NAMESPACE_OPCUA

class OpcUaClient;

class ChDataGather;
using ChDataGatherPtr = std::shared_ptr<ChDataGather>;

class ChDataGather
{
public:
    ChDataGather(OpcUaClient* client);

    OpcUaClient* getClient();

    virtual void start();

    virtual void getData(RelativeTimeInSeconds seconds);

    virtual void stop();
protected:
    OpcUaClient* client;
};

END_NAMESPACE_OPCUA
