#include <gtest/gtest.h>
#include <chrono>
#include <opendaq/opendaq.h>
#include <opendaq/sync_component_private_ptr.h>
#include "docs_test_helpers.h"

using DevicesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/device.adoc
TEST_F(DevicesTest, DeviceInfo)
{
    auto instance = docs_test_helpers::setupInstance();
    auto info = instance.getInfo();
    ASSERT_TRUE(info.getSerialNumber().assigned());
    ASSERT_TRUE(info.getName().assigned());
}

// Corresponding document: Antora/modules/explanation/pages/device.adoc
TEST_F(DevicesTest, DeviceComponents)
{
    auto instance = docs_test_helpers::setupInstance();
    auto device = instance.getRootDevice();
    
    ListPtr<IFunctionBlock> functionBlocks = device.getFunctionBlocks();
    ListPtr<ISignal> signals = device.getSignals();
    ListPtr<IDevice> devices = device.getDevices();
    FolderPtr inputsOutputsFolder = device.getInputsOutputsFolder();
    ListPtr<IChannel> channels =  device.getChannels();
    ListPtr<IComponent> customComponents = device.getCustomComponents();

    ASSERT_TRUE(functionBlocks.assigned());
    ASSERT_TRUE(signals.assigned());
    ASSERT_TRUE(devices.assigned());
    ASSERT_TRUE(inputsOutputsFolder.assigned());
    ASSERT_TRUE(channels.assigned());
    ASSERT_TRUE(customComponents.assigned());
}

// Corresponding document: Antora/modules/explanation/pages/device.adoc
TEST_F(DevicesTest, InstanceAndRoot)
{
    auto instance = docs_test_helpers::setupInstance();
    auto availableDevices1 = instance.getAvailableDevices();
    auto availableDevices2 = instance.getRootDevice().getAvailableDevices();
}

TEST_F(DevicesTest, SettingSyncComponent)
{
    InstancePtr instance = docs_test_helpers::setupInstance();
    TypeManagerPtr typeManager = instance.getContext().getTypeManager();
    DevicePtr device = instance.getRootDevice();
    SyncComponentPtr syncComponent = device.getSyncComponent();
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    // The root device is a reference device which has a sync component with a PTP interface and a clock sync interface
    // lets clean it up and create a new PTP interface
    syncComponentPrivate.removeInterface("PtpSyncInterface");
    syncComponentPrivate.removeInterface("InterfaceClockSync");
    ASSERT_EQ(syncComponent.getInterfaces().getCount(), 0);

    // Create a new PTP interface from property objects class `PtpSyncInterface`
    PropertyObjectPtr ptpSyncInterface = PropertyObject(typeManager, "PtpSyncInterface");

    // Set custom selection options for the mode property
    auto modeOptions = Dict<IInteger, IString>({{2, "Auto"}, {3, "Off"}});
    ptpSyncInterface.setPropertyValue("ModeOptions", modeOptions);

    // Overwrite the default sync component parameters
    ptpSyncInterface.setPropertyValue("Mode", 2); // set the mode to `Auto`
    
    PropertyObjectPtr status = ptpSyncInterface.getPropertyValue("Status");
    status.setPropertyValue("State", 2);
    status.setPropertyValue("Grandmaster", "1234");

    PropertyObjectPtr parameters = ptpSyncInterface.getPropertyValue("Parameters");

    StructPtr configuration = parameters.getPropertyValue("PtpConfigurationStructure");

    auto newConfiguration = StructBuilder(configuration)
        .set("ClockType",  Enumeration("PtpClockTypeEnumeration", "OrdinaryBoundary", typeManager))
        .set("TransportProtocol", Enumeration("PtpProtocolEnumeration", "UDP6_SCOPE", typeManager))
        .set("StepFlag", Enumeration("PtpStepFlagEnumeration", "Two", typeManager))
        .set("DomainNumber", 123)
        .set("LeapSeconds", 123)
        .set("DelayMechanism", Enumeration("PtpDelayMechanismEnumeration", "E2E", typeManager))
        .set("Priority1", 123)
        .set("Priority2", 123)
        .set("Profiles", Enumeration("PtpProfileEnumeration", "802_1AS", typeManager))
        .build();

    parameters.setPropertyValue("PtpConfigurationStructure", newConfiguration);

    // Add the new PTP interface to the sync component
    syncComponentPrivate.addInterface(ptpSyncInterface);

    // Get the new PTP interface
    auto interfaces = syncComponent.getInterfaces();
    ASSERT_EQ(interfaces.getCount(), 1);
    ASSERT_TRUE(interfaces.hasKey("PtpSyncInterface"));
    PropertyObjectPtr newPtpSyncInterface = interfaces.get("PtpSyncInterface");

    // Check that mode has new selecton options
    auto modeProperty = newPtpSyncInterface.getProperty("Mode");
    ASSERT_EQ(modeProperty.getSelectionValues(), modeOptions);
    ASSERT_EQ(newPtpSyncInterface.getPropertySelectionValue("Mode"), "Auto");

    // We can edit the existing PTP interface as well
    newPtpSyncInterface.setPropertyValue("Mode", 3); // set the mode to `Input`
}

END_NAMESPACE_OPENDAQ
