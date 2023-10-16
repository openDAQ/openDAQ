#include <gtest/gtest.h>
#include <coreobjects/owning_list_factory.h>
#include <coreobjects/property_object_factory.h>
#include "test_common.h"

using OwningListTest = testing::Test;

using namespace daq;

TEST_F(OwningListTest, Create)
{
    auto propObj = PropertyObject();

    ListPtr<IBaseObject> list;
    ASSERT_NO_THROW((list = OwningList<IBaseObject>(propObj)));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, PushBack)
{
    auto owner = PropertyObject();
    auto ownable = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    ASSERT_NO_THROW(list.pushBack(ownable));

    auto listOwnable = list[0];
    ASSERT_EQ(listOwnable, ownable);

    ASSERT_TRUE(isOwnedBy(listOwnable, owner));
}

TEST_F(OwningListTest, PushBackNonOwnable)
{
    auto owner = PropertyObject();
    auto obj = BaseObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    ASSERT_NO_THROW(list.pushBack(obj));

    auto listOwnable = list[0];
    ASSERT_EQ(listOwnable, obj);
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, PopBack)
{
    auto owner = PropertyObject();
    auto ownable = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    ASSERT_NO_THROW(list.pushBack(ownable));

    auto listOwnable = list[0];
    ASSERT_TRUE(isOwnedBy(listOwnable, owner));

    listOwnable = list.popBack();
    ASSERT_TRUE(isOwnedBy(listOwnable, nullptr));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, SetAt)
{
    auto owner = PropertyObject();
    auto ownable1 = PropertyObject();
    auto ownable2 = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(ownable1);

    ASSERT_NO_THROW(list.setItemAt(0, ownable2));

    GenericPropertyObjectPtr listOwnable = list.getItemAt(0);
    ASSERT_NE(listOwnable, ownable1);
    ASSERT_EQ(listOwnable, ownable2);

    ASSERT_TRUE(isOwnedBy(listOwnable, owner));
}

TEST_F(OwningListTest, SetAtNonOwnable)
{
    auto owner = PropertyObject();
    auto obj1 = BaseObject();
    auto obj2 = BaseObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(obj1);

    ASSERT_NO_THROW(list.setItemAt(0, obj2));
    ASSERT_EQ(list.getCount(), 1u);

    auto listObj = list.getItemAt(0);
    ASSERT_NE(listObj, obj1);
    ASSERT_EQ(listObj, obj2);
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, DeleteAt)
{
    auto owner = PropertyObject();
    auto ownable1 = PropertyObject();
    auto ownable2 = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(ownable1);

    ASSERT_NO_THROW(list.setItemAt(0, ownable2));

    GenericPropertyObjectPtr listOwnable = list.getItemAt(0);
    ASSERT_TRUE(isOwnedBy(listOwnable, owner));

    list.deleteAt(0);
    ASSERT_TRUE(isOwnedBy(listOwnable, nullptr));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, PushFront)
{
    auto owner = PropertyObject();
    auto ownable = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    ASSERT_NO_THROW(list.pushFront(ownable));

    auto listOwnable = list[0];
    ASSERT_EQ(listOwnable, ownable);

    ASSERT_TRUE(isOwnedBy(listOwnable, owner));
}

TEST_F(OwningListTest, PushFrontNonOwnable)
{
    auto owner = PropertyObject();
    auto obj = BaseObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    ASSERT_NO_THROW(list.pushFront(obj));

    auto listOwnable = list[0];
    ASSERT_EQ(listOwnable, obj);
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, PopFront)
{
    auto owner = PropertyObject();
    auto ownable = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    ASSERT_NO_THROW(list.pushFront(ownable));

    auto listOwnable = list[0];
    ASSERT_TRUE(isOwnedBy(listOwnable, owner));

    listOwnable = list.popFront();
    ASSERT_TRUE(isOwnedBy(listOwnable, nullptr));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, InsertAt)
{
    auto owner = PropertyObject();
    auto ownable1 = PropertyObject();
    auto ownable2 = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(ownable1);

    ASSERT_NO_THROW(list.insertAt(0, ownable2));
    ASSERT_EQ(list.getCount(), 2u);

    GenericPropertyObjectPtr listOwnable = list.getItemAt(0);
    ASSERT_EQ(listOwnable, ownable2);

    ASSERT_TRUE(isOwnedBy(listOwnable, owner));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, RemoveAt)
{
    auto owner = PropertyObject();
    auto ownable1 = PropertyObject();
    auto ownable2 = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(ownable1);

    PropertyObjectPtr listOwnable = list.getItemAt(0);
    ASSERT_TRUE(isOwnedBy(listOwnable, owner));

    listOwnable = list.removeAt(0);
    ASSERT_TRUE(isOwnedBy(listOwnable, nullptr));
}

TEST_F(OwningListTest, InsertAtNonOwnable)
{
    auto owner = PropertyObject();
    auto obj1 = BaseObject();
    auto obj2 = BaseObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(obj1);

    ASSERT_NO_THROW(list.insertAt(0, obj2));
    ASSERT_EQ(list.getCount(), 2u);

    auto listObj = list.getItemAt(0);
    ASSERT_EQ(listObj, obj2);
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningListTest, Clear)
{
    auto owner = PropertyObject();
    auto ownable1 = PropertyObject();
    auto ownable2 = PropertyObject();

    ListPtr<IBaseObject> list = OwningList<IBaseObject>(owner);
    list.pushBack(ownable1);
    list.pushBack(ownable2);

    PropertyObjectPtr listOwnable = list[0];
    ASSERT_TRUE(isOwnedBy(listOwnable, owner));

    listOwnable = list[1];
    ASSERT_TRUE(isOwnedBy(listOwnable, owner));

    list.clear();

    ASSERT_TRUE(isOwnedBy(ownable1, nullptr));
    ASSERT_TRUE(isOwnedBy(ownable2, nullptr));
}

TEST_F(OwningListTest, Inspectable)
{
    auto owner = PropertyObject();
    auto obj = OwningList<IBaseObject>(owner);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IList::Id);
}

TEST_F(OwningListTest, ImplementationName)
{
    auto owner = PropertyObject();
    auto obj = OwningList<IBaseObject>(owner);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::OwningListImpl");
}
