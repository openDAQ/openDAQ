#include <gtest/gtest.h>
#include <coreobjects/coreobjects.h>
#include <coretypes/multi_ptr.h>

using MultiPtrTest = testing::Test;

using namespace daq;

//using TestPtr = MultiPtr<SerializablePtr, PropertyPtr>;

// BEGIN_NAMESPACE_OPENDAQ
//
//DECLARE_OPENDAQ_INTERFACE(ITestObject, IBaseObject)
//{
//    virtual ErrCode INTERFACE_FUNC test() = 0;
//};
//
//END_NAMESPACE_OPENDAQ
//
//TEST_F(MultiPtrTest, AssignedFalse)
//{
//    TestPtr test;
//    ASSERT_FALSE(test.assigned());
//}
//
//TEST_F(MultiPtrTest, AssignedTrue)
//{
//    TestPtr ptr = IntProperty_Create("Test"_daq, Integer(0), Boolean(false));
//    ASSERT_TRUE(ptr.assigned());
//}
//
//TEST_F(MultiPtrTest, GetObject)
//{
//    TestPtr test;
//    ASSERT_EQ(test.getObject<IProperty>(), nullptr);
//}
//
//TEST_F(MultiPtrTest, Test)
//{
//    TestPtr ptr = IntProperty_Create("Test"_daq, Integer(0), Boolean(false));
//    BooleanPtr visibleTest = Boolean(false);
//    ASSERT_EQ(visibleTest, ptr.getVisible());
//}
//
//TEST_F(MultiPtrTest, Release)
//{
//    TestPtr ptr = IntProperty_Create("Test"_daq, Integer(0), Boolean(false));
//    ptr.release();
//}
//
//TEST_F(MultiPtrTest, Detach)
//{
//    TestPtr ptr = IntProperty_Create("Test"_daq, Integer(0), Boolean(false));
//    auto raw = ptr.detach<IProperty>();
//
//    ASSERT_EQ(raw->releaseRef(), 0);
//    ASSERT_FALSE(ptr.assigned());
//}
//
//TEST_F(MultiPtrTest, AddRefAndReturn)
//{
//    TestPtr ptr = IntProperty_Create("Test"_daq, Integer(0), Boolean(false));
//    IProperty* prop = ptr.addRefAndReturn<IProperty>();
//
//    ASSERT_EQ(prop->releaseRef(), 1);
//}
//
//TEST_F(MultiPtrTest, Attach)
//{
//    IProperty* prop = IntProperty_Create("Test"_daq, Integer(0), Boolean(false));
//    TestPtr ptr1(prop);
//    TestPtr ptr2(prop);
//
//    prop->releaseRef();
//}
//
//TEST_F(MultiPtrTest, CastToInterface)
//{
//    TestPtr obj = Property();
//
//    auto obj1 = obj.as<IBaseObject>();
//    obj1->releaseRef();
//
//    auto obj2 = INTF_CAST(obj, IBaseObject);
//    obj2->releaseRef();
//}
//
//TEST_F(MultiPtrTest, CastToPtr)
//{
//    TestPtr obj = Property();
//    auto obj1 = obj.asPtr<IBaseObject>();
//    auto obj2 = PTR_CAST(obj, IBaseObject);
//}
//
//
//TEST_F(MultiPtrTest, CreateFromObjectPtr)
//{
//    TestPtr prop = Property();
//    BaseObjectPtr obj1(prop);
//    BaseObjectPtr obj2 = prop;
//}
//
//TEST_F(MultiPtrTest, CreateObjectFromRValuePtr)
//{
//    TestPtr obj1(Property());
//}
//
//TEST_F(MultiPtrTest, CopyAndMoveFromOtherEmptyObjectPtr)
//{
//    ObjectPtr<ITestObject> objPtr1;
//
//    TestPtr objPtr2(objPtr1);
//    TestPtr objPtr3(std::move(objPtr1));
//}
