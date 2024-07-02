#include <opendaq/sync_component_factory.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_type_factory.h>

using SyncComponentTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(SyncComponentTest, test_getSyncLocked)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    Bool syncLocked = false;
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_FALSE(syncLocked);
}

TEST_F(SyncComponentTest, test_setSyncLocked)
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

TEST_F(SyncComponentTest, test_setSelectedSource)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    Int selectedSource = 0;
    syncComponent->getSelectedSource(&selectedSource);
    ASSERT_EQ(selectedSource, 0);
    syncComponent->setSelectedSource(1);
    syncComponent->getSelectedSource(&selectedSource);
    ASSERT_EQ(selectedSource, 1);
}

TEST_F(SyncComponentTest, test_addInterfaceInvalidArgument)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    PropertyObjectPtr interface = PropertyObject();
    ASSERT_EQ(syncComponent->addInterface(interface), OPENDAQ_ERR_INVALID_ARGUMENT);
}

//#define TEST_DEBUG

TEST_F(SyncComponentTest, DISABLED_addInterface)
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
    syncComponent->addInterface(interface1);
    syncComponent->addInterface(interface2);
    syncComponent->addInterface(interface3);
    ASSERT_EQ(syncComponent->addInterface(interface1), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface2), OPENDAQ_SUCCESS);
    ASSERT_EQ(syncComponent->addInterface(interface3), OPENDAQ_SUCCESS);
    //TBD: Assert GetInterfaces(String list) indeed includes the added interfaces


    //Assert that an interfaces with invalid base class cannot be added
    auto propClass = PropertyObjectClassBuilder("prop").build();
    typeManager.addType(propClass);
    PropertyObjectPtr interface4 = PropertyObject(typeManager, "prop");
    ASSERT_EQ(syncComponent->addInterface(interface4), OPENDAQ_ERR_INVALID_ARGUMENT);
}

END_NAMESPACE_OPENDAQ