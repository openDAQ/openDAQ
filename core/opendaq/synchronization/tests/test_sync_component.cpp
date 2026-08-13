#include <testutils/testutils.h>
#include <opendaq/sync_component_factory.h>
#include <opendaq/sync_component_private_ptr.h>
#include <opendaq/synchronization_internal_ptr.h>
#include <opendaq/sync_interface_base_impl.h>
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

class TestSyncInterface : public SyncInterfaceBaseImpl<>
{
public:
    using Super = SyncInterfaceBaseImpl<>;

    TestSyncInterface(const StringPtr& name, const std::vector<SyncMode> & availableModes)
        : Super(name, availableModes)
    {
    }

    static SyncInterfacePtr Create(const StringPtr& name = "TestInterface", const std::vector<SyncMode> & availableModes = {SyncMode::Off, SyncMode::Input, SyncMode::Output, SyncMode::Auto})
    {
        return createWithImplementation<ISyncInterface, TestSyncInterface>(name, availableModes);
    }
};

using SynchronizationTest = testing::Test;

TEST_F(SynchronizationTest, Create)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization();
    ASSERT_TRUE(sync.assigned());
}

TEST_F(SynchronizationTest, getSyncInterfaces)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization();

    const auto interfaces = sync.getSyncInterfaces();
    ASSERT_EQ(interfaces.getCount(), 1u);
    ASSERT_TRUE(interfaces.hasKey("ClockSyncInterface"));
}

TEST_F(SynchronizationTest, setSource)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization();

    const auto selectedSource = sync.getSource();
    ASSERT_TRUE(selectedSource.assigned());
    ASSERT_EQ(selectedSource.getName(), "ClockSyncInterface");
}

TEST_F(SynchronizationTest, AddTwoTheSameInterfaces)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization();
    const auto syncInternal = sync.asPtr<ISynchronizationInternal>(true);

    const auto newInterface = TestSyncInterface::Create();
    ASSERT_NO_THROW(syncInternal.addInterface(newInterface));
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);

    ASSERT_ANY_THROW(syncInternal.addInterface(newInterface));
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);

    const auto newInterfaceWithTheSameName = TestSyncInterface::Create();
    ASSERT_ANY_THROW(syncInternal.addInterface(newInterfaceWithTheSameName));
    ASSERT_EQ(sync.getSyncInterfaces().getCount(), 2u);
}

TEST_F(SynchronizationTest, SetSelectedSource)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization();
    const auto syncInternal = sync.asPtr<ISynchronizationInternal>(true);

    // Add another interface
    const auto newInterface = TestSyncInterface::Create("TestInterface", {SyncMode::Input});
    syncInternal.addInterface(newInterface);

    // Verify we have 2 interfaces
    const auto interfaces = sync.getSyncInterfaces();
    ASSERT_EQ(interfaces.getCount(), 2u);

    // Change selected source
    sync.setSource("TestInterface");
    
    const auto selectedSource = sync.getSource();
    ASSERT_EQ(selectedSource.getName(), "TestInterface");
    ASSERT_EQ(selectedSource.getMode(), SyncMode::Input);
}

TEST_F(SynchronizationTest, GetSourceReferenceDomainId)
{
    const auto ctx = NullContext();
    const auto sync = Synchronization();

    ListPtr<IString> referenceDomainId;
    ASSERT_ERROR_CODE_EQ(sync->getReferenceDomainIds(&referenceDomainId), OPENDAQ_SUCCESS);
    ASSERT_EQ(referenceDomainId.getCount(), 0);
}

TEST_F(SynchronizationTest, SyncInterfaceGetName)
{
    const auto syncInterface = TestSyncInterface::Create("MyInterface");
    ASSERT_EQ(syncInterface.getName(), "MyInterface");
}


TEST_F(SynchronizationTest, SyncInterfaceGetReferenceDomainId)
{
    const auto syncInterface = TestSyncInterface::Create("MyInterface");
    ASSERT_EQ(syncInterface.getReferenceDomainId(), "");
}

TEST_F(SynchronizationTest, SyncInterfaceProperties)
{
    const auto syncInterface = TestSyncInterface::Create("MyInterface");
    const auto propObj = syncInterface.asPtr<IPropertyObject>(true);

    // Check Name property
    ASSERT_EQ(propObj.getPropertyValue("Name"), "MyInterface");

    // Check Mode property (default is Off)
    ASSERT_EQ(propObj.getPropertySelectionValue("Mode"), "Off");

    // Check Status properties
    ASSERT_EQ(propObj.getPropertyValue("Status.Synchronized"), False);
    ASSERT_EQ(propObj.getPropertyValue("Status.ReferenceDomainId"), "");
}

// =====================================================
// PtpSyncInterfaceBaseImpl Tests
// =====================================================

class TestPtpSyncInterface : public PtpSyncInterfaceBaseImpl
{
public:
    using PtpSyncInterfaceBaseImpl::createPortProporties;
    using PtpSyncInterfaceBaseImpl::setProfileOptions;
    using PtpSyncInterfaceBaseImpl::setTransportProtocolOptions;
    using PtpSyncInterfaceBaseImpl::setPortModeOptions;
    using PtpSyncInterfaceBaseImpl::setPortsMode;
    using PtpSyncInterfaceBaseImpl::setPortDelayMechanismOptions;

    static SyncInterfacePtr Create()
    {
        return createWithImplementation<ISyncInterface, TestPtpSyncInterface>();
    }
};

using PtpSyncInterfaceTest = testing::Test;

TEST_F(PtpSyncInterfaceTest, Create)
{
    const auto iface = TestPtpSyncInterface::Create();
    ASSERT_TRUE(iface.assigned());
}

TEST_F(PtpSyncInterfaceTest, GetName)
{
    const auto iface = TestPtpSyncInterface::Create();
    ASSERT_EQ(iface.getName(), "PtpSyncInterface");
}

TEST_F(PtpSyncInterfaceTest, GetReferenceDomainId)
{
    const auto iface = TestPtpSyncInterface::Create();
    ASSERT_EQ(iface.getReferenceDomainId(), "");
}

TEST_F(PtpSyncInterfaceTest, DefaultMode)
{
    const auto iface = TestPtpSyncInterface::Create();
    ASSERT_EQ(iface.getMode(), SyncMode::Off);
    const auto propObj = iface.asPtr<IPropertyObject>(true);
    ASSERT_EQ(propObj.getPropertyValue("Mode"), SyncMode::Off);
}

TEST_F(PtpSyncInterfaceTest, DefaultPtpConfigurationProperties)
{
    const auto iface = TestPtpSyncInterface::Create();
    const auto propObj = iface.asPtr<IPropertyObject>(true);

    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.Profile"),            "None");
    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.TwoStepFlag"),        True);
    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.DomainNumber"),       0);
    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.UtcOffset"),          37);
    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.Priority1"),          128);
    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.Priority2"),          128);
    ASSERT_EQ(propObj.getPropertyValue("Parameters.PtpConfiguration.TransportProtocol"),  "IEEE802_3");
}


TEST_F(PtpSyncInterfaceTest, CreatePortProperties)
{
    const auto iface = TestPtpSyncInterface::Create();
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto propObj = iface.asPtr<IPropertyObject>(true);

    impl->createPortProporties("eth0");

    // Status port entry should exist
    const PropertyObjectPtr portStatus = propObj.getPropertyValue("Status.Ports.eth0");
    ASSERT_TRUE(portStatus.assigned());
    ASSERT_EQ(portStatus.getPropertyValue("State"), "Disabled");

    // Configuration port entry should exist
    const PropertyObjectPtr portConfig = propObj.getPropertyValue("Parameters.Ports.eth0");
    ASSERT_TRUE(portConfig.assigned());
    ASSERT_EQ(portConfig.getPropertyValue("Mode"),            "Off");
    ASSERT_EQ(portConfig.getPropertyValue("DelayMechanism"),  "E2E");
    ASSERT_EQ(portConfig.getPropertyValue("LogSyncInterval"), 0);
}

TEST_F(PtpSyncInterfaceTest, SetProfileOptions)
{
    const auto iface = TestPtpSyncInterface::Create();
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto propObj = iface.asPtr<IPropertyObject>(true);

    const auto newOptions = List<IString>("Custom1", "Custom2");
    impl->setProfileOptions(newOptions);

    const ListPtr<IString> stored = propObj.getPropertyValue("Parameters.PtpConfiguration.ProfileOptions");
    ASSERT_EQ(stored.getCount(), 2u);
    ASSERT_EQ(stored[0], "Custom1");
    ASSERT_EQ(stored[1], "Custom2");
}

TEST_F(PtpSyncInterfaceTest, SetTransportProtocolOptions)
{
    const auto iface = TestPtpSyncInterface::Create();
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto propObj = iface.asPtr<IPropertyObject>(true);

    const auto newOptions = List<IString>("UDP_IPV4");
    impl->setTransportProtocolOptions(newOptions);

    const ListPtr<IString> stored = propObj.getPropertyValue("Parameters.PtpConfiguration.TransportProtocolOptions");
    ASSERT_EQ(stored.getCount(), 1u);
    ASSERT_EQ(stored[0], "UDP_IPV4");
}

TEST_F(PtpSyncInterfaceTest, SetPortModeOptionsAndPortsMode)
{
    const auto iface = TestPtpSyncInterface::Create();
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto propObj = iface.asPtr<IPropertyObject>(true);

    impl->createPortProporties("eth0");
    impl->createPortProporties("eth1");

    const auto newModeOptions = List<IString>("Input", "Auto");
    impl->setPortModeOptions(newModeOptions);
    impl->setPortsMode("Input");

    const PropertyObjectPtr portConfig0 = propObj.getPropertyValue("Parameters.Ports.eth0");
    const PropertyObjectPtr portConfig1 = propObj.getPropertyValue("Parameters.Ports.eth1");

    ASSERT_EQ(portConfig0.getPropertyValue("Mode"), "Input");
    ASSERT_EQ(portConfig1.getPropertyValue("Mode"), "Input");
}

TEST_F(PtpSyncInterfaceTest, SetPortDelayMechanismOptions)
{
    const auto iface = TestPtpSyncInterface::Create();
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    const auto propObj = iface.asPtr<IPropertyObject>(true);

    impl->createPortProporties("eth0");

    const auto newOptions = List<IString>("P2P");
    impl->setPortDelayMechanismOptions(newOptions);

    const PropertyObjectPtr portConfig = propObj.getPropertyValue("Parameters.Ports.eth0");
    const ListPtr<IString> stored = portConfig.getPropertyValue("DelayMechanismOptions");
    ASSERT_EQ(stored.getCount(), 1u);
    ASSERT_EQ(stored[0], "P2P");
}

TEST_F(PtpSyncInterfaceTest, SaveLoad)
{
    // Create sync and add PtpSyncInterface with eth port
    const auto ctx = NullContext();
    const auto sync = Synchronization();
    const auto syncInternal = sync.asPtr<ISynchronizationInternal>(true);

    const auto iface = TestPtpSyncInterface::Create();
    const auto updateableIface = iface.asPtr<IUpdatable>(true);
    const auto ifacePropObj = iface.asPtr<IPropertyObject>(true);
    auto* impl = dynamic_cast<TestPtpSyncInterface*>(iface.getObject());
    impl->createPortProporties("eth0");

    ASSERT_NO_THROW(syncInternal.addInterface(iface));

    // Check default values
    ASSERT_EQ(sync.getSource().getName(), "ClockSyncInterface");
    ASSERT_EQ(iface.getMode(), SyncMode::Off);
    ASSERT_EQ(ifacePropObj.getPropertyValue("Parameters.PtpConfiguration.TransportProtocol"), "IEEE802_3");
    ASSERT_EQ(ifacePropObj.getPropertyValue("Parameters.Ports.eth0.DelayMechanism"), "E2E");

    // Do serialize for update
    auto serializer = JsonSerializer();
    const ErrCode errCode = updateableIface->serializeForUpdate(serializer);
    ASSERT_ERROR_CODE_EQ(errCode, OPENDAQ_SUCCESS);

    // Do some changes after serialization to verify that deserialization will restore them
    sync.setSource("PtpSyncInterface");
    ifacePropObj.setPropertyValue("Parameters.PtpConfiguration.TransportProtocol", "UDP_IPV4");
    ifacePropObj.setPropertyValue("Parameters.Ports.eth0.DelayMechanism", "P2P");

    // Check that changes are applied
    ASSERT_EQ(sync.getSource().getName(), "PtpSyncInterface");
    ASSERT_EQ(ifacePropObj.getPropertySelectionValue("Mode"), "Auto");
    ASSERT_EQ(ifacePropObj.getPropertyValue("Parameters.PtpConfiguration.TransportProtocol"), "UDP_IPV4");
    ASSERT_EQ(ifacePropObj.getPropertyValue("Parameters.Ports.eth0.DelayMechanism"), "P2P");

    // Do restore syncronization
    const auto deserializer = JsonDeserializer();
    deserializer.update(updateableIface, serializer.getOutput(), nullptr);

    // Verify that values are restored to defaults
    ASSERT_EQ(sync.getSource().getName(), "ClockSyncInterface");
    ASSERT_EQ(ifacePropObj.getPropertyValue("Mode"), "Off");
    ASSERT_EQ(ifacePropObj.getPropertyValue("Parameters.PtpConfiguration.TransportProtocol"), "IEEE802_3");
    ASSERT_EQ(ifacePropObj.getPropertyValue("Parameters.Ports.eth0.DelayMechanism"), "E2E");
}

END_NAMESPACE_OPENDAQ
