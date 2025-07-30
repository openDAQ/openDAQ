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

#include <licensing_module/common.h>
#include <licensing_module/license_checker_impl.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_packet_ptr.h>

BEGIN_NAMESPACE_LICENSING_MODULE

class PassthroughFbImpl final : public FunctionBlock
{
public:
    explicit PassthroughFbImpl(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               std::shared_ptr<LicenseChecker> licenseComponent);
    ~PassthroughFbImpl() override;

    static constexpr const char* TypeID = "LicensingModulePassthrough";
    static FunctionBlockTypePtr CreateType();

private:
    void createInputPorts();
    void createSignals();

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;
    void onPacketReceived(const InputPortPtr& port) override;
    void processEventPacket(const EventPacketPtr& packet);
    void processDataPacket(DataPacketPtr&& packet, ListPtr<IPacket>& outQueue, ListPtr<IPacket>& outDomainQueue);

private:
    std::shared_ptr<LicenseChecker> _licenseComponent;
    bool _isLicenseCheckedOut;
    InputPortPtr _inputPort;
    SignalConfigPtr _outputSignal;
    SignalConfigPtr _outputDomainSignal;
};
/*!@}*/

END_NAMESPACE_LICENSING_MODULE
