#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(ITestObject, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC test() = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(INTERNAL_FACTORY, TestObject)

class TestObjectImpl : public ImplementationOf<ITestObject>
{
public:
    ErrCode INTERFACE_FUNC test() override
    {
        return OPENDAQ_SUCCESS;
    }
};

OPENDAQ_DEFINE_CLASS_FACTORY(INTERNAL_FACTORY, TestObject)

class TestObjectPtr;

template <>
struct InterfaceToSmartPtr<ITestObject>
{
    typedef TestObjectPtr SmartPtr;
};

class TestObjectPtr : public ObjectPtr<ITestObject>
{
public:
    using ObjectPtr<ITestObject>::ObjectPtr;

    void test() const
    {
        if (!object)
            throw InvalidParameterException();

        const auto errCode = object->test();
        checkErrorInfo(errCode);
    }
};

static TestObjectPtr TestObject()
{
    TestObjectPtr obj(TestObject_Create());
    return obj;
}

END_NAMESPACE_OPENDAQ

using namespace daq;

using ObjectPtrTest = testing::Test;

TEST_F(ObjectPtrTest, EmptyConstructor)
{
    BaseObjectPtr ptr1;
    BaseObjectPtr ptr2(nullptr);
}

TEST_F(ObjectPtrTest, MovePtrConstructor)
{
    BaseObjectPtr obj1(BaseObject());
}

TEST_F(ObjectPtrTest, MoveIntfConstructor)
{
    BaseObjectPtr obj(BaseObject_Create());
}

TEST_F(ObjectPtrTest, CopyPtrConstructor)
{
    BaseObjectPtr baseObject = BaseObject();
    BaseObjectPtr obj(baseObject);
}

TEST_F(ObjectPtrTest, CopyIntfConstructor)
{
    IBaseObject* baseObject = BaseObject_Create();
    BaseObjectPtr obj(baseObject);
    baseObject->releaseRef();
}

TEST_F(ObjectPtrTest, MovePtrAssignment)
{
    BaseObjectPtr baseObject1(BaseObject());

    baseObject1 = BaseObject();
}

TEST_F(ObjectPtrTest, MoveIntfAssignment)
{
    BaseObjectPtr baseObject1(BaseObject());

    baseObject1 = BaseObject_Create();
}

TEST_F(ObjectPtrTest, CopyPtrAssignment)
{
    BaseObjectPtr baseObject1(BaseObject());
    BaseObjectPtr baseObject2(BaseObject());

    baseObject1 = baseObject2;
}

TEST_F(ObjectPtrTest, CopyIntfAssignment)
{
    BaseObjectPtr baseObject1(BaseObject());
    IBaseObject* baseObject2 = BaseObject_Create();
    baseObject1 = baseObject2;
    baseObject2->releaseRef();
}

TEST_F(ObjectPtrTest, FuncCallAndRefCount)
{
    BaseObjectPtr baseObject = BaseObject();
    baseObject->addRef();
    baseObject->releaseRef();
}

TEST_F(ObjectPtrTest, EmptyPtrCall)
{
    BaseObjectPtr ptr;
    ASSERT_ANY_THROW(ptr->releaseRef());
}

TEST_F(ObjectPtrTest, Equality)
{
    BaseObjectPtr ptr1 = BaseObject();
    BaseObjectPtr ptr2 = BaseObject();
    BaseObjectPtr ptr3;

    ASSERT_TRUE(ptr1 != ptr2);
    ASSERT_TRUE(ptr1 != nullptr);
    ASSERT_FALSE(ptr1 == nullptr);
    ASSERT_FALSE(ptr1 == ptr2);
    ASSERT_TRUE(ptr1 != ptr3);
    ASSERT_FALSE(ptr1 == ptr3);
    ASSERT_TRUE(ptr3 != ptr1);
    ASSERT_FALSE(ptr3 == ptr1);
}

TEST_F(ObjectPtrTest, EmptyEqual)
{
    BaseObjectPtr ptr;
    BaseObjectPtr ptr2;

    ASSERT_EQ(ptr, ptr2);
}

TEST_F(ObjectPtrTest, Attach)
{
    auto obj = BaseObject_Create();
    BaseObjectPtr ptr1(obj);
    BaseObjectPtr ptr2(obj);
    obj->releaseRef();
}

TEST_F(ObjectPtrTest, CastToInterface)
{
    auto obj = BaseObject();

    auto obj1 = obj.as<IBaseObject>();
    obj1->releaseRef();

    auto obj2 = INTF_CAST(obj, IBaseObject);
    obj2->releaseRef();
}

TEST_F(ObjectPtrTest, CastToPtr)
{
    auto obj = BaseObject();
    auto obj1 = obj.asPtr<IBaseObject>();
    auto obj2 = PTR_CAST(obj, IBaseObject);
}

TEST_F(ObjectPtrTest, CreateObjectFromLValueString)
{
    auto str = String("Test");
    BaseObjectPtr obj1(str);
    BaseObjectPtr obj2 = str;
}

TEST_F(ObjectPtrTest, CreateObjectFromRValueString)
{
    BaseObjectPtr obj1(String("Test"));
    BaseObjectPtr obj2 = String("Test");
}

TEST_F(ObjectPtrTest, CreateObjectFromCharPtr)
{
    BaseObjectPtr obj1("Test");
    BaseObjectPtr obj2 = "Test";
    ConstCharPtr str = "Test";
    BaseObjectPtr obj3 = str;
}

TEST_F(ObjectPtrTest, CreateObjectFromStdString)
{
    BaseObjectPtr obj1(std::string("Test"));
    BaseObjectPtr obj2 = std::string("Test");
    std::string str = std::string("Test");
    BaseObjectPtr obj3 = str;
}

TEST_F(ObjectPtrTest, CreateObjectFromLValueInt)
{
    auto objValue = (Int) 1;
    BaseObjectPtr obj1(objValue);
    BaseObjectPtr obj2 = objValue;
}

TEST_F(ObjectPtrTest, CreateObjectFromRValueInt)
{
    BaseObjectPtr obj1(Int(1));
    BaseObjectPtr obj2 = Int(1);
}

TEST_F(ObjectPtrTest, CreateObjectFromLValueFloat)
{
    auto objValue = 1.0f;
    BaseObjectPtr obj1(objValue);
    BaseObjectPtr obj2 = objValue;
}

TEST_F(ObjectPtrTest, CreateObjectFromRValueFloat)
{
    BaseObjectPtr obj1(1.0f);
    BaseObjectPtr obj2 = Int(1.0f);
}

TEST_F(ObjectPtrTest, CreateObjectFromLValueBool)
{
    auto objValue = True;
    BaseObjectPtr obj1(objValue);
    BaseObjectPtr obj2 = objValue;
}

TEST_F(ObjectPtrTest, CreateObjectFromRValueBool)
{
    BaseObjectPtr obj1(True);
    BaseObjectPtr obj2 = Bool(True);
}

TEST_F(ObjectPtrTest, AssignValueToExistingObject)
{
    BaseObjectPtr obj(1.0f);

    obj = True;
    obj = "Test";
    obj = 1;
    obj = 1.0f;
    obj = std::string("Test");
}

TEST_F(ObjectPtrTest, Cast)
{
    BaseObjectPtr obj("1");

    const Int valInt = obj;
    ASSERT_EQ(valInt, 1);

    const Float valFloat = obj;
    ASSERT_EQ(valFloat, 1.0);

    const Bool valBool = obj;
    ASSERT_EQ(valBool, True);

    std::string valString = obj;
    ASSERT_TRUE(valString == "1");
}

TEST_F(ObjectPtrTest, StaticCast)
{
    ObjectPtr<IBaseObject> bo = String("Test");
    auto str = static_cast<std::string>(bo);
    ASSERT_EQ(str, "Test");
}

TEST_F(ObjectPtrTest, CastFromEmpty)
{
    BaseObjectPtr obj;

    ASSERT_ANY_THROW([[maybe_unused]] const Int valInt = obj);

    ASSERT_ANY_THROW([[maybe_unused]] const Float valFloat = obj);
    ASSERT_ANY_THROW([[maybe_unused]] const Bool valBool = obj);
    ASSERT_ANY_THROW([[maybe_unused]] std::wstring valString = obj);

    ObjectPtr<ITestObject> objPtr1;
    ASSERT_ANY_THROW(auto objPtr2 = PTR_CAST(objPtr1, ITestObject));

    ASSERT_ANY_THROW(INTF_CAST(objPtr1, ITestObject));
}

TEST_F(ObjectPtrTest, Borrow)
{
    IBaseObject* obj = BaseObject_Create();
    BaseObjectPtr objPtr = BaseObjectPtr::Borrow(obj);
    ASSERT_EQ(obj->releaseRef(), 0);

    objPtr.detach();
}

TEST_F(ObjectPtrTest, BorrowFromAnotherInterface)
{
    TestObjectPtr testObject = TestObject();
    IBaseObject* baseObject = testObject.as<IBaseObject>();

    auto testObject2 = TestObjectPtr::Borrow(baseObject);
    testObject2.test();

    baseObject->releaseRef();
}

TEST_F(ObjectPtrTest, BorrowFromPtr)
{
    const auto obj = BaseObject();
    const auto borrowedObj = BaseObjectPtr::Borrow(obj);
    ASSERT_EQ(obj->addRef(), 2);
    obj->releaseRef();
}


TEST_F(ObjectPtrTest, Detach)
{
    auto obj = BaseObject();
    IBaseObject* intf = obj.detach();
    ASSERT_EQ(intf->releaseRef(), 0);
}

TEST_F(ObjectPtrTest, CopyAndMoveFromOtherEmptyObjectPtr)
{
    ObjectPtr<ITestObject> objPtr1;

    BaseObjectPtr objPtr2(objPtr1);
    BaseObjectPtr objPtr3(std::move(objPtr1));
}

TEST_F(ObjectPtrTest, MoveCreateFromOtherObjectPtr)
{
    TestObjectPtr testObject = TestObject();

    BaseObjectPtr objPtr(std::move(testObject));
}

TEST_F(ObjectPtrTest, MoveAssignFromOtherObjectPtr)
{
    TestObjectPtr testObject = TestObject();

    BaseObjectPtr objPtr{};
    objPtr = std::move(testObject);
}

TEST_F(ObjectPtrTest, MoveCreateFromOtherBorrowedType)
{
    auto intf = TestObject_Create();
    auto obj = TestObjectPtr::Borrow(intf);
    BaseObjectPtr obj1(std::move(obj));
    intf->releaseRef();
}

TEST_F(ObjectPtrTest, MoveAssignFromOtherBorrowedType)
{
    auto intf = TestObject_Create();
    auto obj = TestObjectPtr::Borrow(intf);
    BaseObjectPtr obj1;
    obj1 = std::move(obj);
    intf->releaseRef();
}

TEST_F(ObjectPtrTest, Getters)
{
    auto integer = Integer(1);
    Int i = integer;
    ASSERT_EQ(i, 1);

    auto floating = Floating(1.0);
    Float f = floating;
    ASSERT_EQ(f, 1.0);

    auto boolean = Boolean(true);
    Bool b = boolean;
    ASSERT_TRUE(b);

    auto stringObject = String("Test");
    std::string s = stringObject;
    ASSERT_EQ(s, "Test");
}

TEST_F(ObjectPtrTest, Comparison)
{
    auto obj1 = Integer(1);
    auto obj2 = Integer(1);
    auto obj3 = Integer(3);

    ASSERT_EQ(obj1, obj2);
    ASSERT_NE(obj1, obj3);
}

TEST_F(ObjectPtrTest, OrdinalComparison)
{
    auto obj1 = Integer(1);

    ASSERT_EQ(obj1, 1LL);
    ASSERT_EQ(obj1, 1L);
    ASSERT_EQ(obj1, 1);

    auto obj2 = String("test");
    ASSERT_EQ(obj2, std::string("test"));
}

TEST_F(ObjectPtrTest, CoreType)
{
    ASSERT_EQ(BaseObject().getCoreType(), ctObject);
}

TEST_F(ObjectPtrTest, NotIFreezable)
{
    auto intObj = Integer(1);

    ASSERT_ANY_THROW(intObj.freeze());
}

TEST_F(ObjectPtrTest, IsFrozenNotIFreezable)
{
    auto intObj = Integer(1);

    ASSERT_ANY_THROW(intObj.isFrozen());
}

TEST_F(ObjectPtrTest, ConvertToCoreType)
{
    auto intObj = Integer(3);

    auto floatObj = intObj.convertTo(ctFloat);
    ASSERT_EQ(floatObj.getCoreType(), ctFloat);
    ASSERT_EQ(floatObj, 3.0);

    auto boolObj = intObj.convertTo(ctBool);
    ASSERT_EQ(boolObj.getCoreType(), ctBool);
    ASSERT_TRUE(IsTrue(boolObj));

    auto stringObj = intObj.convertTo(ctString);
    ASSERT_EQ(stringObj.getCoreType(), ctString);
    ASSERT_EQ(stringObj, "3");

    auto newIntObj = intObj.convertTo(ctInt);
    ASSERT_EQ(newIntObj.getCoreType(), ctInt);
    ASSERT_EQ(newIntObj, 3);
}

TEST_F(ObjectPtrTest, CheckInvalidObjByNull)
{
    BaseObjectPtr ptr = BaseObject();
    ptr = nullptr;

    ASSERT_THROW(ptr.getCoreType(), InvalidParameterException);
}

TEST_F(ObjectPtrTest, CheckInvalidObjByRelease)
{
    auto obj = BaseObject();
    obj.release();

    ASSERT_THROW(obj.getCoreType(), InvalidParameterException);
}

void testSmartPtrByRef(IBaseObject** obj)
{
    *obj = BaseObject().detach();
}

TEST_F(ObjectPtrTest, SmartPtrByRef)
{
    BaseObjectPtr obj1;
    testSmartPtrByRef(&obj1);
    ASSERT_EQ(obj1->addRef(), 2);
    ASSERT_EQ(obj1->releaseRef(), 1);

    BaseObjectPtr obj2;
    testSmartPtrByRef(obj2.addressOf());
    ASSERT_EQ(obj2->addRef(), 2);
    ASSERT_EQ(obj2->releaseRef(), 1);
}

TEST_F(ObjectPtrTest, CreateWithImplementation)
{
    TestObjectPtr testObject = createWithImplementation<ITestObject, TestObjectImpl>();
}

TEST_F(ObjectPtrTest, SupportsInterface)
{
    auto baseObject = BaseObject();
    ASSERT_TRUE(baseObject.supportsInterface<IBaseObject>());
    ASSERT_TRUE(baseObject.supportsInterface(IBaseObject::Id));
}
