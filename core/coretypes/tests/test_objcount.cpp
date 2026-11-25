#include <gtest/gtest.h>
#include <coretypes/coretypes.h>
#include <vector>

using namespace daq;

namespace test_objcount
{


using ObjCountTest = testing::Test;

#ifndef NDEBUG

class CreatesObjectInToStringImpl : public ImplementationOf<>
{
public:
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override
    {
        const auto temporary = BaseObject();
        return daqDuplicateCharPtr("CreatesObjectInToString", str);
    }
};

class DestroysObjectInToStringImpl : public ImplementationOf<>
{
public:
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override
    {
        held.release();
        return daqDuplicateCharPtr("DestroysObjectInToString", str);
    }

private:
    BaseObjectPtr held = BaseObject();
};

TEST_F(ObjCountTest, PrintTrackedObjectWhoseToStringCreatesAnObject)
{
    const auto obj = createWithImplementation<IBaseObject, CreatesObjectInToStringImpl>();
    const size_t count = daqGetTrackedObjectCount();

    daqPrintTrackedObjects();

    ASSERT_EQ(daqGetTrackedObjectCount(), count);
    const auto afterPrinting = BaseObject();
    ASSERT_EQ(daqGetTrackedObjectCount(), count + 1);
}

TEST_F(ObjCountTest, PrintTrackedObjectWhoseToStringDestroysAnObject)
{
    const auto obj = createWithImplementation<IBaseObject, DestroysObjectInToStringImpl>();
    const size_t count = daqGetTrackedObjectCount();

    daqPrintTrackedObjects();

    ASSERT_EQ(daqGetTrackedObjectCount(), count - 1);
}

TEST_F(ObjCountTest, CountObjects)
{
    size_t val1 = daqGetTrackedObjectCount();
    size_t val2 = daqGetTrackedObjectCount();
    ASSERT_EQ(val1, val2);

    auto obj = BaseObject();
    size_t val3 = daqGetTrackedObjectCount();
    ASSERT_EQ(val1 + 1, val3);
}

TEST_F(ObjCountTest, PooledObjectsTrackedOnlyWhileAlive)
{
    // Above the small-integer cache so the pool is used, and within its pre-allocated objects.
    constexpr size_t count = 50;
    const size_t baseline = daqGetTrackedObjectCount();

    {
        std::vector<ObjectPtr<IInteger>> integers;
        integers.reserve(count);
        for (size_t i = 0; i < count; ++i)
            integers.push_back(IntegerFromPool(static_cast<Int>(1000 + i)));

        // Handed out to a caller, so alive and tracked: leaking one has to remain detectable.
        ASSERT_EQ(daqGetTrackedObjectCount(), baseline + count);
    }

    // Back on the free list. A pooled object is not destroyed, but it is no longer alive either, so
    // it must not stay tracked - otherwise the next leak check reports every object the pool had to
    // construct as leaked, and blames whichever test happened to raise the pool's high-water mark.
    ASSERT_EQ(daqGetTrackedObjectCount(), baseline);
}

#endif
}
// namespace test_objcount
