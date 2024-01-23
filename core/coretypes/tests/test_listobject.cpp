#include <coretypes/coretypes.h>
#include <coretypes/list_element_type.h>
#include <coretypes/cloneable.h>
#include <gtest/gtest.h>

using namespace daq;

using ListObjectTest = testing::Test;

TEST_F(ListObjectTest, Pushing)
{
    auto list = List<IBaseObject>();
    ASSERT_EQ(list.getCount(), 0u);

    list.pushBack(BaseObject());
    ASSERT_EQ(list.getCount(), 1u);

    list.pushBack(BaseObject());
    ASSERT_EQ(list.getCount(), 2u);

    auto obj = BaseObject();

    list.pushBack(obj);
    ASSERT_EQ(list.getCount(), 3u);

    auto obj1 = list.getItemAt(2);

    ASSERT_TRUE(obj == obj1);
}

TEST_F(ListObjectTest, PushingFront)
{
    auto list = List<IBaseObject>();
    ASSERT_EQ(list.getCount(), 0u);

    list.pushFront(BaseObject());
    ASSERT_EQ(list.getCount(), 1u);

    list.pushFront(BaseObject());
    ASSERT_EQ(list.getCount(), 2u);

    auto obj = BaseObject();

    list.pushFront(obj);
    ASSERT_EQ(list.getCount(), 3u);

    auto obj1 = list.getItemAt(0);

    ASSERT_TRUE(obj == obj1);
}

TEST_F(ListObjectTest, Popping)
{
    auto list = List<IBaseObject>();
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());
    auto obj = BaseObject();
    list.pushBack(obj);

    auto obj1 = list.popBack();
    ASSERT_EQ(list.getCount(), 2u);

    ASSERT_TRUE(obj1 == obj);
}

TEST_F(ListObjectTest, PoppingFront)
{
    auto list = List<IBaseObject>();
    list.pushFront(BaseObject());
    list.pushFront(BaseObject());
    auto obj = BaseObject();
    list.pushFront(obj);

    auto obj1 = list.popFront();
    ASSERT_EQ(list.getCount(), 2u);

    ASSERT_TRUE(obj1 == obj);
}

TEST_F(ListObjectTest, SetItem)
{
    auto list = List<IBaseObject>();
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());
    list.pushBack(nullptr);
    list.pushBack(nullptr);

    list.setItemAt(1, BaseObject());
    list.setItemAt(2, nullptr);
    list.setItemAt(3, BaseObject());
    list.setItemAt(4, nullptr);

    ASSERT_EQ(list.getCount(), 5u);

    auto item2 = list.getItemAt(2);
    ASSERT_EQ(item2, nullptr);
}

TEST_F(ListObjectTest, DeleteRemove)
{
    auto list = List<IBaseObject>();
    list.pushBack(BaseObject());
    auto obj = BaseObject();
    list.pushBack(obj);
    list.pushBack(BaseObject());
    list.pushBack(nullptr);
    list.pushBack(nullptr);

    BaseObjectPtr obj1 = list.removeAt(1);
    list.deleteAt(1);

    ASSERT_EQ(obj, obj1);

    ASSERT_EQ(list.getCount(), 3u);
    list.deleteAt(2);
    obj = list.removeAt(1);
    ASSERT_TRUE(obj == nullptr);
}

TEST_F(ListObjectTest, Clear)
{
    auto list = List<IBaseObject>();
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());

    list.clear();

    ASSERT_EQ(list.getCount(), 0u);
}

TEST_F(ListObjectTest, Insert)
{
    auto list = List<IBaseObject>();
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());
    list.pushBack(BaseObject());

    auto obj = BaseObject();
    list->insertAt(1, obj);

    auto obj1 = list.getItemAt(1);
    ASSERT_EQ(obj, obj1);

    list.insertAt(3, nullptr);
    obj1 = list.getItemAt(3);
    ASSERT_TRUE(obj1 == nullptr);

    SizeT size;
    ASSERT_EQ(list->getCount(&size), OPENDAQ_SUCCESS);
    ASSERT_EQ(size, 5u);
}

TEST_F(ListObjectTest, UnassignedList)
{
    ListObjectPtr<IList, IBaseObject> list;
    ASSERT_ANY_THROW(list.getCount());
    ASSERT_ANY_THROW(list.pushBack(BaseObject()));
    ASSERT_ANY_THROW(list.pushFront(BaseObject()));
    ASSERT_ANY_THROW(list.popFront());
    ASSERT_ANY_THROW(list.popBack());
    ASSERT_ANY_THROW(list.deleteAt(0));
    ASSERT_ANY_THROW(list.removeAt(0));
    ASSERT_ANY_THROW(list.insertAt(0, BaseObject()));
    ASSERT_ANY_THROW(list.clear());
    ASSERT_ANY_THROW(list.getItemAt(0));
    ASSERT_ANY_THROW(list.setItemAt(0, BaseObject()));
}

TEST_F(ListObjectTest, OutOfRange)
{
    ListPtr<IBaseObject> list = List<IBaseObject>();
    ASSERT_ANY_THROW(list.deleteAt(0));
    ASSERT_ANY_THROW(list.removeAt(0));
    ASSERT_ANY_THROW(list.insertAt(0, BaseObject()));
    ASSERT_ANY_THROW(list.getItemAt(0));
    ASSERT_ANY_THROW(list.setItemAt(0, BaseObject()));
    ASSERT_ANY_THROW(list.popFront());
    ASSERT_ANY_THROW(list.popBack());
}

TEST_F(ListObjectTest, NoSmartPtr)
{
    auto list = List<IBaseObject>();
    ASSERT_THROW(checkErrorInfo(list->getItemAt(0, nullptr)), ArgumentNullException);
    ASSERT_THROW(checkErrorInfo(list->popBack(nullptr)), ArgumentNullException);
    ASSERT_THROW(checkErrorInfo(list->popFront(nullptr)), ArgumentNullException);
    ASSERT_THROW(checkErrorInfo(list->createStartIterator(nullptr)), ArgumentNullException);
    ASSERT_THROW(checkErrorInfo(list->createEndIterator(nullptr)), ArgumentNullException);

    list->pushBack(BaseObject());
    BaseObjectPtr obj;
    checkErrorInfo(list->getItemAt(0, obj.addressOf()));
}

TEST_F(ListObjectTest, CoreType)
{
    auto listObj = List<IBaseObject>();
    enum CoreType coreType = listObj.getCoreType();

    ASSERT_EQ(coreType, ctList);
}

TEST_F(ListObjectTest, CreateStatic)
{
    auto obj1 = BaseObject();
    auto obj2 = BaseObject();
    auto list = List<IBaseObject>(obj1, obj2);
    ASSERT_EQ(list.getCount(), 2u);
    ASSERT_EQ(list.getItemAt(0), obj1);
    ASSERT_EQ(list.getItemAt(1), obj2);
}

TEST_F(ListObjectTest, CreateStaticRValue)
{
    auto obj1 = BaseObject();
    auto list = List<IBaseObject>(obj1, BaseObject());
    ASSERT_EQ(list.getCount(), 2u);
    ASSERT_EQ(list.getItemAt(0), obj1);
    ASSERT_NE(list.getItemAt(1), nullptr);
}

TEST_F(ListObjectTest, Clone)
{
    auto list1 = List<IBaseObject>("foo", 1, 1.21);

    BaseObjectPtr list2;
    list1.asPtr<ICloneable>()->clone(&list2);

    ASSERT_EQ(list1, list2);
}

TEST_F(ListObjectTest, CloneListOfLists)
{
    auto list1 = List<IList>(List<IString>("foo"), List<IInteger>(1), List<IFloat>(1.123));

    BaseObjectPtr list2;
    list1.asPtr<ICloneable>()->clone(&list2);

    ASSERT_EQ(list1, list2);
}

TEST_F(ListObjectTest, ToString)
{
    auto list = List<IBaseObject>(1, 2, 3);
    std::string str = list;

    ASSERT_EQ(str, "[ 1, 2, 3 ]");
}

TEST_F(ListObjectTest, ToStringWithCylces)
{
    auto list = List<IBaseObject>();
    list.pushBack(list);
    std::string str = list;
    ASSERT_EQ(str, "[ [ ... ] ]");
    list.dispose();

    auto list1 = List<IBaseObject>(1, 2, 3);
    auto list2 = List<IBaseObject>("a", "b", "c");
    list1.pushBack(list2);
    list2.pushBack(list1);
    std::string str1 = list1;

    ASSERT_EQ(str1, "[ 1, 2, 3, [ ""a"", ""b"", ""c"", [ ... ] ] ]");
    list1.dispose();
    list2.dispose();
}

TEST_F(ListObjectTest, StringItemChangeString)
{
    auto str = List<IString>("test", "test2", "test3");
    str[0] = "bababab";

    ASSERT_NE(str[0], "bababab");
}

TEST_F(ListObjectTest, StringItemChangeInt)
{
    auto istr = List<Int>(0, 1, 2);
    istr[0] = 3;

    ASSERT_NE(Int(istr[0]), 3);
}

TEST_F(ListObjectTest, Freeze)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_TRUE(list.isFrozen());
}

TEST_F(ListObjectTest, SetItemAtWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.setItemAt(0, nullptr), FrozenException);
}

TEST_F(ListObjectTest, PushBackWhenFrozen)
{
    ListObjectPtr<IList, IBaseObject> list = List<IBaseObject>();
    list.freeze();

    IBaseObject* obj = nullptr;
    ErrCode errCode = list->pushBack(obj);

    ASSERT_EQ(errCode, OPENDAQ_ERR_FROZEN);
}

TEST_F(ListObjectTest, PushBackRValueWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.pushBack(nullptr), FrozenException);
}

TEST_F(ListObjectTest, PushFrontWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.pushFront(nullptr), FrozenException);
}

TEST_F(ListObjectTest, PopBackWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.popBack(), FrozenException);
}

TEST_F(ListObjectTest, PopFrontWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.popFront(), FrozenException);
}

TEST_F(ListObjectTest, InsertAtWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.insertAt(0, nullptr), FrozenException);
}

TEST_F(ListObjectTest, RemoveAtWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.removeAt(0), FrozenException);
}

TEST_F(ListObjectTest, DeleteAtWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.deleteAt(0), FrozenException);
}

TEST_F(ListObjectTest, ClearWhenFrozen)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_THROW(list.clear(), FrozenException);
}

TEST_F(ListObjectTest, IsFrozenFalse)
{
    auto list = List<IBaseObject>();

    ASSERT_FALSE(list.isFrozen());
}

TEST_F(ListObjectTest, DoubleFreeze)
{
    auto list = List<IBaseObject>();
    list.freeze();

    ASSERT_NO_THROW(list.freeze());
    ASSERT_EQ(list.asPtr<IFreezable>(true)->freeze(), OPENDAQ_IGNORED);
}

TEST_F(ListObjectTest, SerializeId)
{
    auto list = List<IBaseObject>();
    ASSERT_THROW(list.getSerializeId(), NotImplementedException);
}

TEST_F(ListObjectTest, GetItemWithSubscriptionOperator)
{
    auto list = List<Int>(1, 2, 3);

    ASSERT_EQ(list[0], 1);
    ASSERT_EQ(list[1], 2);
    ASSERT_EQ(list[2], 3);
}

TEST_F(ListObjectTest, ElementSmartPointerType)
{
    auto list = List<IString>("e1", "e2", "e3");
    auto el = list.getItemAt(0);

    // element must be StringPtr
    ASSERT_EQ(el.getLength(), 2u);
}

TEST_F(ListObjectTest, FromVectorInt)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5, 6 };
    auto list = ListPtr<IInteger>::FromVector(vec);

    ASSERT_EQ(list.getCount(), 6u);
    ASSERT_EQ(list[0], 1);
    ASSERT_EQ(list[1], 2);
    ASSERT_EQ(list[2], 3);
    ASSERT_EQ(list[3], 4);
    ASSERT_EQ(list[4], 5);
    ASSERT_EQ(list[5], 6);
}

TEST_F(ListObjectTest, FromVectorIntImplicit)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5, 6 };
    ListPtr<IInteger> list = vec;

    ASSERT_EQ(list.getCount(), 6u);
    ASSERT_EQ(list[0], 1);
    ASSERT_EQ(list[1], 2);
    ASSERT_EQ(list[2], 3);
    ASSERT_EQ(list[3], 4);
    ASSERT_EQ(list[4], 5);
    ASSERT_EQ(list[5], 6);
}

TEST_F(ListObjectTest, FromVectorIntInitializerList)
{
    ListPtr<IInteger> list = { 1, 2, 3, 4, 5, 6 };

    ASSERT_EQ(list.getCount(), 6u);
    ASSERT_EQ(list[0], 1);
    ASSERT_EQ(list[1], 2);
    ASSERT_EQ(list[2], 3);
    ASSERT_EQ(list[3], 4);
    ASSERT_EQ(list[4], 5);
    ASSERT_EQ(list[5], 6);
}

TEST_F(ListObjectTest, FromVectorString)
{
    std::vector<std::string> strings = { "a", "b", "c" };
    auto list2 = ListPtr<IString>::FromVector(strings);

    // Should not compile!
    // auto list3 = ListPtr<IInteger>::FromVector(strings);
}

TEST_F(ListObjectTest, FromVectorStringImplicit)
{
    std::vector<std::string> strings = { "a", "b", "c" };
    ListPtr<IString> list2 = strings;

    ASSERT_EQ(list2.getCount(), 3u);
    ASSERT_EQ(list2[0], "a");
    ASSERT_EQ(list2[1], "b");
    ASSERT_EQ(list2[2], "c");
}

TEST_F(ListObjectTest, FromVectorStringInitializerList)
{
    ListPtr<IString> list2 = { "a", "b", "c" };

    ASSERT_EQ(list2.getCount(), 3u);
    ASSERT_EQ(list2[0], "a");
    ASSERT_EQ(list2[1], "b");
    ASSERT_EQ(list2[2], "c");
}

TEST_F(ListObjectTest, FromIterators)
{
    std::vector<std::string> vector = {"a", "b", "c"};
    ListPtr<IString> list(vector.cbegin(), vector.cend());

    ASSERT_EQ(list.getCount(), 3u);
    ASSERT_EQ(list[0], "a");
    ASSERT_EQ(list[1], "b");
    ASSERT_EQ(list[2], "c");
}

TEST_F(ListObjectTest, FromIterator)
{
    ListPtr<IString> list1 = { "a", "b", "c" };

    ListPtr<IString> list2(list1.begin(), list1.end());

    ASSERT_EQ(list2.getCount(), 3u);
    ASSERT_EQ(list2[0], "a");
    ASSERT_EQ(list2[1], "b");
    ASSERT_EQ(list2[2], "c");

    ListPtr<IString> list3(++list1.begin(), list1.end());

    ASSERT_EQ(list3.getCount(), 2u);
    ASSERT_EQ(list3[0], "b");
    ASSERT_EQ(list3[1], "c");

    ListPtr<IString> list4(list1.begin(), ++list1.begin());

    ASSERT_EQ(list4.getCount(), 1u);
    ASSERT_EQ(list4[0], "a");
}

TEST_F(ListObjectTest, FloatListFromInts)
{
    ListPtr<IFloat> floats = List<IFloat>(1, 2, 3);
    ASSERT_EQ(floats[0], 1);
    ASSERT_EQ(floats[1], 2);
    ASSERT_EQ(floats[2], 3);
}

enum class TestingEnum
{
    Zero,
    One,
    Two,
    Three
};

TEST_F(ListObjectTest, SetEnumClass)
{
    ListPtr<IBaseObject> list = List<IBaseObject>(3, 2);
    list.setItemAt(0, TestingEnum::One);

    ASSERT_EQ(list[0], 1);
}

TEST_F(ListObjectTest, IteratorConstCompare)
{
    ListPtr<IString> list = { "a", "b", "c" };

    const auto it = list.begin();
    ASSERT_TRUE(it != list.end());
}

TEST_F(ListObjectTest, IteratorAssignmentOperator)
{
    ListPtr<IString> list = { "a", "b", "c" };

    auto it = list.begin();
    it = it; // self-assignment
    it = list.end();
}

TEST_F(ListObjectTest, StdFindIf)
{
    ListPtr<IString> list = {"a", "b", "c"};

    auto it = std::find_if(list.begin(), list.end(), [](const StringPtr& el) -> bool
    {
        bool eq = el == "b";
        return eq;
    });
    ASSERT_NE(it, list.end());
    ASSERT_EQ(*it, "b");
}

TEST_F(ListObjectTest, FactoryNonInterfaceInt64)
{
    auto testInt64 = List<Int>();
    testInt64.pushBack(int64_t(5));
    auto elInt64 = testInt64[0];
}

TEST_F(ListObjectTest, FactoryNonInterfaceBoolTrue)
{
    auto testBool = List<bool>();

    bool boolTrue = true;
    testBool.pushBack(boolTrue);
    float trueFromList = testBool[0];
    ASSERT_EQ(boolTrue, trueFromList);

    bool boolFalse = true;
    testBool.pushBack(boolFalse);

    bool falseFromList = testBool[0];
    ASSERT_EQ(boolFalse, falseFromList);
}

TEST_F(ListObjectTest, FactoryNonInterfaceBoolFalse)
{
    auto testBool = List<bool>();

    bool boolFalse = false;
    testBool.pushBack(boolFalse);

    bool falseFromList = testBool[0];
    ASSERT_EQ(boolFalse, falseFromList);
}

TEST_F(ListObjectTest, FactoryNonInterfaceUInt64)
{
    auto testUnsigned = List<uint64_t>();
    auto minU = std::numeric_limits<uint64_t>::min();
    testUnsigned.pushBack(minU);

    uint64_t minFromListU = testUnsigned[0];
    ASSERT_EQ(minU, minFromListU);
}

TEST_F(ListObjectTest, FactoryNonInterfaceDouble)
{
    auto testDouble = List<Float>();
    double minD = std::numeric_limits<Float>::min();
    testDouble.pushBack(minD);

    auto minFromListD = testDouble[0];
    ASSERT_EQ(minD, minFromListD);
}

TEST_F(ListObjectTest, FactoryNonInterfaceString)
{
    auto testString = List<std::string>();
    std::string test = "test";
    testString.pushBack(test);

    auto testFromList = testString[0];
    ASSERT_EQ(test, testFromList);
}

TEST_F(ListObjectTest, FactoryNonInterfaceFloat)
{
    auto testFloat = List<float>();
    float minF = std::numeric_limits<float>::min();

    testFloat.pushBack(minF);
    float minFromListF = testFloat[0];
    ASSERT_EQ(minF, minFromListF);
}

TEST_F(ListObjectTest, SmartPtrNonInterfaceInt64)
{
    ListPtr<Int> test;
}

TEST_F(ListObjectTest, SmartPtrNonInterfaceBool)
{
    ListPtr<bool> test;
}

TEST_F(ListObjectTest, SmartPtrNonInterfaceUInt64)
{
    ListPtr<uint64_t> test;
}

TEST_F(ListObjectTest, SmartPtrNonInterfaceDouble)
{
    ListPtr<double> test;
}

TEST_F(ListObjectTest, SmartPtrNonInterfaceFloat)
{
    ListPtr<float> test;
}

TEST_F(ListObjectTest, SmartPtrNonInterfaceString)
{
    ListPtr<std::string> test;
}

TEST_F(ListObjectTest, EqualsEmpty)
{
    auto list1 = List<IBaseObject>();
    auto list2 = List<IBaseObject>();

    Bool eq{false};
    list1->equals(list2, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(ListObjectTest, EqualsSameSize)
{
    auto list1 = List<IBaseObject>();
    list1.pushBack("Tesla");
    list1.pushBack(10);

    auto list2 = List<IBaseObject>();
    list2.pushBack("Tesla");
    list2.pushBack(10);

    auto list3 = List<IBaseObject>();
    list3.pushBack("Tesla");
    list3.pushBack(11);


    Bool eq{false};
    list1->equals(list2, &eq);
    ASSERT_TRUE(eq);

    list1->equals(list3, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(ListObjectTest, EqualsDifferentSize)
{
    auto list1 = List<IBaseObject>();
    list1.pushBack("Tesla");
    list1.pushBack(10);
    list1.pushBack(20);

    auto list2 = List<IBaseObject>();
    list2.pushBack("Tesla");
    list2.pushBack(10);

    Bool eq{false};
    list1->equals(list2, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(ListObjectTest, Inspectable)
{
    auto obj = List<IBaseObject>();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IList::Id);
}

TEST_F(ListObjectTest, ImplementationName)
{
    auto obj = List<IBaseObject>();

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::ListImpl");
}

static IntfID getInterfaceId(const ObjectPtr<IBaseObject>& obj)
{
    IntfID iid{};
    ErrCode errCode = obj.asPtrOrNull<IListElementType>(true)->getElementInterfaceId(&iid);
    checkErrorInfo(errCode);

    return iid;
}

TEST_F(ListObjectTest, ElementTypeDefault)
{
    ListPtr<IBaseObject> obj;
    checkErrorInfo(createList(&obj));

    auto iid = getInterfaceId(obj);
    ASSERT_EQ(iid, IUnknown::Id);
}

TEST_F(ListObjectTest, ElementTypeNative)
{
    // Element type in template arguments can be proper types
    auto obj1 = List<Int>();
    auto iid = getInterfaceId(obj1);
    ASSERT_EQ(iid, IInteger::Id);
}

TEST_F(ListObjectTest, ElementTypeInterface)
{
    // Element type in template arguments can be interfaces
    auto obj1 = List<IInteger>();
    auto iid = getInterfaceId(obj1);
    ASSERT_EQ(iid, IInteger::Id);
}

TEST_F(ListObjectTest, ElementTypeDifferentFromFirstArgument)
{
    // Type still Int even if first argument is double
    auto objB = List<Int>(1.0);
    auto iid = getInterfaceId(objB);
    ASSERT_EQ(iid, IInteger::Id);
}

TEST_F(ListObjectTest, ElementTypePtr)
{
    auto obj1 = List<IInteger>();
    ASSERT_EQ(obj1.getElementInterfaceId(), IInteger::Id);
}

TEST_F(ListObjectTest, IteratorStartType)
{
    auto obj1 = List<IInteger>(1, 2, 3);

    ObjectPtr<IIterator> iter;
    ErrCode errCode = obj1->createStartIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IListElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID id{};
    errCode = elType->getElementInterfaceId(&id);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(id, IInteger::Id);
}

TEST_F(ListObjectTest, IteratorEndType)
{
    auto obj1 = List<IInteger>(1, 2, 3);

    ObjectPtr<IIterator> iter;
    ErrCode errCode = obj1->createEndIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IListElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID id{};
    errCode = elType->getElementInterfaceId(&id);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(id, IInteger::Id);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IList", "daq");

TEST_F(ListObjectTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IList::Id);
}

TEST_F(ListObjectTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IList>(), "{E7866BCC-0563-5504-B61B-A8116B614D8F}");
}
