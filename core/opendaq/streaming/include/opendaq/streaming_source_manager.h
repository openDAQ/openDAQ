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

#include <opendaq/mirrored_device_config_ptr.h>
#include <map>
#include <unordered_set>

BEGIN_NAMESPACE_OPENDAQ

class StreamingSourceManager
{
public:
    explicit StreamingSourceManager(const ContextPtr& context,
                                    const DevicePtr& ownerDevice,
                                    const PropertyObjectPtr& deviceConfig);

    ~StreamingSourceManager();

private:
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);

    void componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);

    void enableStreamingForAddedComponent(const ComponentPtr& addedComponent);
    void enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent);

    static ListPtr<IMirroredDeviceConfig> getAllDevicesRecursively(const MirroredDeviceConfigPtr& device);

    AddressInfoPtr findMatchingAddress(const ListPtr<IAddressInfo>& availableAddresses,
                                        const AddressInfoPtr& deviceConnectionAddress);
    static AddressInfoPtr getDeviceConnectionAddress(const DevicePtr& device);
    void completeStreamingConnections(const MirroredDeviceConfigPtr& topDevice);
    void attachStreamingsToDevice(const MirroredDeviceConfigPtr& device);

    ContextPtr context;
    WeakRefPtr<IDevice> ownerDeviceRef;
    PropertyObjectPtr deviceConfig;
    LoggerComponentPtr loggerComponent;

    bool minHopsStreamingHeuristicEnabled{false};
    bool automaticallyConnectStreamings{false};
    std::unordered_set<std::string> allowedProtocolsOnly;
    std::map<StringPtr, SizeT> prioritizedProtocolsMap; // protocol Id as a key, protocol priority as a value
    StringPtr primaryAddressType;
};

END_NAMESPACE_OPENDAQ
