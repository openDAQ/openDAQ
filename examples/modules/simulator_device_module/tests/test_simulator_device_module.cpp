#include <testutils/testutils.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_ptr.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/module_ptr.h>
#include <opendaq/search_filter_factory.h>
#include <simulator_device_module/module_dll.h>

using namespace daq;
using SimulatorDeviceModuleTest = testing::Test;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

static DevicePtr CreateDevice(const ModulePtr& module)
{
    const auto info = module.getAvailableDevices()[0];
    const DeviceTypePtr deviceType = module.getAvailableDeviceTypes().get("SimulatorDevice");
    return module.createDevice(info.getConnectionString(), nullptr, deviceType.createDefaultConfig());
}

TEST_F(SimulatorDeviceModuleTest, Synchronization)
{
    const auto module = CreateModule();
    const auto device = CreateDevice(module);

    const auto sync = device.getSynchronization();
    ASSERT_TRUE(sync.assigned());
    ASSERT_EQ(sync.getInterfaces().getCount(), 1u);
    ASSERT_EQ(sync.getSource().getId(), "ClockSyncInterface");
    ASSERT_EQ(sync.getSource().getReferenceDomainId().toStdString(), "local:" + device.getLocalId().toStdString());
}

TEST_F(SimulatorDeviceModuleTest, ReferenceDomainInfo)
{
    const auto module = CreateModule();
    const auto device = CreateDevice(module);

    const StringPtr referenceDomainId = String("local:" + device.getLocalId().toStdString());
    const auto info = device.getDomain().getReferenceDomainInfo();
    ASSERT_EQ(info.getReferenceDomainId(), referenceDomainId);
    ASSERT_EQ(info.getReferenceDomainIds(), List<IString>(referenceDomainId));
    ASSERT_EQ(info.getReferenceDomainOffset(), 0);
    ASSERT_EQ(info.getReferenceTimeProtocol(), TimeProtocol::Utc);

    for (const auto& signal : device.getSignals(search::Recursive(search::Any())))
    {
        const auto domainSignal = signal.getDomainSignal();
        if (!domainSignal.assigned())
            continue;
        ASSERT_EQ(domainSignal.getDescriptor().getReferenceDomainInfo(), info) << domainSignal.getGlobalId();
    }
}
