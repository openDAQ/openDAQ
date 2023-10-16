#include <gtest/gtest.h>
#include <coretypes/cycle_detector.h>

using namespace daq;

using CycleDetectorTest = testing::Test;

TEST_F(CycleDetectorTest, EnterLeave)
{
    IBaseObject* baseObject;
    ErrCode res = createBaseObject(&baseObject);
    ASSERT_EQ(res, OPENDAQ_SUCCESS);

    ASSERT_EQ(daqCycleDetectEnter(baseObject), 1);
    daqCycleDetectLeave(baseObject);

    baseObject->releaseRef();
}

TEST_F(CycleDetectorTest, DetectCycle)
{
    IBaseObject* baseObject;
    ErrCode res = createBaseObject(&baseObject);
    ASSERT_EQ(res, OPENDAQ_SUCCESS);

    ASSERT_EQ(daqCycleDetectEnter(baseObject), 1);
    ASSERT_EQ(daqCycleDetectEnter(baseObject), 0);
    daqCycleDetectLeave(baseObject);

    baseObject->releaseRef();
}

TEST_F(CycleDetectorTest, TwoObjects)
{
    IBaseObject* baseObject1;
    ErrCode res = createBaseObject(&baseObject1);
    ASSERT_EQ(res, OPENDAQ_SUCCESS);

    IBaseObject* baseObject2;
    res = createBaseObject(&baseObject2);
    ASSERT_EQ(res, OPENDAQ_SUCCESS);

    ASSERT_EQ(daqCycleDetectEnter(baseObject1), 1);

    ASSERT_EQ(daqCycleDetectEnter(baseObject2), 1);
    daqCycleDetectLeave(baseObject2);

    daqCycleDetectLeave(baseObject1);

    baseObject1->releaseRef();
    baseObject2->releaseRef();
}

