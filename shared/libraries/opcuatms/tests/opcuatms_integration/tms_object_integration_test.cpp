#include "tms_object_integration_test.h"

using namespace daq::opcua;
using namespace daq::opcua::tms;

TmsObjectIntegrationTest::TmsObjectIntegrationTest()
    : TmsObjectTest()
{
}

void TmsObjectIntegrationTest::SetUp()
{
    TmsObjectTest::SetUp();

    clientContext = std::make_shared<TmsClientContext>(client);
}

void TmsObjectIntegrationTest::TearDown()
{
    clientContext.reset();

    TmsObjectTest::TearDown();
}
