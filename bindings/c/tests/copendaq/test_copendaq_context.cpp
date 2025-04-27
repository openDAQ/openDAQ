#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqContextTest = testing::Test;

TEST_F(COpendaqContextTest, Context)
{
    Context* ctx = nullptr;
    List* sinks = nullptr;
    List_createList(&sinks);

    LoggerSink* sink = nullptr;
    LoggerSink_createStdErrLoggerSink(&sink);
    List_pushBack(sinks, sink);

    Logger* logger = nullptr;
    Logger_createLogger(&logger, sinks, LogLevel::LogLevelDebug);

    TypeManager* typeManager = nullptr;
    TypeManager_createTypeManager(&typeManager);

    Dict *options = nullptr, *discoveryServers = nullptr;
    Dict_createDict(&options);
    Dict_createDict(&discoveryServers);

    Context_createContext(&ctx, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    Logger* outLogger = nullptr;
    TypeManager* outTm = nullptr;
    Dict *outOptions = nullptr, *outDiscoveryServers = nullptr;
    Context_getLogger(ctx, &outLogger);
    Context_getTypeManager(ctx, &outTm);
    Context_getOptions(ctx, &outOptions);
    Context_getDiscoveryServers(ctx, &outDiscoveryServers);

    ASSERT_NE(outLogger, nullptr);
    ASSERT_NE(outTm, nullptr);
    ASSERT_NE(outOptions, nullptr);
    ASSERT_NE(outDiscoveryServers, nullptr);

    BaseObject_releaseRef(outDiscoveryServers);
    BaseObject_releaseRef(outOptions);
    BaseObject_releaseRef(outTm);
    BaseObject_releaseRef(outLogger);

    BaseObject_releaseRef(discoveryServers);
    BaseObject_releaseRef(options);
    BaseObject_releaseRef(typeManager);
    BaseObject_releaseRef(logger);
    BaseObject_releaseRef(sink);
    BaseObject_releaseRef(sinks);
    BaseObject_releaseRef(ctx);
}