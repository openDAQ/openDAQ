#include <coretypes/objectptr.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/component.h>
#include <opendaq/folder_factory.h>
#include <opendaq/io_folder_factory.h>

using namespace testing;

class FolderTest : public Test
{
protected:
    MockContext::Strict context;
};

TEST_F(FolderTest, LocalID)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    ASSERT_EQ(folder.getLocalId(), "folder");
}

TEST_F(FolderTest, DefaultItemID)
{
    auto folder = daq::Folder(context, nullptr, "folder");
    auto items = folder.getItems();

    ASSERT_EQ(daq::IComponent::Id, items.getElementInterfaceId());
}

TEST_F(FolderTest, ItemID)
{
    auto folder = daq::Folder<daq::IFolder>(context, nullptr, "folder");
    auto items = folder.getItems();

    ASSERT_EQ(daq::IFolder::Id, items.getElementInterfaceId());
}

TEST_F(FolderTest, DefaultName)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    ASSERT_EQ(folder.getName(), "folder");
}

TEST_F(FolderTest, Name)
{
    auto folder = daq::Folder(context, nullptr, "folder");
    folder.setName("fld");

    ASSERT_EQ(folder.getName(), "fld");
}

TEST_F(FolderTest, Items)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component1);
    ASSERT_EQ(folder.getItems().getCount(), 1u);

    daq::MockComponent::Strict component2;
    EXPECT_CALL(component2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp2"));
    EXPECT_CALL(component2.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component2);
    ASSERT_EQ(folder.getItems().getCount(), 2u);

    ASSERT_EQ(folder.getItems()[0], component1.ptr);
    ASSERT_EQ(folder.getItems()[1], component2.ptr);

    ASSERT_EQ(folder.getItem("comp1"), component1.ptr);
    ASSERT_EQ(folder.getItem("comp2"), component2.ptr);
    ASSERT_THROW(folder.getItem("x"), daq::NotFoundException);

    folder.removeItem(component1.ptr);
    folder.removeItemWithLocalId("comp2");
    ASSERT_THROW(folder.removeItemWithLocalId("xxx"), daq::NotFoundException);
    //ASSERT_EQ(folder.getItems().getCount(), 0u);
}

TEST_F(FolderTest, Remove)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component1);
    ASSERT_EQ(folder.getItems().getCount(), 1u);

    daq::MockComponent::Strict component2;
    EXPECT_CALL(component2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp2"));
    EXPECT_CALL(component2.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component2);
    ASSERT_EQ(folder.getItems().getCount(), 2u);

    folder.remove();
    ASSERT_EQ(folder.getItems().getCount(), 0u);
}

TEST_F(FolderTest, Duplicate)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component1);
    ASSERT_THROW(folder.addItem(component1), daq::DuplicateItemException);

    ASSERT_EQ(folder.getItems().getCount(), 1u);
}

TEST_F(FolderTest, Clear)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component1);

    daq::MockComponent::Strict component2;
    EXPECT_CALL(component2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp2"));
    EXPECT_CALL(component2.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component2);

    folder.clear();
    ASSERT_EQ(folder.getItems().getCount(), 0u);
}

TEST_F(FolderTest, NotEmpty)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component1);

    ASSERT_FALSE(folder.isEmpty());
}

TEST_F(FolderTest, Empty)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    ASSERT_TRUE(folder.isEmpty());
}

TEST_F(FolderTest, HasItem)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly( daq::GetBool(true));
    folder.addItem(component1);

    ASSERT_FALSE(folder.hasItem("comp2"));
    ASSERT_TRUE(folder.hasItem("comp1"));
}

TEST_F(FolderTest, StandardProperties)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = daq::Folder(context, nullptr, "foo");

    component.setName(name);
    component.setDescription(desc);

    ASSERT_EQ(component.getName(), name);
    ASSERT_EQ(component.getDescription(), desc);
}

TEST_F(FolderTest, IOStandardProperties)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = daq::IoFolder(context, nullptr, "foo");

    component.setName(name);
    component.setDescription(desc);

    ASSERT_EQ(component.getName(), name);
    ASSERT_EQ(component.getDescription(), desc);
}
