#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqSynchronizationTest = testing::Test;

TEST_F(COpendaqSynchronizationTest, SyncComponent)
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

    Context* context = nullptr;
    Context_createContext(&context, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    SyncComponent* syncComponent = nullptr;
    String* localId = nullptr;
    String_createString(&localId, "localId");

    SyncComponent_createSyncComponent(&syncComponent, context, nullptr, localId);

    SyncComponentPrivate* syncComponentPrivate = nullptr;
    BaseObject_borrowInterface(syncComponent, SYNC_COMPONENT_PRIVATE_INTF_ID, reinterpret_cast<void**>(&syncComponentPrivate));

    ASSERT_NE(syncComponentPrivate, nullptr);

    PropertyObject* interface = nullptr;
    String* className = nullptr;
    String_createString(&className, "InterfaceClockSync");
    PropertyObject_createPropertyObjectWithClassAndManager(&interface, typeManager, className);

    ErrCode err = SyncComponentPrivate_addInterface(syncComponentPrivate, interface);
    ASSERT_EQ(err, 0);

    Dict* interfaces = nullptr;
    SyncComponent_getInterfaces(syncComponent, &interfaces);
    ASSERT_NE(interfaces, nullptr);
    SizeT size = 0;
    Dict_getCount(interfaces, &size);
    ASSERT_EQ(size, 1);

    BaseObject_releaseRef(interfaces);
    BaseObject_releaseRef(interface);
    BaseObject_releaseRef(className);
    BaseObject_releaseRef(syncComponent);
    BaseObject_releaseRef(localId);
    BaseObject_releaseRef(context);
    BaseObject_releaseRef(discoveryServers);
    BaseObject_releaseRef(options);
    BaseObject_releaseRef(typeManager);
    BaseObject_releaseRef(logger);
    BaseObject_releaseRef(sink);
    BaseObject_releaseRef(sinks);
}
