#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqContextTest = testing::Test;

TEST_F(COpendaqContextTest, Context)
{
    daqContext* ctx = nullptr;
    daqList* sinks = nullptr;
    daqList_createList(&sinks);

    daqLoggerSink* sink = nullptr;
    daqLoggerSink_createStdErrLoggerSink(&sink);
    daqList_pushBack(sinks, sink);

    daqLogger* logger = nullptr;
    daqLogger_createLogger(&logger, sinks, daqLogLevel::daqLogLevelDebug);

    daqTypeManager* typeManager = nullptr;
    daqTypeManager_createTypeManager(&typeManager);

    daqDict *options = nullptr, *discoveryServers = nullptr;
    daqDict_createDict(&options);
    daqDict_createDict(&discoveryServers);

    daqContext_createContext(&ctx, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    daqLogger* outLogger = nullptr;
    daqTypeManager* outTm = nullptr;
    daqDict *outOptions = nullptr, *outDiscoveryServers = nullptr;
    daqContext_getLogger(ctx, &outLogger);
    daqContext_getTypeManager(ctx, &outTm);
    daqContext_getOptions(ctx, &outOptions);
    daqContext_getDiscoveryServers(ctx, &outDiscoveryServers);

    ASSERT_NE(outLogger, nullptr);
    ASSERT_NE(outTm, nullptr);
    ASSERT_NE(outOptions, nullptr);
    ASSERT_NE(outDiscoveryServers, nullptr);

    daqBaseObject_releaseRef(outDiscoveryServers);
    daqBaseObject_releaseRef(outOptions);
    daqBaseObject_releaseRef(outTm);
    daqBaseObject_releaseRef(outLogger);

    daqBaseObject_releaseRef(discoveryServers);
    daqBaseObject_releaseRef(options);
    daqBaseObject_releaseRef(typeManager);
    daqBaseObject_releaseRef(logger);
    daqBaseObject_releaseRef(sink);
    daqBaseObject_releaseRef(sinks);
    daqBaseObject_releaseRef(ctx);
}