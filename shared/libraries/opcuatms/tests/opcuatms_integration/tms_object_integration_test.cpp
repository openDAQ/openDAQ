#include "tms_object_integration_test.h"
#include <opendaq/context_factory.h>

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;

void TmsObjectIntegrationTest::SetUp()
{
    TmsObjectTest::SetUp();

    ctx = daq::NullContext();
    clientContext = std::make_shared<TmsClientContext>(client, ctx);
    serverContext = std::make_shared<TmsServerContext>(ctx);
}

void TmsObjectIntegrationTest::TearDown()
{
    clientContext.reset();
    serverContext = nullptr;
    ctx = nullptr;

    TmsObjectTest::TearDown();
}
