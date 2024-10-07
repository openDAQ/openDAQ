/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <config_protocol/config_protocol.h>
#include <opendaq/folder_config_ptr.h>
#include <coreobjects/object_keys.h>
#include <opendaq/mirrored_signal_config_ptr.h>

namespace daq::config_protocol
{

class ConfigProtocolStreamingConsumer
{
    using ComponentFindCallback = std::function<ComponentPtr(const StringPtr& /*globalId*/)>;

public:
    ConfigProtocolStreamingConsumer(const ContextPtr& daqContext, const FolderConfigPtr& externalSignalsFolder);
    ~ConfigProtocolStreamingConsumer();

    MirroredSignalConfigPtr getOrAddExternalSignal(const ParamsDictPtr& params);
    void removeExternalSignals(const ParamsDictPtr& params);

    bool isExternalSignal(const SignalPtr& signal);

    void processClientToServerStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet);
    bool isForwardedCoreEvent(const ComponentPtr& component, const CoreEventArgsPtr& eventArgs);

private:
    MirroredSignalConfigPtr createMirroredExternalSignal(const StringPtr& signalStringId,
                                                         const StringPtr& serializedSignal,
                                                         SignalNumericIdType signalNumericId);

    void addExternalSignal(const MirroredSignalConfigPtr& signal, SignalNumericIdType signalNumericId);
    void removeExternalSignal(const MirroredSignalConfigPtr& signal, SignalNumericIdType signalNumericId);

    ContextPtr daqContext;
    LoggerComponentPtr loggerComponent;
    std::mutex sync;

    std::unordered_map<SignalNumericIdType, MirroredSignalConfigPtr> mirroredExternalSignals;
    FolderConfigPtr externalSignalsFolder;
    std::unordered_set<StringPtr, StringHash, StringEqualTo> mirroredExternalSignalsIds;
};

}
