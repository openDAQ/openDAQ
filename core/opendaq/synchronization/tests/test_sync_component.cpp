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
    SyncComponentPtr syncComponent = SyncComponent();
    ASSERT_TRUE(true);
}

TEST_F(SyncComponentTest, test_getSyncLocked)
{
    SyncComponentPtr syncComponent = SyncComponent();
    Bool syncLocked = false;
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_FALSE(syncLocked);
}

TEST_F(SyncComponentTest, test_setSyncLocked)
{
    SyncComponentPtr syncComponent = SyncComponent();
    Bool syncLocked = false;
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_FALSE(syncLocked);
    syncComponent->setSyncLocked(true);
    syncComponent->getSyncLocked(&syncLocked);
    ASSERT_TRUE(syncLocked);
}

TEST_F(SyncComponentTest, test_setSetSelectedSource)
{
    SyncComponentPtr syncComponent = SyncComponent();
    Int selectedSource = 0;
    syncComponent->getSelectedSource(&selectedSource);
    ASSERT_EQ(selectedSource, 0);
    syncComponent->setSelectedSource(1);
    syncComponent->getSelectedSource(&selectedSource);
    ASSERT_EQ(selectedSource, 1);
}

TEST_F(SyncComponentTest, test_addInterface)
{
    SyncComponentPtr syncComponent = SyncComponent();
    PropertyObjectPtr interface = PropertyObject();
    ASSERT_EQ(syncComponent->addInterface(interface), OPENDAQ_ERR_INVALID_ARGUMENT);
}

END_NAMESPACE_OPENDAQ
