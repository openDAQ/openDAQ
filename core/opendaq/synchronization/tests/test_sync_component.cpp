#include <opendaq/sync_component_factory.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_type_factory.h>

using SyncComponentTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(SyncComponentTest, testGetSyncLocked)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    Bool syncLocked = false;
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_FALSE(syncLocked);
}

TEST_F(SyncComponentTest, testSetSyncLocked)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    Bool syncLocked = false;
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_FALSE(syncLocked);
    syncComponent->setSyncLocked(true);
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_TRUE(syncLocked);
}

TEST_F(SyncComponentTest, testAddInterface)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    PropertyObjectPtr interface = PropertyObject();
    ASSERT_EQ(syncComponent->addInterface(interface), OPENDAQ_ERR_INVALID_ARGUMENT);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_SUCCESS);
}

TEST_F(SyncComponentTest, testRemoveInterface)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_SUCCESS);

    ASSERT_EQ(syncComponent->removeInterface(String("SyncInterfaceBase")), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->removeInterface(String("InterfaceClockSync")), OPENDAQ_ERR_NOTFOUND);
}

//#define TEST_DEBUG

TEST_F(SyncComponentTest, testAddInhertiedInterfaces)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    auto typeManager = ctx.getTypeManager();

#ifdef TEST_DEBUG
    ListPtr<IString> typesList = typeManager.getTypes();
    for (const auto& type : typesList)
    {
        std::cout << type << std::endl;
    }
#endif

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

#ifdef TEST_DEBUG
    std::cout << "Class Name: " << interface1.getClassName() << std::endl;
    std::cout << "Class Name: " << interface2.getClassName() << std::endl;
    std::cout << "Class Name: " << interface3.getClassName() << std::endl;
#endif

    //Assert that an interfaces with valid base class can be added
    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface3), OPENDAQ_SUCCESS);

    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_ERR_ALREADYEXISTS);
    ASSERT_EQ(syncComponent->addInterface(interface2), OPENDAQ_ERR_ALREADYEXISTS);
    ASSERT_EQ(syncComponent->addInterface(interface3), OPENDAQ_ERR_ALREADYEXISTS);
    //TBD: Assert GetInterfaces(String list) indeed includes the added interfaces


    //Assert that an interfaces with invalid base class cannot be added
    auto propClass = PropertyObjectClassBuilder("prop").build();
    typeManager.addType(propClass);
    PropertyObjectPtr interface4 = PropertyObject(typeManager, "prop");
    ASSERT_EQ(syncComponent->addInterface(interface4), OPENDAQ_ERR_INVALID_ARGUMENT);
}

TEST_F(SyncComponentTest, testSetSelectedSource)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();

    SyncComponentPtr syncComponent = SyncComponent(ctx);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface3), OPENDAQ_SUCCESS);

    Int selectedSource = 0;
    syncComponent->getSelectedSource(&selectedSource);
    ASSERT_EQ(selectedSource, 0);
    syncComponent->setSelectedSource(1);
    syncComponent->getSelectedSource(&selectedSource);
    ASSERT_EQ(selectedSource, 1);

    ASSERT_NO_THROW(syncComponent.setSelectedSource(2));
    ASSERT_EQ(syncComponent.getSelectedSource(), 2);

    // out of range
    ASSERT_ANY_THROW(syncComponent.setSelectedSource(3));
    ASSERT_EQ(syncComponent.getSelectedSource(), 2);
}

TEST_F(SyncComponentTest, testSelectedSourceListChanged)
{
    const auto ctx = daq::NullContext();
    auto typeManager = ctx.getTypeManager();

    SyncComponentPtr syncComponent = SyncComponent(ctx);

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface3), OPENDAQ_SUCCESS);

    auto interfaceNames = syncComponent.getInterfaceNames();
    ASSERT_EQ(interfaceNames.getCount(), 3);

    syncComponent.setSelectedSource(1);
    ASSERT_EQ(syncComponent->removeInterface(String("SyncInterfaceBase")), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent.getSelectedSource(), 0);

    interfaceNames = syncComponent.getInterfaceNames();
    ASSERT_EQ(interfaceNames.getCount(), 2);
    ASSERT_EQ(interfaceNames[0], "PtpSyncInterface");

    syncComponent.setSelectedSource(1);
    ASSERT_EQ(syncComponent->removeInterface(String("InterfaceClockSync")), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent.getSelectedSource(), 0);
    
    interfaceNames = syncComponent.getInterfaceNames();
    ASSERT_EQ(interfaceNames.getCount(), 1);
    ASSERT_EQ(interfaceNames[0], "PtpSyncInterface");
}


END_NAMESPACE_OPENDAQ