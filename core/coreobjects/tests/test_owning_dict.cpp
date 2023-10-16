#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/owning_dict_factory.h>
#include <coreobjects/property_object_impl.h>
#include "test_common.h"

using namespace daq;

using OwningDictTest = testing::Test;

TEST_F(OwningDictTest, Create)
{
    auto owner = PropertyObject();

    DictPtr<IBaseObject, IBaseObject> dict;
    ASSERT_NO_THROW((dict = OwningDict<IBaseObject, IBaseObject>(owner)));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningDictTest, Set)
{
    GenericPropertyObjectPtr owner = PropertyObject();
    GenericPropertyObjectPtr ownable = createWithImplementation<IPropertyObject, PropertyObjectImpl>();

    DictPtr<IString, IBaseObject> dict = OwningDict<IString, IBaseObject>(owner);
    dict.set("owned", ownable);
    ASSERT_EQ(dict.getCount(), 1u);

    GenericPropertyObjectPtr dictOwnable = dict.get("owned");
    ASSERT_EQ(ownable, dictOwnable);

    auto* impl = dynamic_cast<PropertyObjectImpl*>(dictOwnable.getObject());

    auto ownableOwnerRef = impl->getOwner();
    ASSERT_TRUE(ownableOwnerRef.assigned());

    GenericPropertyObjectPtr ownableOwner;
    ASSERT_NO_THROW((ownableOwner = ownableOwnerRef.getRef()));
    ASSERT_EQ(owner, ownableOwner);
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningDictTest, Remove)
{
    GenericPropertyObjectPtr owner = PropertyObject();
    GenericPropertyObjectPtr ownable = PropertyObject();

    DictPtr<IString, IBaseObject> dict = OwningDict<IString, IBaseObject>(owner);
    dict.set("owned", ownable);
    ASSERT_EQ(dict.getCount(), 1u);

    PropertyObjectPtr dictOwnable = dict.get("owned");
    ASSERT_TRUE(isOwnedBy(dictOwnable, owner));

    dictOwnable = dict.remove("owned");
    ASSERT_TRUE(isOwnedBy(dictOwnable, nullptr));
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningDictTest, Delete)
{
    GenericPropertyObjectPtr owner = PropertyObject();
    GenericPropertyObjectPtr ownable = PropertyObject();

    DictPtr<IString, IBaseObject> dict = OwningDict<IString, IBaseObject>(owner);
    dict.set("owned", ownable);
    ASSERT_EQ(dict.getCount(), 1u);

    GenericPropertyObjectPtr dictOwnable = dict.get("owned");
    ASSERT_TRUE(isOwnedBy(dictOwnable, owner));

    dict.deleteItem("owned");
    ASSERT_TRUE(isOwnedBy(dictOwnable, nullptr));
}

TEST_F(OwningDictTest, SetNonOwnable)
{
    auto owner = PropertyObject();
    auto obj = BaseObject();

    DictPtr<IString, IBaseObject> dict = OwningDict<IString, IBaseObject>(owner);
    ASSERT_NO_THROW(dict.set("owned", obj));
    ASSERT_EQ(dict.getCount(), 1u);

    auto dictObj = dict["owned"];
    ASSERT_EQ(obj, dictObj);
}

// See is isOwnedBy() comment in test_common.h
TEST_F(OwningDictTest, Clear)
{
    GenericPropertyObjectPtr owner = PropertyObject();
    GenericPropertyObjectPtr ownable = PropertyObject();
    GenericPropertyObjectPtr ownable2 = PropertyObject();

    DictPtr<IString, IBaseObject> dict = OwningDict<IString, IBaseObject>(owner);
    dict.set("owned", ownable);
    dict.set("owned2", ownable2);

    ASSERT_EQ(dict.getCount(), 2u);

    ASSERT_TRUE(isOwnedBy(ownable, owner));
    ASSERT_TRUE(isOwnedBy(ownable2, owner));

    dict.clear();

    ASSERT_TRUE(isOwnedBy(ownable, nullptr));
    ASSERT_TRUE(isOwnedBy(ownable2, nullptr));
}

TEST_F(OwningDictTest, Inspectable)
{
    auto owner = PropertyObject();
    auto obj = OwningDict<IString, IBaseObject>(owner);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IDict::Id);
}

TEST_F(OwningDictTest, ImplementationName)
{
    auto owner = PropertyObject();
    auto obj = OwningDict<IString, IBaseObject>(owner);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::OwningDictImpl");
}
