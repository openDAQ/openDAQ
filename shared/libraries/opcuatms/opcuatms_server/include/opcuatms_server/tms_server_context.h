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
#include <opcuatms/opcuatms.h>
#include <opendaq/context_ptr.h>
#include <opendaq/device_ptr.h>
#include <opcuatms_server/objects/tms_server_object.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerContext : public std::enable_shared_from_this<TmsServerContext>
{
public:
    TmsServerContext(const ContextPtr& context, const DevicePtr& rootDevice);
    ~TmsServerContext();
    void registerComponent(const ComponentPtr& component, TmsServerObject& obj);
    DevicePtr getRootDevice();
    ComponentPtr findComponent(const std::string& globalId);

private:
    ContextPtr context;
    DevicePtr rootDevice;

    std::unordered_map<std::string, std::weak_ptr<tms::TmsServerObject>> idToObjMap;
    void coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs);
    std::string toRelativeGlobalId(const std::string& globalId);
};

using TmsServerContextPtr = std::shared_ptr<TmsServerContext>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
