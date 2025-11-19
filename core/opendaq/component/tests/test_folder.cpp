#include <coretypes/objectptr.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/component.h>
#include <opendaq/folder_factory.h>
#include <opendaq/io_folder_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/tags_private_ptr.h>
#include <opendaq/folder_ptr.h>
#include <opendaq/folder_impl.h>

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
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.addItem(component1);
    ASSERT_EQ(folder.getItems().getCount(), 1u);

    daq::MockComponent::Strict component2;
    EXPECT_CALL(component2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp2"));
    EXPECT_CALL(component2.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component2.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
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
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.addItem(component1);
    ASSERT_EQ(folder.getItems().getCount(), 1u);

    daq::MockComponent::Strict component2;
    EXPECT_CALL(component2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp2"));
    EXPECT_CALL(component2.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component2.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
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
    EXPECT_CALL(component1.mock(), getGlobalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("folder/comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.addItem(component1);
    ASSERT_THROW(folder.addItem(component1), daq::DuplicateItemException);

    ASSERT_EQ(folder.getItems().getCount(), 1u);
}

TEST_F(FolderTest, Clear)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.addItem(component1);

    daq::MockComponent::Strict component2;
    EXPECT_CALL(component2.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp2"));
    EXPECT_CALL(component2.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component2.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.addItem(component2);

    folder.clear();
    ASSERT_EQ(folder.getItems().getCount(), 0u);
}

TEST_F(FolderTest, NotEmpty)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
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
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
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

TEST_F(FolderTest, SerializeAndDeserialize)
{
    const auto ctx = daq::NullContext();
    const auto folder = daq::Folder(ctx, nullptr, "folder");
    folder.setName("fld_name");
    folder.setDescription("fld_desc");
    folder.getTags().asPtr<daq::ITagsPrivate>().add("fld_tag");

    const auto component = daq::Component(ctx, nullptr, "component");

    component.setName("comp_name");
    component.setDescription("comp_desc");
    component.getTags().asPtr<daq::ITagsPrivate>().add("comp_tag");

    folder.addItem(component);

    const auto serializer = daq::JsonSerializer(daq::True);
    folder.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(ctx, nullptr, nullptr, "folder");

    const daq::FolderPtr newFolder = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(newFolder.getName(), folder.getName());
    ASSERT_EQ(newFolder.getDescription(), folder.getDescription());
    ASSERT_EQ(newFolder.getTags(), folder.getTags());

    ASSERT_EQ(newFolder.getItems()[0].getName(), component.getName());
    ASSERT_EQ(newFolder.getItems()[0].getDescription(), component.getDescription());
    ASSERT_EQ(newFolder.getItems()[0].getTags(), component.getTags());

    ASSERT_EQ(newFolder.getItems().getElementInterfaceId(), daq::IComponent::Id);

    const auto serializer2 = daq::JsonSerializer(daq::True);
    newFolder.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(FolderTest, BeginUpdateEndUpdate)
{
    const auto ctx = daq::NullContext();
    const auto folder = daq::Folder(ctx, nullptr, "folder");
    folder.addProperty(daq::StringPropertyBuilder("FolderProp", "-").build());

    const auto component = daq::Component(ctx, folder, "component");

    folder.addItem(component);
    component.addProperty(daq::StringPropertyBuilder("ComponentProp", "-").build());

    folder.beginUpdate();

    folder.setPropertyValue("FolderProp", "s");
    ASSERT_EQ(folder.getPropertyValue("FolderProp"), "-");

    component.setPropertyValue("ComponentProp", "cs");
    ASSERT_EQ(component.getPropertyValue("ComponentProp"), "-");

    folder.endUpdate();

    ASSERT_EQ(folder.getPropertyValue("FolderProp"), "s");
    ASSERT_EQ(component.getPropertyValue("ComponentProp"), "cs");
}

TEST_F(FolderTest, SetActive)
{
    auto folder = daq::Folder(context, nullptr, "folder");

    daq::MockComponent::Strict component1;
    EXPECT_CALL(component1.mock(), getLocalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("comp1"));
    EXPECT_CALL(component1.mock(), getVisible(_)).WillRepeatedly(daq::GetBool(true));
    EXPECT_CALL(component1.mock(), setParentActive(daq::True)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.addItem(component1);

    EXPECT_CALL(component1.mock(), setParentActive(daq::False)).WillOnce(Return(OPENDAQ_SUCCESS));
    folder.setActive(daq::False);
}

TEST_F(FolderTest, SetActiveNested)
{
    auto folder = daq::Folder(context, nullptr, "folder");
    auto folder1 = daq::Folder(context, folder, "folder1");
    folder.addItem(folder1);
    auto comp = daq::Component(context, folder1, "comp");
    folder1.addItem(comp);

    ASSERT_TRUE(comp.getActive());
    ASSERT_TRUE(comp.getLocalActive());
    folder.setActive(daq::False);
    ASSERT_FALSE(comp.getActive());
    ASSERT_TRUE(comp.getLocalActive());
    folder.setActive(daq::True);
    ASSERT_TRUE(comp.getActive());
    ASSERT_TRUE(comp.getLocalActive());
}

using FolderActiveTest = testing::TestWithParam<std::tuple<bool, bool, bool>>;

TEST_P(FolderActiveTest, SerializeAndDeserializeActive)
{
    const auto ctx = daq::NullContext();
    const auto folder0 = daq::Folder(ctx, nullptr, "folder");
    const auto folder1 = daq::Folder(ctx, folder0, "folder1");
    folder0.addItem(folder1);
    const auto component = daq::Component(ctx, folder1, "component");
    folder1.addItem(component);

    folder0.setActive(std::get<0>(GetParam()));
    folder1.setActive(std::get<1>(GetParam()));
    component.setActive(std::get<2>(GetParam()));

    const auto serializer = daq::JsonSerializer(daq::True);
    folder0.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(ctx, nullptr, nullptr, "folder0");

    const daq::FolderPtr newFolder0 = deserializer.deserialize(str1, deserializeContext, nullptr);
    ASSERT_EQ(folder0.getActive(), newFolder0.getActive());
    const auto newFolder1 = newFolder0.getItems()[0].asPtr<daq::IFolder>(true);
    ASSERT_EQ(folder1.getActive(), newFolder1.getActive());
    const auto newComponent = newFolder1.getItems()[0];
    ASSERT_EQ(component.getActive(), newComponent.getActive());
}

class CustomFolder : public daq::FolderImpl<>
{
public:
    using daq::FolderImpl<>::FolderImpl;

protected:
    void updateObject(const daq::SerializedObjectPtr& obj, const daq::BaseObjectPtr& context) override
    {
        daq::FolderImpl<>::updateObject(obj, context);

        auto serializedItems = this->getSerializedItems(obj);
        for (const auto& serializedItem : serializedItems)
        {
            const auto localId = serializedItem.first;
            const auto updatableComponent = this->items[localId].template asPtr<IUpdatable>(true);
            updatableComponent.updateInternal(serializedItem.second, context);
        }

    }
};

TEST_P(FolderActiveTest, UpdateActive)
{
    const auto ctx = daq::NullContext();
    const auto folder0 = daq::createWithImplementation<daq::IFolderConfig, CustomFolder>(ctx, nullptr, "folder0");
    const auto folder1 = daq::createWithImplementation<daq::IFolderConfig, CustomFolder>(ctx, folder0, "folder1");
    folder0.addItem(folder1);
    const auto component = daq::Component(ctx, folder1, "component");
    folder1.addItem(component);

    ASSERT_TRUE(folder0.getActive());
    ASSERT_TRUE(folder1.getActive());
    ASSERT_TRUE(component.getActive());

    folder0.setActive(std::get<0>(GetParam()));
    folder1.setActive(std::get<1>(GetParam()));
    component.setActive(std::get<2>(GetParam()));

    ASSERT_EQ(folder0.getLocalActive(), std::get<0>(GetParam()));
    ASSERT_EQ(folder1.getLocalActive(), std::get<1>(GetParam()));
    ASSERT_EQ(component.getLocalActive(), std::get<2>(GetParam()));

    ASSERT_EQ(folder0.getActive(), std::get<0>(GetParam()));
    ASSERT_EQ(folder1.getActive(), std::get<1>(GetParam()) && std::get<0>(GetParam()));
    ASSERT_EQ(component.getActive(), std::get<2>(GetParam()) && std::get<1>(GetParam()) && std::get<0>(GetParam()));

    const auto serializer = daq::JsonSerializer(daq::True);
    folder0.asPtr<daq::IUpdatable>(true).serializeForUpdate(serializer);
    const auto str1 = serializer.getOutput();

    folder0.setActive(true);
    folder1.setActive(true);
    component.setActive(true);
    ASSERT_TRUE(folder0.getLocalActive() && folder0.getActive());
    ASSERT_TRUE(folder1.getLocalActive() && folder1.getActive());
    ASSERT_TRUE(component.getLocalActive() && component.getActive());

    const auto deserializer = daq::JsonDeserializer();
    deserializer.update(folder0.asPtr<daq::IUpdatable>(true), str1, nullptr);

    ASSERT_EQ(folder0.getLocalActive(), std::get<0>(GetParam()));
    ASSERT_EQ(folder1.getLocalActive(), std::get<1>(GetParam()));
    ASSERT_EQ(component.getLocalActive(), std::get<2>(GetParam()));

    ASSERT_EQ(folder0.getActive(), std::get<0>(GetParam()));
    ASSERT_EQ(folder1.getActive(), std::get<1>(GetParam()) && std::get<0>(GetParam()));
    ASSERT_EQ(component.getActive(), std::get<2>(GetParam()) && std::get<1>(GetParam()) && std::get<0>(GetParam()));
}

static std::vector<std::tuple<bool, bool, bool>> GenerateAllPermutations()
{
    std::vector<std::tuple<bool, bool, bool>> permutations;
    for (int i = 0; i < 8; ++i)
    {
        bool a = (i & 4) != 0;
        bool b = (i & 2) != 0;
        bool c = (i & 1) != 0;
        permutations.emplace_back(a, b, c);
    }
    return permutations;
}

INSTANTIATE_TEST_CASE_P(
    FolderActiveTestInstance,
    FolderActiveTest,
    testing::ValuesIn(GenerateAllPermutations())
);

using namespace daq;

TEST_F(FolderTest, InheritedLocking1)
{
    const auto ctx = daq::NullContext();
    const auto folder = daq::Folder(ctx, nullptr, "folder");
    const auto folderInternal = folder.asPtr<IPropertyObjectInternal>();
    
    const auto folder1 = daq::Folder(ctx, folder, "folder");
    const auto folderInternal1 = folder1.asPtr<IPropertyObjectInternal>();
    folderInternal1.setLockingStrategy(LockingStrategy::InheritLock);
    folder.addItem(folder1);
    
    const auto component = daq::Component(ctx, folder1, "folder");
    const auto componentInternal = component.asPtr<IPropertyObjectInternal>();
    componentInternal.setLockingStrategy(LockingStrategy::InheritLock);
    folder1.addItem(component);

    auto propObj = PropertyObject();
    auto objInternal = propObj.asPtr<IPropertyObjectInternal>();
    objInternal.setLockingStrategy(LockingStrategy::InheritLock);
    component.addProperty(ObjectProperty("child", propObj));

    auto propObj1 = PropertyObject();
    auto objInternal1 = propObj1.asPtr<IPropertyObjectInternal>();

    objInternal1.setLockingStrategy(LockingStrategy::InheritLock);
    propObj.addProperty(ObjectProperty("child", propObj1));

    folderInternal.enableCoreEventTrigger();
    
    ASSERT_EQ(folderInternal.getLockingStrategy(), LockingStrategy::OwnLock);
    ASSERT_EQ(folderInternal1.getLockingStrategy(), LockingStrategy::InheritLock);
    ASSERT_EQ(componentInternal.getLockingStrategy(), LockingStrategy::InheritLock);
    ASSERT_EQ(objInternal.getLockingStrategy(), LockingStrategy::InheritLock);
    ASSERT_EQ(objInternal1.getLockingStrategy(), LockingStrategy::InheritLock);

    auto mutex = folderInternal.getMutex();
    ASSERT_EQ(mutex, folderInternal1.getMutex());
    ASSERT_EQ(mutex, componentInternal.getMutex());
    ASSERT_EQ(mutex, objInternal.getMutex());
    ASSERT_EQ(mutex, objInternal1.getMutex());

    {
        auto lg = objInternal1.getLockGuard();
        ASSERT_FALSE(mutex.tryLock());
    }

    {
        auto lg = folderInternal.getRecursiveLockGuard();
        auto lg1 = folderInternal1.getRecursiveLockGuard();
        auto lg2 = componentInternal.getRecursiveLockGuard();
        auto lg3 = objInternal.getRecursiveLockGuard();
        auto lg4 = objInternal1.getRecursiveLockGuard();
    }

    ASSERT_EQ(folderInternal, componentInternal.getMutexOwner());
    ASSERT_EQ(folderInternal, objInternal1.getMutexOwner());
}

TEST_F(FolderTest, InheritedLocking2)
{
    const auto ctx = daq::NullContext();
    const auto folder = daq::Folder(ctx, nullptr, "folder");
    const auto folderInternal = folder.asPtr<IPropertyObjectInternal>();
    
    const auto folder1 = daq::Folder(ctx, folder, "folder");
    const auto folderInternal1 = folder1.asPtr<IPropertyObjectInternal>();
    folderInternal1.setLockingStrategy(LockingStrategy::ForwardOwnerLockOwn);
    folder.addItem(folder1);
    
    const auto component = daq::Component(ctx, folder1, "folder");
    const auto componentInternal = component.asPtr<IPropertyObjectInternal>();
    componentInternal.setLockingStrategy(LockingStrategy::InheritLock);
    folder1.addItem(component);

    auto propObj = PropertyObject();
    auto objInternal = propObj.asPtr<IPropertyObjectInternal>();
    objInternal.setLockingStrategy(LockingStrategy::ForwardOwnerLockOwn);
    component.addProperty(ObjectProperty("child", propObj));

    auto propObj1 = PropertyObject();
    auto objInternal1 = propObj1.asPtr<IPropertyObjectInternal>();

    objInternal1.setLockingStrategy(LockingStrategy::InheritLock);
    propObj.addProperty(ObjectProperty("child", propObj1));

    folderInternal.enableCoreEventTrigger();
    
    ASSERT_EQ(folderInternal.getLockingStrategy(), LockingStrategy::OwnLock);
    ASSERT_EQ(folderInternal1.getLockingStrategy(), LockingStrategy::ForwardOwnerLockOwn);
    ASSERT_EQ(componentInternal.getLockingStrategy(), LockingStrategy::InheritLock);
    ASSERT_EQ(objInternal.getLockingStrategy(), LockingStrategy::ForwardOwnerLockOwn);
    ASSERT_EQ(objInternal1.getLockingStrategy(), LockingStrategy::InheritLock);

    auto mutex = folderInternal.getMutex();
    ASSERT_NE(mutex, folderInternal1.getMutex());
    ASSERT_EQ(mutex, componentInternal.getMutex());
    ASSERT_NE(mutex, objInternal.getMutex());
    ASSERT_EQ(mutex, objInternal1.getMutex());

    {
        auto lg = objInternal1.getLockGuard();
        ASSERT_FALSE(mutex.tryLock());

        auto mutex1 = folderInternal1.getMutex();
        auto mutex2 = objInternal.getMutex();
        ASSERT_TRUE(mutex1.tryLock());
        ASSERT_TRUE(mutex2.tryLock());

        mutex1.unlock();
        mutex2.unlock();
    }

    {
        auto lg = folderInternal.getRecursiveLockGuard();
        auto lg1 = folderInternal1.getRecursiveLockGuard();
        auto lg2 = componentInternal.getRecursiveLockGuard();
        auto lg3 = objInternal.getRecursiveLockGuard();
        auto lg4 = objInternal1.getRecursiveLockGuard();
    }

    ASSERT_EQ(folderInternal, componentInternal.getMutexOwner());
    ASSERT_EQ(folderInternal, objInternal1.getMutexOwner());
    ASSERT_EQ(folderInternal, folderInternal1.getMutexOwner());
    ASSERT_EQ(folderInternal, objInternal.getMutexOwner());
}
