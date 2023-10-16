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
#include "opcuatms/opcuatms.h"
#include "opcuaclient/opcuaclient.h"
#include <mutex>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientContext;
using TmsClientContextPtr = std::shared_ptr<TmsClientContext>;

class TmsClientContext
{
public:
    explicit TmsClientContext(const opcua::OpcUaClientPtr& client);

    const opcua::OpcUaClientPtr& getClient() const;

    void registerObject(const opcua::OpcUaNodeId& nodeId, const BaseObjectPtr& object);
    void unregisterObject(const opcua::OpcUaNodeId& nodeId);
    BaseObjectPtr getObject(const opcua::OpcUaNodeId& nodeId) const;
    opcua::OpcUaNodeId getNodeId(const BaseObjectPtr object) const;

    template <class I, class Ptr = typename InterfaceToSmartPtr<I>::SmartPtr>
    Ptr getObject(const opcua::OpcUaNodeId& nodeId)
    {
        auto obj = this->getObject(nodeId);
        if (obj.assigned())
            return obj.asPtrOrNull<I, Ptr>();
        return Ptr();
    }

    template <class I, class Ptr = typename InterfaceToSmartPtr<I>::SmartPtr>
    opcua::OpcUaNodeId getNodeId(const Ptr object) const
    {
        return this->getNodeId(object);
    }

protected:
    opcua::OpcUaClientPtr client;
    mutable std::mutex mutex;

    // Context should not hold objects because of cycling reference
    std::unordered_map<opcua::OpcUaNodeId, IBaseObject*> objects;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
