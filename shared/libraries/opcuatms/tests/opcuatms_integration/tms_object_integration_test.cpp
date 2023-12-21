#include "tms_object_integration_test.h"
#include <opendaq/context_factory.h>

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;

TmsObjectIntegrationTest::TmsObjectIntegrationTest()
    : TmsObjectTest()
{
}

void TmsObjectIntegrationTest::SetUp()
{
    TmsObjectTest::SetUp();

    context = NullContext();
    clientContext = std::make_shared<TmsClientContext>(client, context);
}

void TmsObjectIntegrationTest::TearDown()
{
    clientContext.reset();

    TmsObjectTest::TearDown();
}
