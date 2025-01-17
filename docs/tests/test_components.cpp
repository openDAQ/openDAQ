#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using ComponentsTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/components.adoc
TEST_F(ComponentsTest, FolderTraversal)
{
    auto instance = docs_test_helpers::setupInstance();

    const auto searchFilter = search::Recursive(search::Any());
    ASSERT_NO_THROW(instance.getSignals(searchFilter));
}

// Corresponding document: Antora/modules/explanation/pages/components.adoc
TEST_F(ComponentsTest, DeviceFolders)
{
    auto instance = docs_test_helpers::setupInstance();
    auto items = instance.getRootDevice().getItems();
    int knownFolderCount = 0;
    for (auto item : items)
    {
        auto id = item.getLocalId();
        if (id == "Sig" || id == "IO" || id == "FB" || id == "Dev"  || id == "Synchronization")
            knownFolderCount++;
    }

    ASSERT_EQ(knownFolderCount, 5);
}

// Corresponding document: Antora/modules/explanation/pages/components.adoc
TEST_F(ComponentsTest, DeviceFolderTypes)
{
    auto instance = docs_test_helpers::setupInstance();
    auto items = instance.getRootDevice().getItems();

    auto inOut = instance.getInputsOutputsFolder();
    ASSERT_EQ(inOut.asPtrOrNull<IIoFolderConfig>().assigned(), true);
}

// Corresponding document: Antora/modules/background_info/pages/components.adoc
TEST_F(ComponentsTest, ComponentStatuses)
{
    auto instance = docs_test_helpers::setupInstance();

    const auto scalingFb = instance.addFunctionBlock("RefFBModuleScaling");
    auto statuses = scalingFb.getStatusContainer().getStatuses();

    auto status = scalingFb.getStatusContainer().getStatus("ComponentStatus");
    auto message = scalingFb.getStatusContainer().getStatusMessage("ComponentStatus");

    ASSERT_GT(statuses.getCount(), 0);
    ASSERT_TRUE(status == Enumeration("ComponentStatusType", "Ok", instance.getContext().getTypeManager()));
    ASSERT_EQ(status, Enumeration("ComponentStatusType", "Ok", instance.getContext().getTypeManager()));
    ASSERT_EQ(message, "");
}

END_NAMESPACE_OPENDAQ
