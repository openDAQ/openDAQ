#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using ComponentsTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ
namespace docs_test_helpers
{
    static void traverseFolder(const FolderPtr& folder)
    {
        for (auto childComponent : folder.getItems())
        {
            if (auto childFolder = childComponent.asPtrOrNull<IFolder>(); childFolder.assigned())
                traverseFolder(childFolder);
        }
    }

}


// Corresponding document: Antora/modules/explanation/pages/components.adoc
TEST_F(ComponentsTest, FolderTraversal)
{
    auto instance = docs_test_helpers::setupInstance();
    ASSERT_NO_THROW(docs_test_helpers::traverseFolder(instance.getRootDevice()));
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
        if (id == "Sig" || id == "IO" || id == "FB" || id == "Dev")
            knownFolderCount++;
    }

    ASSERT_EQ(knownFolderCount, 4);
}

// Corresponding document: Antora/modules/explanation/pages/components.adoc
TEST_F(ComponentsTest, DeviceFolderTypes)
{
    auto instance = docs_test_helpers::setupInstance();
    auto items = instance.getRootDevice().getItems();

    auto inOut = instance.getInputsOutputsFolder();
    ASSERT_EQ(inOut.asPtrOrNull<IIoFolderConfig>().assigned(), true);
}

END_NAMESPACE_OPENDAQ
