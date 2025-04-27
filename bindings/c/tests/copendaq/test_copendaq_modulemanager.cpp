#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqModuleManagerTest : public testing::Test
{
    void SetUp() override
    {
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

        BaseObject_releaseRef(discoveryServers);
        BaseObject_releaseRef(options);
        BaseObject_releaseRef(typeManager);
        BaseObject_releaseRef(logger);
        BaseObject_releaseRef(sink);
        BaseObject_releaseRef(sinks);
    }

    void TearDown() override
    {
        BaseObject_releaseRef(ctx);
    }

protected:
    Context* ctx = nullptr;
};

TEST_F(COpendaqModuleManagerTest, ModuleManager)
{
    ModuleManager* moduleManager = nullptr;

    String* path = nullptr;
    String_createString(&path, ".");
    ModuleManager_createModuleManager(&moduleManager, path);
    ASSERT_NE(moduleManager, nullptr);

    ModuleManager_loadModules(moduleManager, ctx);

    List* modules = nullptr;
    ModuleManager_getModules(moduleManager, &modules);

    ASSERT_NE(modules, nullptr);
    SizeT size = 0;
    List_getCount(modules, &size);
    ASSERT_GT(size, 0);

    Module* module = nullptr;
    List_getItemAt(modules, 0, reinterpret_cast<BaseObject**>(&module));
    ASSERT_NE(module, nullptr);

    ModuleInfo* info = nullptr;
    Module_getModuleInfo(module, &info);
    ASSERT_NE(info, nullptr);

    String* name = nullptr;
    ModuleInfo_getId(info, &name);
    ASSERT_NE(name, nullptr);

    BaseObject_releaseRef(name);
    BaseObject_releaseRef(info);
    BaseObject_releaseRef(module);
    BaseObject_releaseRef(modules);
    BaseObject_releaseRef(moduleManager);
    BaseObject_releaseRef(path);
}
