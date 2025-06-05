#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqModuleManagerTest : public testing::Test
{
    void SetUp() override
    {
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

        daqBaseObject_releaseRef(discoveryServers);
        daqBaseObject_releaseRef(options);
        daqBaseObject_releaseRef(typeManager);
        daqBaseObject_releaseRef(logger);
        daqBaseObject_releaseRef(sink);
        daqBaseObject_releaseRef(sinks);
    }

    void TearDown() override
    {
        daqBaseObject_releaseRef(ctx);
    }

protected:
    daqContext* ctx = nullptr;
};

// Even just constructing and destroying the ModuleManager produces leaks on some platforms
TEST_F(COpendaqModuleManagerTest, DISABLED_ModuleManager)
{
    daqModuleManager* moduleManager = nullptr;

    daqString* path = nullptr;
    daqString_createString(&path, ".");
    daqModuleManager_createModuleManager(&moduleManager, path);
    ASSERT_NE(moduleManager, nullptr);

    daqModuleManager_loadModules(moduleManager, ctx);

    daqList* modules = nullptr;
    daqModuleManager_getModules(moduleManager, &modules);

    ASSERT_NE(modules, nullptr);
    daqSizeT size = 0;
    daqList_getCount(modules, &size);
    ASSERT_GT(size, 0);

    daqModule* module = nullptr;
    daqList_getItemAt(modules, 0, (daqBaseObject**) &module);
    ASSERT_NE(module, nullptr);

    daqModuleInfo* info = nullptr;
    daqModule_getModuleInfo(module, &info);
    ASSERT_NE(info, nullptr);

    daqString* name = nullptr;
    daqModuleInfo_getId(info, &name);
    ASSERT_NE(name, nullptr);

    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(info);
    daqBaseObject_releaseRef(module);
    daqBaseObject_releaseRef(modules);
    daqBaseObject_releaseRef(moduleManager);
    daqBaseObject_releaseRef(path);
}
