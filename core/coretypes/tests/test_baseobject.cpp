#include <testutils/bb_memcheck_listener.h>
#include <coretypes/baseobject.h>
#include <coretypes/mem.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/inspectable_ptr.h>
#include <coretypes/string_ptr.h>

using namespace daq;

using BaseObjectTest = testing::Test;

TEST_F(BaseObjectTest, Create)
{
    IBaseObject* baseObject;
    ErrCode res = createBaseObject(&baseObject);
    ASSERT_EQ(res, OPENDAQ_SUCCESS);
    ASSERT_TRUE(baseObject);
    ASSERT_EQ(baseObject->releaseRef(), 0);
}

TEST_F(BaseObjectTest, CreatePtr)
{
    auto baseObject = BaseObject();

    ASSERT_TRUE(baseObject.assigned());
    ASSERT_EQ(baseObject->releaseRef(), 0);

    baseObject.detach();
}

TEST_F(BaseObjectTest, CreateNullParam)
{
    ErrCode res = createBaseObject(nullptr);
    ASSERT_EQ(res, OPENDAQ_ERR_ARGUMENT_NULL);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IBaseObject", "daq");

TEST_F(BaseObjectTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IBaseObject::Id);
}

TEST_F(BaseObjectTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IBaseObject>(), "{9C911F6D-1664-5AA2-97BD-90FE3143E881}");
}

TEST_F(BaseObjectTest, AddReleaseRef)
{
    IBaseObject* baseObject;
    ErrCode res = createBaseObject(&baseObject);
    ASSERT_EQ(res, OPENDAQ_SUCCESS);

    baseObject->addRef();
    baseObject->addRef();
    baseObject->addRef();
    baseObject->releaseRef();
    baseObject->releaseRef();
    baseObject->addRef();
    baseObject->releaseRef();
    baseObject->releaseRef();
    baseObject->releaseRef();
}

TEST_F(BaseObjectTest, MemoryLeak)
{
    MemCheckListener::expectMemoryLeak = true;
    IBaseObject* baseObject;
    createBaseObject(&baseObject);
}

TEST_F(BaseObjectTest, ManualDispose)
{
    IBaseObject* baseObject;
    createBaseObject(&baseObject);
    baseObject->dispose();
    ASSERT_EQ(baseObject->releaseRef(), 0);
}

TEST_F(BaseObjectTest, Hashing)
{
    IBaseObject* baseObject1;
    IBaseObject* baseObject2;

    createBaseObject(&baseObject1);
    createBaseObject(&baseObject2);

    SizeT hashCode1;
    baseObject1->getHashCode(&hashCode1);
    SizeT hashCode2;
    baseObject2->getHashCode(&hashCode2);

    ASSERT_TRUE(hashCode1 != 0 && hashCode2 != 0 && hashCode1 != hashCode2)
                << "Hashing failed: hashCode1 = " << hashCode1 << "   hashCode2 = " << hashCode2;

    ASSERT_EQ(baseObject1->releaseRef(), 0);
    ASSERT_EQ(baseObject2->releaseRef(), 0);
}

TEST_F(BaseObjectTest, Equality)
{
    IBaseObject* baseObject1;
    IBaseObject* baseObject2;

    createBaseObject(&baseObject1);
    createBaseObject(&baseObject2);

    Bool eq{false};
    baseObject1->equals(baseObject1, &eq);
    ASSERT_TRUE(eq);

    baseObject1->equals(nullptr, &eq);
    ASSERT_FALSE(eq);

    baseObject1->equals(baseObject2, &eq);
    ASSERT_FALSE(eq);

    baseObject2->equals(baseObject1, &eq);
    ASSERT_FALSE(eq);

    baseObject2->equals(baseObject2, &eq);
    ASSERT_TRUE(eq);

    ASSERT_EQ(baseObject1->releaseRef(), 0);
    ASSERT_EQ(baseObject2->releaseRef(), 0);
}

TEST_F(BaseObjectTest, ToString)
{
    IBaseObject* baseObject;
    createBaseObject(&baseObject);

    CharPtr str;
    ASSERT_EQ(baseObject->toString(&str), OPENDAQ_SUCCESS);

    ASSERT_STREQ(str, "daq::IInspectable");
    daqFreeMemory(str);

    ASSERT_EQ(baseObject->toString(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(baseObject->releaseRef(), 0);
}

TEST_F(BaseObjectTest, QueryInterfaceIUnknown)
{
    IBaseObject* baseObject1;
    createBaseObject(&baseObject1);
    IUnknown* unk;
    ASSERT_EQ(baseObject1->queryInterface(IUnknown::Id, reinterpret_cast<void**>(&unk)), OPENDAQ_SUCCESS);
    ASSERT_EQ(baseObject1->releaseRef(), 1);

    ASSERT_EQ(unk->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject1)), OPENDAQ_SUCCESS);
    ASSERT_EQ(unk->releaseRef(), 1);
    ASSERT_EQ(baseObject1->releaseRef(), 0);
}

TEST_F(BaseObjectTest, QueryInterface)
{
    IBaseObject* baseObject1;
    createBaseObject(&baseObject1);

    ASSERT_EQ(baseObject1->queryInterface(IBaseObject::Id, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);

    IBaseObject* baseObject2;
    ASSERT_EQ(baseObject1->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject2)), OPENDAQ_SUCCESS);

    Bool eq{false};
    baseObject1->equals(baseObject2, &eq);
    ASSERT_TRUE(eq);

    static const IntfID TestGuid = { 0x11111111, 0x1111, 0x1111, { { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 } } };

    IBaseObject* baseObject3;
    ASSERT_EQ(baseObject1->queryInterface(TestGuid, reinterpret_cast<void**>(&baseObject3)), OPENDAQ_ERR_NOINTERFACE);

    ASSERT_EQ(baseObject1->releaseRef(), 1);
    ASSERT_EQ(baseObject2->releaseRef(), 0);
}

TEST_F(BaseObjectTest, BorrowInterface)
{
    IBaseObject* baseObject1;
    createBaseObject(&baseObject1);

    ASSERT_EQ(baseObject1->borrowInterface(IBaseObject::Id, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);

    IBaseObject* baseObject2;
    ASSERT_EQ(baseObject1->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObject2)), OPENDAQ_SUCCESS);

    Bool eq{false};
    baseObject1->equals(baseObject2, &eq);
    ASSERT_TRUE(eq);

    static const IntfID TestGuid = { 0x11111111, 0x1111, 0x1111, { { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 } } };

    IBaseObject* baseObject3;
    ASSERT_EQ(baseObject1->borrowInterface(TestGuid, reinterpret_cast<void**>(&baseObject3)), OPENDAQ_ERR_NOINTERFACE);

    ASSERT_EQ(baseObject1->releaseRef(), 0);
}

TEST_F(BaseObjectTest, Inspectable)
{
    BaseObjectPtr obj;
    createBaseObject(&obj);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IInspectable::Id);
}

TEST_F(BaseObjectTest, ImplementationName)
{
    BaseObjectPtr obj;
    createBaseObject(&obj);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::BaseObjectImpl");
}
