#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(ITestObject1, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC setParent(ITestObject1* parent) = 0;
    virtual ErrCode INTERFACE_FUNC setChild(ITestObject1* child) = 0;
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;
    virtual ErrCode INTERFACE_FUNC toString2(IString** str) = 0;
    virtual ErrCode INTERFACE_FUNC getInternalWeakRef(IWeakRef** weakRef) = 0;
};

END_NAMESPACE_OPENDAQ

class TestObject1Impl : public IntfObjectWithWeakRefImpl<ITestObject1, IInspectable>
{
public:
    TestObject1Impl()
        : child(nullptr), parent(nullptr)
    {
    }

    ErrCode INTERFACE_FUNC setParent(ITestObject1* parent) override
    {
        this->parent = parent;

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setChild(ITestObject1* child) override
    {
        this->child = child;

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setName(IString* name) override
    {
        this->name = name;

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getName(IString** name) override
    {
        *name = this->name.addRefAndReturn();

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC toString2(IString** str) override
    {
        std::ostringstream s;
        s << name;
        if (child.assigned())
        {
            StringPtr childName;
            child->getName(childName.addressOf());
            s << "; child = " << childName;
        }
        if (parent.assigned())
        {
            auto parentRef = parent.getRef();
            StringPtr parentName;
            parentRef->getName(parentName.addressOf());
            s << "; parent = " << parentName;
        }

        StringPtr r = s.str();
        *str = r.detach();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getInternalWeakRef(IWeakRef** weakRef) override
    {
        auto wr = getWeakRefInternal<ITestObject1>();
        *weakRef = wr.detach();
        return OPENDAQ_SUCCESS;
    }

protected:
    void internalDispose(bool disposing) override
    {
        if (disposing)
        {
            child.release();
            parent.release();
            name.release();
        }
    }

private:
    StringPtr name;
    ObjectPtr<ITestObject1> child;
    WeakRefPtr<ITestObject1> parent;
};

ITestObject1* CreateTestObject()
{
    ITestObject1* testObject = new TestObject1Impl();
    testObject->addRef();
    return testObject;
}

using WeakRefTest = testing::Test;

TEST_F(WeakRefTest, CheckObject)
{
    auto obj1 = CreateTestObject();
    obj1->setName(StringPtr("Object"));
    StringPtr str;
    obj1->toString2(str.addressOf());
    obj1->releaseRef();

    ASSERT_EQ(str, "Object");
}

TEST_F(WeakRefTest, ReleasedObject)
{
    ObjectPtr<ITestObject1> obj(CreateTestObject());

    WeakRefPtr<ITestObject1> weakRef = obj;

    obj.release();

    ASSERT_EQ(weakRef.getRef(), nullptr);
}

TEST_F(WeakRefTest, NullObject)
{
    ObjectPtr<ITestObject1> obj;
    WeakRefPtr<ITestObject1> weakRef = obj;

    ASSERT_FALSE(weakRef.assigned());
}

TEST_F(WeakRefTest, NoMemLeakParentChildFirst)
{
    auto child = CreateTestObject();
    child->setName(StringPtr("Object1"));

    auto parent = CreateTestObject();
    parent->setName(StringPtr("Object2"));

    child->setParent(parent);
    parent->setChild(child);

    StringPtr s1;
    child->toString2(&s1);
    ASSERT_EQ(s1, "Object1; parent = Object2");

    StringPtr s2;
    parent->toString2(&s2);
    ASSERT_EQ(s2, "Object2; child = Object1");

    child->releaseRef();
    parent->releaseRef();
}

TEST_F(WeakRefTest, NoMemLeakParentParentFirst)
{
    auto child = CreateTestObject();
    child->setName(StringPtr("Object1"));

    auto parent = CreateTestObject();
    parent->setName(StringPtr("Object2"));

    child->setParent(parent);
    parent->setChild(child);

    StringPtr s1;
    child->toString2(&s1);
    ASSERT_EQ(s1, "Object1; parent = Object2");

    StringPtr s2;
    parent->toString2(&s2);
    ASSERT_EQ(s2, "Object2; child = Object1");

    parent->releaseRef();
    child->releaseRef();
}

TEST_F(WeakRefTest, InternalWeakRef)
{
    ObjectPtr<ITestObject1> test(CreateTestObject());
    IWeakRef* wr;
    checkErrorInfo(test->getInternalWeakRef(&wr));

    auto wrPtr = WeakRefPtr<ITestObject1>(wr);
    wr->releaseRef();
    auto obj = wrPtr.getRef();
    obj->setName(String("test"));
}

TEST_F(WeakRefTest, Inspectable)
{
    ObjectPtr<ITestObject1> test(CreateTestObject());
    WeakRefPtr<ITestObject1> obj = test;

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IWeakRef::Id);
}

TEST_F(WeakRefTest, ImplementationName)
{
    ObjectPtr<ITestObject1> test(CreateTestObject());
    WeakRefPtr<ITestObject1> obj = test;

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::WeakRefImpl");
}
