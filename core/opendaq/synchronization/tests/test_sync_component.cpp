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

END_NAMESPACE_OPENDAQ
