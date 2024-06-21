#include <opendaq/sync_component_factory.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_type_factory.h>

using SyncComponentTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(SyncComponentTest, test1)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);
    ASSERT_TRUE(true);
}


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

TEST_F(SyncComponentTest, test_setSetSelectedSource)
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

TEST_F(SyncComponentTest, test_addInterface)
{
    const auto ctx = daq::NullContext();
    SyncComponentPtr syncComponent = SyncComponent(ctx);

    auto typeManager = ctx.getTypeManager();
    ListPtr<IString> typesList = typeManager.getTypes();

    for (const auto& type : typesList)
    {
        std::cout << type << std::endl;
    }

    PropertyObjectPtr interface1 = PropertyObject(typeManager, "SyncInterfaceBase");
    PropertyObjectPtr interface2 = PropertyObject(typeManager, "PtpSyncInterface");
    PropertyObjectPtr interface3 = PropertyObject(typeManager, "InterfaceClockSync");

    StringPtr className = interface1.getClassName();
    std::cout << "Class Name: " << interface1.getClassName() << std::endl;
    std::cout << "Class Name: " << interface2.getClassName() << std::endl;
    std::cout << "Class Name: " << interface3.getClassName() << std::endl;

    //ASSERT_EQ(syncComponent->addInterface(interface), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_EQ(syncComponent->addInterface(interface2), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_EQ(syncComponent->addInterface(interface3), OPENDAQ_ERR_INVALID_ARGUMENT);
}

END_NAMESPACE_OPENDAQ
