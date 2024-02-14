#include "tms_server_test.h"

void TmsServerObjectTest::SetUp()
{
    TmsObjectTest::SetUp();

    ctx = daq::NullContext();
    tmsCtx = std::make_shared<daq::opcua::tms::TmsServerContext>(ctx, nullptr);
}

void TmsServerObjectTest::TearDown()
{
    ctx = nullptr;
    tmsCtx = nullptr;

    TmsObjectTest::TearDown();
}
