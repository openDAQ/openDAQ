#include "tms_object_integration_test.h"
#include <opendaq/context_factory.h>

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;

static LoggerPtr CreateLoggerWithDebugSink(const LoggerSinkPtr& sink)
{
    sink.setLevel(LogLevel::Warn);
    auto sinks = DefaultSinks(nullptr);
    sinks.pushBack(sink);
    return LoggerWithSinks(sinks);
}

void TmsObjectIntegrationTest::SetUp()
{
    TmsObjectTest::SetUp();
    debugSink = LastMessageLoggerSink();
    logger = CreateLoggerWithDebugSink(debugSink);

    ctx = daq::NullContext(logger);
    ctxClient = daq::NullContext(logger);
    clientContext = std::make_shared<TmsClientContext>(client, ctxClient);
    serverContext = std::make_shared<TmsServerContext>(ctx, nullptr);

    clientContext->addEnumerationTypesToTypeManager();
}

LastMessageLoggerSinkPrivatePtr TmsObjectIntegrationTest::getPrivateSink()
{
    if(!debugSink.assigned())
        throw ArgumentNullException("Sink must not be null");
    auto sinkPtr = debugSink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();
    if (sinkPtr == nullptr)
        throw InvalidTypeException("Wrong sink. GetLastMessage supports only by LastMessageLoggerSink");
    return sinkPtr;
}

void TmsObjectIntegrationTest::TearDown()
{
    clientContext.reset();
    serverContext = nullptr;
    ctx = nullptr;
    ctxClient = nullptr;

    TmsObjectTest::TearDown();
}
