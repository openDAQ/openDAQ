#include <testutils/testutils.h>
#include <opendaq/sync_component_factory.h>
#include <opendaq/sync_component_private_ptr.h>
#include <opendaq/synchronization_private_ptr.h>
#include <opendaq/sync_interface_base_impl.h>
#include <opendaq/sync_interface_internal_ptr.h>
#include <opendaq/ptp_sync_interface_impl.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/component_deserialize_context_factory.h>

using SyncComponentTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(SyncComponentTest, testGetSyncLocked)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    ASSERT_FALSE(syncComponent.getSyncLocked());
}

TEST_F(SyncComponentTest, testSetSyncLocked)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    ASSERT_FALSE(syncComponent.getSyncLocked());
    syncComponentPrivate.setSyncLocked(true);
    ASSERT_TRUE(syncComponent.getSyncLocked());
}

TEST_F(SyncComponentTest, testAddInterface)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();
    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    PropertyObjectPtr interface = PropertyObject();
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface), OPENDAQ_ERR_INVALID_ARGUMENT);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "InterfaceClockSync");
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_SUCCESS);
}

TEST_F(SyncComponentTest, testRemoveInterface)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();
    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_ERR_INVALID_ARGUMENT);

    PropertyObjectPtr interface2 = PropertyObject(typeManager, "InterfaceClockSync");
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);

    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->removeInterface(String("SyncInterfaceBase")), OPENDAQ_ERR_NOTFOUND);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->removeInterface(String("InterfaceClockSync")), OPENDAQ_SUCCESS);
}

TEST_F(SyncComponentTest, testAddInhertiedInterfaces)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();
    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    //Assert that an interfaces with valid base class can be added
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);

    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_ERR_ALREADYEXISTS);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_ERR_ALREADYEXISTS);

    //Assert that an interfaces with invalid base class cannot be added
    auto propClass = PropertyObjectClassBuilder("prop").build();
    typeManager.addType(propClass);
    PropertyObjectPtr interface4 = PropertyObject(typeManager, "prop");
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface4), OPENDAQ_ERR_INVALID_ARGUMENT);
}

TEST_F(SyncComponentTest, testSetSelectedSource)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();

    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);


    ASSERT_EQ(syncComponent.getSelectedSource(), 0);
    syncComponent.setSelectedSource(1);
    ASSERT_EQ(syncComponent.getSelectedSource(), 1);

    // out of range
    ASSERT_ANY_THROW(syncComponent.setSelectedSource(2));
    ASSERT_EQ(syncComponent.getSelectedSource(), 1);
}

TEST_F(SyncComponentTest, testSelectedSourceListChanged)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();

    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);

    auto interfaceNames = syncComponent.getInterfaces();
    ASSERT_EQ(interfaceNames.getCount(), 2u);

    syncComponent.setSelectedSource(1);
    ASSERT_EQ(syncComponentPrivate->removeInterface(String("InterfaceClockSync")), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent.getSelectedSource(), 0);
    
    interfaceNames = syncComponent.getInterfaces();
    ASSERT_EQ(interfaceNames.getCount(), 1u);
    ASSERT_TRUE(interfaceNames.hasKey("PtpSyncInterface"));
}

TEST_F(SyncComponentTest, Serialization)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();
    SyncComponentPtr syncComponent = SyncComponent(ctx, nullptr, String("localId"));
    SyncComponentPrivatePtr syncComponentPrivate = syncComponent.asPtr<ISyncComponentPrivate>(true);

    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);

    const auto serializer = JsonSerializer();

    syncComponent.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(ctx, nullptr, nullptr, "temp");
    const SyncComponentPtr syncComponentDeserialized = deserializer.deserialize(serializedJson, deserializeContext);

    ASSERT_EQ(syncComponent.getSelectedSource(), syncComponentDeserialized.getSelectedSource());
    ASSERT_EQ(syncComponent.getSyncLocked(), syncComponentDeserialized.getSyncLocked());
    ASSERT_EQ(syncComponent.getInterfaces().getKeyList(), syncComponentDeserialized.getInterfaces().getKeyList());
}

// =====================================================
// sync and SyncInterface Tests
// =====================================================

class TestSyncInterface : public SyncInterfaceBaseImpl
{
public:
    using Super = SyncInterfaceBaseImpl;

    TestSyncInterface(const TypeManagerPtr& manager,
                      const StringPtr& name,
                      const std::vector<SyncMode> & availableModes)
        : Super(manager, name, availableModes)
    {
    }

    static SyncInterfacePtr Create(const TypeManagerPtr& manager,
                                   const StringPtr& name = "TestInterface",
                                   const std::vector<SyncMode> & availableModes = {SyncMode::Off, SyncMode::Input, SyncMode::Output, SyncMode::Auto})
    {
        return createWithImplementation<ISyncInterface, TestSyncInterface>(manager, name, availableModes);
    }

    void setSyncType(const StringPtr& syncType)
    {
        this->syncType = syncType;
    }

    ErrCode INTERFACE_FUNC getSyncType(IString** syncType) override
    {
        if (!this->syncType.assigned())
            return Super::getSyncType(syncType);

        OPENDAQ_PARAM_NOT_NULL(syncType);
        *syncType = this->syncType.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

private:
    StringPtr syncType;
};

using SynchronizationTest = testing::Test;

TEST_F(SynchronizationTest, Create)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization(ctx.getTypeManager(), "testDevice");
    ASSERT_TRUE(sync.assigned());
}

TEST_F(SynchronizationTest, getSyncInterfaces)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization(ctx.getTypeManager(), "testDevice");

    const auto interfaces = sync.getSyncInterfaces();
    ASSERT_EQ(interfaces.getCount(), 1u);
    ASSERT_TRUE(interfaces.hasKey("ClockSyncInterface"));
    ASSERT_EQ(sync.getSource().getSyncType(), "local");
}

TEST_F(SynchronizationTest, getAvailableSyncSources)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    // ClockSyncInterface can be a source by default
    const auto sourcesBefore = sync.getAvailableSyncSources();
    ASSERT_EQ(sourcesBefore.getCount(), 1u);
    ASSERT_TRUE(sourcesBefore.hasKey("ClockSyncInterface"));

    // An interface that can never act as a source (no Input/Auto mode available)
    const auto outputOnlyInterface = TestSyncInterface::Create(manager, "OutputOnlyInterface", {SyncMode::Off, SyncMode::Output});
    syncPrivate.addInterface(outputOnlyInterface);

    // getSyncInterfaces returns both, getAvailableSyncSources only the one that can be a source
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);

    const auto sourcesAfter = sync.getAvailableSyncSources();
    ASSERT_EQ(sourcesAfter.getCount(), 1u);
    ASSERT_TRUE(sourcesAfter.hasKey("ClockSyncInterface"));
    ASSERT_FALSE(sourcesAfter.hasKey("OutputOnlyInterface"));
}

TEST_F(SynchronizationTest, setSource)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization(ctx.getTypeManager(), "testDevice");

    const auto selectedSource = sync.getSource();
    ASSERT_TRUE(selectedSource.assigned());
    ASSERT_EQ(selectedSource.getId(), "ClockSyncInterface");
}

TEST_F(SynchronizationTest, AddTwoTheSameInterfaces)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    const auto newInterface = TestSyncInterface::Create(manager);
    ASSERT_NO_THROW(syncPrivate.addInterface(newInterface));
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);

    ASSERT_ANY_THROW(syncPrivate.addInterface(newInterface));
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);

    const auto newInterfaceWithTheSameName = TestSyncInterface::Create(manager);
    ASSERT_ANY_THROW(syncPrivate.addInterface(newInterfaceWithTheSameName));
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);
}

TEST_F(SynchronizationTest, SetSelectedSource)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    // Add another interface
    const auto newInterface = TestSyncInterface::Create(manager);
    syncPrivate.addInterface(newInterface);

    // Verify we have 2 interfaces
    const auto interfaces = sync.getSyncInterfaces();
    ASSERT_EQ(interfaces.getCount(), 2u);

    // Change selected source
    sync.setSource("TestInterface");
    
    const auto selectedSource = sync.getSource();
    ASSERT_EQ(selectedSource.getId(), "TestInterface");
    ASSERT_EQ(selectedSource.getMode(), SyncMode::Auto);
}

TEST_F(SynchronizationTest, SetSelectedSourceRevertsOldSource)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    // Default source is ClockSyncInterface, which has no Auto mode so it starts as Input
    const auto oldSource = sync.getSource();
    ASSERT_EQ(oldSource.getId(), "ClockSyncInterface");
    ASSERT_EQ(oldSource.getMode(), SyncMode::Input);

    const auto newInterface = TestSyncInterface::Create(manager);
    syncPrivate.addInterface(newInterface);
    sync.setSource("TestInterface");
    ASSERT_EQ(sync.getSource().getId(), "TestInterface");

    // The old source should have been demoted: Mode back to Off, and no longer selectable
    // as an input/output source (ModeOptions switched back to the output-only set)
    ASSERT_EQ(oldSource.getMode(), SyncMode::Off);

    const auto oldSourceModes = oldSource.getAvailableModes();
    ASSERT_TRUE(oldSourceModes.hasKey(static_cast<Int>(SyncMode::Off)));
    ASSERT_FALSE(oldSourceModes.hasKey(static_cast<Int>(SyncMode::Input)));
}

TEST_F(SynchronizationTest, SetSourceRollsBackOnFailure)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    // An interface that can never act as a source (no Input/Auto mode available)
    const auto outputOnlyInterface = TestSyncInterface::Create(manager, "OutputOnlyInterface", {SyncMode::Off, SyncMode::Output});
    syncPrivate.addInterface(outputOnlyInterface);

    const auto originalSource = sync.getSource();
    ASSERT_EQ(originalSource.getId(), "ClockSyncInterface");
    ASSERT_EQ(originalSource.getMode(), SyncMode::Input);

    ASSERT_ANY_THROW(sync.setSource("OutputOnlyInterface"));

    // Original source should be fully restored, not left demoted mid-switch
    ASSERT_EQ(sync.getSource().getId(), "ClockSyncInterface");
    ASSERT_EQ(sync.getSource().getMode(), SyncMode::Input);

    const auto restoredModes = sync.getSource().getAvailableModes();
    ASSERT_TRUE(restoredModes.hasKey(static_cast<Int>(SyncMode::Input)));
}

TEST_F(SynchronizationTest, SetSourceUnknownNameFails)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");

    const auto originalSource = sync.getSource();
    ASSERT_EQ(originalSource.getId(), "ClockSyncInterface");
    ASSERT_EQ(originalSource.getMode(), SyncMode::Input);

    ASSERT_ANY_THROW(sync.setSource("DoesNotExist"));

    // Failing to resolve the requested interface should leave the current source untouched
    ASSERT_EQ(sync.getSource().getId(), "ClockSyncInterface");
    ASSERT_EQ(sync.getSource().getMode(), SyncMode::Input);
}

TEST_F(SynchronizationTest, AddInterfaceFailsAfterAttached)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    // Attach the Synchronization component to a parent, the same way Device does internally
    auto parent = PropertyObject();
    parent.addProperty(ObjectProperty("Sync", PropertyObject()));
    parent.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("Sync", sync);

    const auto newInterface = TestSyncInterface::Create(manager);
    ASSERT_ERROR_CODE_EQ(syncPrivate->addInterface(newInterface), OPENDAQ_ERR_INVALID_OPERATION);
}

TEST_F(SynchronizationTest, GetSourceReferenceDomainId)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization(ctx.getTypeManager(), "testDevice");

    ListPtr<IString> referenceDomainId;
    ASSERT_ERROR_CODE_EQ(sync->getReferenceDomainIds(&referenceDomainId), OPENDAQ_SUCCESS);
    ASSERT_EQ(referenceDomainId.getCount(), 1u);
    ASSERT_EQ(referenceDomainId[0], "local:testDevice");
}

TEST_F(SynchronizationTest, ClockSyncInterfaceReferenceDomainIdEmptyWhenNotSource)
{
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);

    // ClockSyncInterface starts out as the selected source
    const DictPtr<IString, ISyncInterface> interfaces = sync.getSyncInterfaces();
    const SyncInterfacePtr clockSyncInterface = interfaces.get("ClockSyncInterface");
    ASSERT_EQ(clockSyncInterface.getReferenceDomainId(), "local:testDevice");

    // Demote it by switching the source to another interface
    const auto newInterface = TestSyncInterface::Create(manager);
    syncPrivate.addInterface(newInterface);
    sync.setSource("TestInterface");

    ASSERT_EQ(clockSyncInterface.getReferenceDomainId(), "");

    ListPtr<IString> referenceDomainIds;
    ASSERT_ERROR_CODE_EQ(sync->getReferenceDomainIds(&referenceDomainIds), OPENDAQ_SUCCESS);
    ASSERT_EQ(referenceDomainIds.getCount(), 0u);
}

TEST_F(SynchronizationTest, SyncInterfaceGetId)
{
    const auto ctx = NullContext();

    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface");
    ASSERT_EQ(syncInterface.getId(), "MyInterface");
}

TEST_F(SynchronizationTest, SyncInterfaceCanBeSourceTrue)
{
    const auto ctx = NullContext();

    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface", {SyncMode::Off, SyncMode::Input});
    ASSERT_TRUE(syncInterface.canBeSource());
}

TEST_F(SynchronizationTest, SyncInterfaceCanBeSourceFalse)
{
    const auto ctx = NullContext();

    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface", {SyncMode::Off, SyncMode::Output});
    ASSERT_FALSE(syncInterface.canBeSource());
}

TEST_F(SynchronizationTest, SyncInterfaceGetReferenceDomainId)
{
    const auto ctx = NullContext();

    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface");
    ASSERT_EQ(syncInterface.getReferenceDomainId(), "");
}

TEST_F(SynchronizationTest, SyncInterfaceGetSyncType)
{
    const auto ctx = NullContext();

    // Defaults to "" unless a subclass overrides it (e.g. via setSyncType in its own constructor)
    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface");
    ASSERT_EQ(syncInterface.getSyncType(), "");
}

TEST_F(SynchronizationTest, SyncInterfaceSetSyncType)
{
    const auto ctx = NullContext();

    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface");
    auto* impl = dynamic_cast<TestSyncInterface*>(syncInterface.getObject());

    // Sync type is intrinsic to an interface and never changes at runtime, so setSyncType is
    // meant to be called once, from the constructor of the concrete interface class.
    impl->setSyncType("gps");

    ASSERT_EQ(syncInterface.getSyncType(), "gps");
}

TEST_F(SynchronizationTest, SyncInterfaceProperties)
{
    const auto ctx = NullContext();

    const auto syncInterface = TestSyncInterface::Create(ctx.getTypeManager(), "MyInterface");
    const auto propObj = syncInterface.asPtr<IPropertyObject>(true);

    // Check Name property
    ASSERT_EQ(propObj.getPropertyValue("Id"), "MyInterface");

    // Check Mode property (default is Off)
    ASSERT_EQ(propObj.getPropertySelectionValue("Configuration.Mode"), "Off");

    // Check ReferenceDomainId property
    ASSERT_EQ(propObj.getPropertyValue("ReferenceDomainId"), "");

    // Check status container entries
    ASSERT_EQ(syncInterface.getStatusContainer().getStatus("SynchronizationSourceStatus").getValue(), "Off");
}

// =====================================================
// PtpSyncInterfaceBaseImpl Tests
// =====================================================

class TestPtpSyncInterface : public PtpSyncInterfaceBaseImpl
{
public:
    using Super = PtpSyncInterfaceBaseImpl;

    explicit TestPtpSyncInterface(const TypeManagerPtr& manager)
        : Super(manager)
    {
    }

    using Super::createPortProporties;
    using Super::setProfileOptions;
    using Super::setTransportProtocolOptions;
    using Super::setPortDelayMechanismOptions;
    using Super::setPortSyncStatus;

    static SyncInterfacePtr Create(const TypeManagerPtr& manager)
    {
        return createWithImplementation<ISyncInterface, TestPtpSyncInterface>(manager);
    }
};

using PtpSyncInterfaceTest = testing::Test;

TEST_F(PtpSyncInterfaceTest, Create)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    ASSERT_TRUE(iface.assigned());
}

TEST_F(PtpSyncInterfaceTest, GetId)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    ASSERT_EQ(iface.getId(), "PtpSyncInterface");
}

TEST_F(PtpSyncInterfaceTest, GetSyncType)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    ASSERT_EQ(iface.getSyncType(), "ptp");
}

TEST_F(PtpSyncInterfaceTest, GetReferenceDomainId)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    ASSERT_EQ(iface.getReferenceDomainId(), "");
}

TEST_F(PtpSyncInterfaceTest, DefaultMode)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    ASSERT_EQ(iface.getMode(), SyncMode::Off);
    const auto propObj = iface.asPtr<IPropertyObject>(true);
    ASSERT_EQ(propObj.getPropertyValue("Configuration.Mode"), SyncMode::Off);
}

TEST_F(PtpSyncInterfaceTest, DefaultPtpConfigurationProperties)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    const auto configuration = iface.getConfiguration();

    ASSERT_EQ(configuration.getPropertyValue("Profile"),            "None");
    ASSERT_EQ(configuration.getPropertyValue("TwoStepFlag"),        True);
    ASSERT_EQ(configuration.getPropertyValue("DomainNumber"),       0);
    ASSERT_EQ(configuration.getPropertyValue("UtcOffset"),          37);
    ASSERT_EQ(configuration.getPropertyValue("Priority1"),          128);
    ASSERT_EQ(configuration.getPropertyValue("Priority2"),          128);
    ASSERT_EQ(configuration.getPropertyValue("TransportProtocol"),  "IEEE802_3");
}

TEST_F(PtpSyncInterfaceTest, CreatePortProperties)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());

    impl->createPortProporties("eth0");

    // Status container entry should exist
    ASSERT_EQ(iface.getStatusContainer().getStatus("eth0").getValue(), "Off");

    // Configuration port entry should exist
    const auto configuration = iface.getConfiguration();
    const PropertyObjectPtr portConfig = configuration.getPropertyValue("PortConfiguration.eth0");
    ASSERT_TRUE(portConfig.assigned());
    ASSERT_EQ(portConfig.getPropertySelectionValue("Mode"),   "Off");
    ASSERT_EQ(portConfig.getPropertyValue("DelayMechanism"),  "E2E");
    ASSERT_EQ(portConfig.getPropertyValue("LogSyncInterval"), 0);
}

TEST_F(PtpSyncInterfaceTest, PerPortStatus)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto statusContainer = iface.getStatusContainer();

    impl->createPortProporties("eth0");
    impl->createPortProporties("eth1");

    // Each port starts Off, as a status container entry keyed by port name
    ASSERT_EQ(statusContainer.getStatus("eth0").getValue(), "Off");
    ASSERT_EQ(statusContainer.getStatus("eth1").getValue(), "Off");

    // Updating one port's status leaves the other port untouched
    impl->setPortSyncStatus("eth0", SyncRoleStatus::Input, "Client");

    ASSERT_EQ(statusContainer.getStatus("eth0").getValue(), "Input");
    ASSERT_EQ(statusContainer.getStatusMessage("eth0"), "Client");

    ASSERT_EQ(statusContainer.getStatus("eth1").getValue(), "Off");
}

TEST_F(PtpSyncInterfaceTest, SetProfileOptions)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto configuration = iface.getConfiguration();

    const auto newOptions = List<IString>("Custom1", "Custom2");
    impl->setProfileOptions(newOptions);

    const ListPtr<IString> stored = configuration.getPropertyValue("ProfileOptions");
    ASSERT_EQ(stored.getCount(), 2u);
    ASSERT_EQ(stored[0], "Custom1");
    ASSERT_EQ(stored[1], "Custom2");
}

TEST_F(PtpSyncInterfaceTest, SetTransportProtocolOptions)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto configuration = iface.getConfiguration();

    const auto newOptions = List<IString>("UDP_IPV4");
    impl->setTransportProtocolOptions(newOptions);

    const ListPtr<IString> stored = configuration.getPropertyValue("TransportProtocolOptions");
    ASSERT_EQ(stored.getCount(), 1u);
    ASSERT_EQ(stored[0], "UDP_IPV4");
}

TEST_F(PtpSyncInterfaceTest, PortModeFollowsInterfaceModeAutomatically)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto configuration = iface.getConfiguration();

    impl->createPortProporties("eth0");
    impl->createPortProporties("eth1");

    const PropertyObjectPtr portConfig0 = configuration.getPropertyValue("PortConfiguration.eth0");
    const PropertyObjectPtr portConfig1 = configuration.getPropertyValue("PortConfiguration.eth1");

    // Initially a port can only be Off
    const DictPtr<IInteger, IString> initialOptions = portConfig0.getPropertyValue("ModeOptions");
    ASSERT_EQ(initialOptions.getCount(), 3u);
    ASSERT_TRUE(initialOptions.hasKey(static_cast<Int>(PortSyncMode::Off)));

    // Switching the interface to Output makes ports selectable as Output too, but a port
    // that is still Off is left untouched
    iface.setMode(SyncMode::Output);

    const DictPtr<IInteger, IString> outputOptions0 = portConfig0.getPropertyValue("ModeOptions");
    ASSERT_TRUE(outputOptions0.hasKey(static_cast<Int>(PortSyncMode::Off)));
    ASSERT_TRUE(outputOptions0.hasKey(static_cast<Int>(PortSyncMode::Output)));
    ASSERT_FALSE(outputOptions0.hasKey(static_cast<Int>(PortSyncMode::Auto)));
    ASSERT_EQ(portConfig0.getPropertySelectionValue("Mode"), "Off");
    ASSERT_EQ(portConfig1.getPropertySelectionValue("Mode"), "Off");

    // Once a port actively participates (non-Off), its mode follows the interface automatically
    portConfig0.setPropertyValue("Mode", static_cast<Int>(PortSyncMode::Output));
    ASSERT_EQ(portConfig0.getPropertySelectionValue("Mode"), "Output");

    // Becoming the sync source switches the interface to Auto, which the active port follows
    iface.asPtr<ISyncInterfaceInternal>(true).setAsSource(True);

    const DictPtr<IInteger, IString> autoOptions0 = portConfig0.getPropertyValue("ModeOptions");
    ASSERT_TRUE(autoOptions0.hasKey(static_cast<Int>(PortSyncMode::Auto)));
    ASSERT_EQ(portConfig0.getPropertySelectionValue("Mode"), "Auto");

    // The port that stayed Off the whole time is still untouched
    ASSERT_EQ(portConfig1.getPropertySelectionValue("Mode"), "Off");
}

TEST_F(PtpSyncInterfaceTest, SetPortDelayMechanismOptions)
{
    const auto ctx = NullContext();

    const auto iface = TestPtpSyncInterface::Create(ctx.getTypeManager());
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto configuration = iface.getConfiguration();

    impl->createPortProporties("eth0");

    const auto newOptions = List<IString>("P2P");
    impl->setPortDelayMechanismOptions(newOptions);

    const PropertyObjectPtr portConfig = configuration.getPropertyValue("PortConfiguration.eth0");
    const ListPtr<IString> stored = portConfig.getPropertyValue("DelayMechanismOptions");
    ASSERT_EQ(stored.getCount(), 1u);
    ASSERT_EQ(stored[0], "P2P");
}

TEST_F(PtpSyncInterfaceTest, SaveLoad)
{
    // Create sync and add PtpSyncInterface with eth port
    const auto ctx = NullContext();
    const auto manager = ctx.getTypeManager();

    const auto sync = Synchronization(manager, "testDevice");
    const auto syncPrivate = sync.asPtr<ISynchronizationPrivate>(true);
    const auto updateableSync = sync.asPtr<IUpdatable>(true);

    const auto iface = TestPtpSyncInterface::Create(manager);
    const auto configuration = iface.getConfiguration();

    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    impl->createPortProporties("eth0");

    ASSERT_NO_THROW(syncPrivate.addInterface(iface));

    // Check default values
    ASSERT_EQ(sync.getSource().getId(), "ClockSyncInterface");
    ASSERT_EQ(iface.getMode(), SyncMode::Off);
    ASSERT_EQ(configuration.getPropertyValue("TransportProtocol"), "IEEE802_3");
    ASSERT_EQ(configuration.getPropertyValue("PortConfiguration.eth0.DelayMechanism"), "E2E");

    auto serializer = JsonSerializer();
    const ErrCode errCode = updateableSync->serializeForUpdate(serializer);
    ASSERT_ERROR_CODE_EQ(errCode, OPENDAQ_SUCCESS);

    // Do some changes after serialization to verify that deserialization will restore them
    sync.setSource("PtpSyncInterface");
    configuration.setPropertyValue("TransportProtocol", "UDP_IPV4");
    configuration.setPropertyValue("PortConfiguration.eth0.DelayMechanism", "P2P");

    // Check that changes are applied
    ASSERT_EQ(sync.getSource().getId(), "PtpSyncInterface");
    ASSERT_EQ(iface.getMode(), SyncMode::Auto);
    ASSERT_EQ(configuration.getPropertyValue("TransportProtocol"), "UDP_IPV4");
    ASSERT_EQ(configuration.getPropertyValue("PortConfiguration.eth0.DelayMechanism"), "P2P");

    // Do restore syncronization
    const auto deserializer = JsonDeserializer();
    deserializer.update(updateableSync, serializer.getOutput(), nullptr);

    // Verify that values are restored to defaults
    ASSERT_EQ(sync.getSource().getId(), "ClockSyncInterface");
    ASSERT_EQ(iface.getMode(), SyncMode::Off);
    ASSERT_EQ(configuration.getPropertyValue("TransportProtocol"), "IEEE802_3");
    ASSERT_EQ(configuration.getPropertyValue("PortConfiguration.eth0.DelayMechanism"), "E2E");
}

END_NAMESPACE_OPENDAQ
