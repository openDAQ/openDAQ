#pragma once

#include <licensing_module/common.h>
#include <licensing_module/license_checker_impl.h>
#include <opendaq/license_checker_ptr.h>
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
                               LicenseCheckerPtr licenseComponent);
    ~PassthroughFbImpl() override;

    static constexpr const char* TypeID = "ProtectedAnalyticsModulePassthrough";
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
    LicenseCheckerPtr mLicenseComponent;
    bool mIsLicenseCheckedOut;
    InputPortPtr _inputPort;
    SignalConfigPtr _outputSignal;
    SignalConfigPtr _outputDomainSignal;
};
/*!@}*/

END_NAMESPACE_LICENSING_MODULE
