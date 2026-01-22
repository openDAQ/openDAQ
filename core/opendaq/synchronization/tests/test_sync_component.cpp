#include <testutils/testutils.h>
#include <opendaq/sync_component_factory.h>
#include <opendaq/sync_component_private_ptr.h>
#include <opendaq/sync_component2_internal_ptr.h>
#include <opendaq/sync_interface_base_impl.h>
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
    ASSERT_EQ(syncComponentPrivate->addInterface(interface1), OPENDAQ_SUCCESS);
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
    ASSERT_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);

    ASSERT_ERROR_CODE_EQ(syncComponentPrivate->removeInterface(String("SyncInterfaceBase")), OPENDAQ_ERR_NOTFOUND);
    ASSERT_EQ(syncComponentPrivate->removeInterface(String("InterfaceClockSync")), OPENDAQ_SUCCESS);
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
    ASSERT_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);

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
    ASSERT_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);


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
    ASSERT_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);

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

    ASSERT_EQ(syncComponentPrivate->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponentPrivate->addInterface(interface3), OPENDAQ_SUCCESS);

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
// SyncComponent2 and SyncInterface Tests
// =====================================================

using SyncComponent2Test = testing::Test;

TEST_F(SyncComponent2Test, Create)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");
    ASSERT_TRUE(syncComponent2.assigned());
}

TEST_F(SyncComponent2Test, GetInterfaces)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");

    const auto interfaces = syncComponent2.getInterfaces();
    ASSERT_EQ(interfaces.getCount(), 1u);
    ASSERT_TRUE(interfaces.hasKey("ClockSyncInterface"));
}

TEST_F(SyncComponent2Test, GetSelectedSource)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");

    const auto selectedSource = syncComponent2.getSelectedSource();
    ASSERT_TRUE(selectedSource.assigned());
    ASSERT_EQ(selectedSource.getName(), "ClockSyncInterface");
}

TEST_F(SyncComponent2Test, SetSelectedSource)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");
    const auto syncComponent2Internal = syncComponent2.asPtr<ISyncComponent2Internal>(true);

    // Add another interface
    const auto newInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("TestInterface");
    syncComponent2Internal.addInterface(newInterface);

    // Verify we have 2 interfaces
    const auto interfaces = syncComponent2.getInterfaces();
    ASSERT_EQ(interfaces.getCount(), 2u);

    // Change selected source
    syncComponent2.setSelectedSource("TestInterface");
    const auto selectedSource = syncComponent2.getSelectedSource();
    ASSERT_EQ(selectedSource.getName(), "TestInterface");
}

TEST_F(SyncComponent2Test, SetSelectedSourceNotFound)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");

    ASSERT_THROW(syncComponent2.setSelectedSource("NonExistent"), NotFoundException);
}

TEST_F(SyncComponent2Test, GetSourceSynced)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");

    Bool synced;
    ASSERT_EQ(syncComponent2->getSourceSynced(&synced), OPENDAQ_SUCCESS);
    ASSERT_FALSE(synced);
}

TEST_F(SyncComponent2Test, GetSourceReferenceDomainId)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");

    StringPtr referenceDomainId;
    ASSERT_EQ(syncComponent2->getSourceReferenceDomainId(&referenceDomainId), OPENDAQ_SUCCESS);
    ASSERT_EQ(referenceDomainId, "");
}

TEST_F(SyncComponent2Test, SyncInterfaceGetName)
{
    const auto syncInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("MyInterface");
    ASSERT_EQ(syncInterface.getName(), "MyInterface");
}

TEST_F(SyncComponent2Test, SyncInterfaceGetSynced)
{
    const auto syncInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("MyInterface");
    ASSERT_FALSE(syncInterface.getSynced());
}

TEST_F(SyncComponent2Test, SyncInterfaceGetReferenceDomainId)
{
    const auto syncInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("MyInterface");
    ASSERT_EQ(syncInterface.getReferenceDomainId(), "");
}

TEST_F(SyncComponent2Test, SyncInterfaceProperties)
{
    const auto syncInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("MyInterface");
    const auto propObj = syncInterface.asPtr<IPropertyObject>(true);

    // Check Name property
    ASSERT_EQ(propObj.getPropertyValue("Name"), "MyInterface");

    // Check Mode property (default is Off = 3)
    ASSERT_EQ(propObj.getPropertyValue("Mode"), 3);

    // Check Status properties
    ASSERT_EQ(propObj.getPropertyValue("Status.Synchronized"), False);
    ASSERT_EQ(propObj.getPropertyValue("Status.ReferenceDomainId"), "");
}

TEST_F(SyncComponent2Test, Serialization)
{
    const auto ctx = NullContext();
    const auto syncComponent2 = SyncComponent2(ctx, nullptr, "sync");
    const auto syncComponent2Internal = syncComponent2.asPtr<ISyncComponent2Internal>(true);

    // Add another interface
    const auto newInterface = createWithImplementation<ISyncInterface, SyncInterfaceBase>("TestInterface");
    syncComponent2Internal.addInterface(newInterface);

    // Change selected source
    syncComponent2.setSelectedSource("TestInterface");

    const auto serializer = JsonSerializer();
    syncComponent2.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(ctx, nullptr, nullptr, "sync");
    const SyncComponent2Ptr deserialized = deserializer.deserialize(serializedJson, deserializeContext);


    const auto interfaces = deserialized.getInterfaces();
    ASSERT_EQ(deserialized.getInterfaces().getCount(), 2u);
    ASSERT_TRUE(interfaces.hasKey("ClockSyncInterface"));
    ASSERT_TRUE(interfaces.hasKey("TestInterface"));
    ASSERT_EQ(deserialized.getSelectedSource().getName(), "TestInterface");
}

END_NAMESPACE_OPENDAQ
