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
#include <opcuatms/opcuatms.h>
#include <opcuaclient/opcuaclient.h>
#include <mutex>
#include <opcuaclient/cached_reference_browser.h>
#include <opcuaclient/attribute_reader.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_ptr.h>
#include <opendaq/device_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientContext;
using TmsClientContextPtr = std::shared_ptr<TmsClientContext>;

class TmsClientContext
{
public:
    explicit TmsClientContext(const opcua::OpcUaClientPtr& client, const ContextPtr& context);

    const opcua::OpcUaClientPtr& getClient() const;

    void registerRootDevice(const DevicePtr& rootDevice);
    DevicePtr getRootDevice();
    void registerObject(const opcua::OpcUaNodeId& nodeId, const BaseObjectPtr& object);
    void unregisterObject(const opcua::OpcUaNodeId& nodeId);
    BaseObjectPtr getObject(const opcua::OpcUaNodeId& nodeId) const;
    opcua::OpcUaNodeId getNodeId(const BaseObjectPtr object) const;
    CachedReferenceBrowserPtr getReferenceBrowser();
    AttributeReaderPtr getAttributeReader();
    void readObjectAttributes(const OpcUaNodeId& nodeId, bool forceRead = false);
    size_t getMaxNodesPerBrowse();
    size_t getMaxNodesPerRead();
    void addEnumerationTypesToTypeManager();

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
    ContextPtr context;
    LoggerComponentPtr loggerComponent;
    CachedReferenceBrowserPtr referenceBrowser;
    AttributeReaderPtr attributeReader;
    mutable std::mutex mutex;
    // Context should not hold objects because of cycling reference
    std::unordered_map<opcua::OpcUaNodeId, IBaseObject*> objects;
    size_t maxNodesPerBrowse = 0;
    size_t maxNodesPerRead = 0;
    WeakRefPtr<IDevice> rootDevice;
    bool enumerationTypesAdded = false;

    void initReferenceBrowser();
    void initAttributeReader();
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
