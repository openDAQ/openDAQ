#include <copendaq.h>
#include <coretypes/errorinfo.h>

#include <gtest/gtest.h>

using COpendaqSynchronizationTest = testing::Test;

TEST_F(COpendaqSynchronizationTest, SyncComponent)
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

    daqContext* context = nullptr;
    daqContext_createContext(&context, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    daqSyncComponent* syncComponent = nullptr;
    daqString* localId = nullptr;
    daqString_createString(&localId, "localId");

    daqSyncComponent_createSyncComponent(&syncComponent, context, nullptr, localId);

    daqSyncComponentPrivate* syncComponentPrivate = nullptr;
    daqBaseObject_borrowInterface(syncComponent, DAQ_SYNC_COMPONENT_PRIVATE_INTF_ID, (void**) &syncComponentPrivate);

    ASSERT_NE(syncComponentPrivate, nullptr);

    daqPropertyObject* interface = nullptr;
    daqString* className = nullptr;
    daqString_createString(&className, "InterfaceClockSync");
    daqPropertyObject_createPropertyObjectWithClassAndManager(&interface, typeManager, className);

    daqErrCode err = daqSyncComponentPrivate_addInterface(syncComponentPrivate, interface);
    ASSERT_EQ(err, 0u);

    daqDict* interfaces = nullptr;
    daqSyncComponent_getInterfaces(syncComponent, &interfaces);
    ASSERT_NE(interfaces, nullptr);
    daqSizeT size = 0u;
    daqDict_getCount(interfaces, &size);
    ASSERT_EQ(size, 1u);

    daqBaseObject_releaseRef(interfaces);
    daqBaseObject_releaseRef(interface);
    daqBaseObject_releaseRef(className);
    daqBaseObject_releaseRef(syncComponent);
    daqBaseObject_releaseRef(localId);
    daqBaseObject_releaseRef(context);
    daqBaseObject_releaseRef(discoveryServers);
    daqBaseObject_releaseRef(options);
    daqBaseObject_releaseRef(typeManager);
    daqBaseObject_releaseRef(logger);
    daqBaseObject_releaseRef(sink);
    daqBaseObject_releaseRef(sinks);
}

TEST_F(COpendaqSynchronizationTest, Synchronization)
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

    // The context registers the synchronization status types in the type manager
    daqContext* context = nullptr;
    daqContext_createContext(&context, nullptr, logger, typeManager, nullptr, nullptr, options, discoveryServers);

    daqString* deviceId = nullptr;
    daqString_createString(&deviceId, "testDevice");

    daqSynchronization* synchronization = nullptr;
    ASSERT_EQ(daqSynchronization_createSynchronization(&synchronization, typeManager, deviceId), 0u);
    ASSERT_NE(synchronization, nullptr);

    daqSizeT count = 0u;

    daqDict* interfaces = nullptr;
    daqSynchronization_getInterfaces(synchronization, &interfaces);
    daqDict_getCount(interfaces, &count);
    ASSERT_EQ(count, 1u);

    daqDict* sources = nullptr;
    daqSynchronization_getAvailableSources(synchronization, &sources);
    daqDict_getCount(sources, &count);
    ASSERT_EQ(count, 1u);

    daqList* referenceDomainIds = nullptr;
    daqSynchronization_getReferenceDomainIds(synchronization, &referenceDomainIds);
    daqList_getCount(referenceDomainIds, &count);
    ASSERT_EQ(count, 1u);

    daqSyncInterface* source = nullptr;
    daqSynchronization_getSource(synchronization, &source);
    ASSERT_NE(source, nullptr);

    daqString* id = nullptr;
    daqSyncInterface_getId(source, &id);
    daqString* syncType = nullptr;
    daqSyncInterface_getSyncType(source, &syncType);
    daqString* referenceDomainId = nullptr;
    daqSyncInterface_getReferenceDomainId(source, &referenceDomainId);
    daqBool sourceSupported = False;
    daqSyncInterface_getSourceSupported(source, &sourceSupported);
    daqSyncMode mode = daqSyncModeOff;
    daqSyncInterface_getMode(source, &mode);

    daqConstCharPtr idStr = nullptr;
    daqString_getCharPtr(id, &idStr);
    daqConstCharPtr syncTypeStr = nullptr;
    daqString_getCharPtr(syncType, &syncTypeStr);
    daqConstCharPtr referenceDomainIdStr = nullptr;
    daqString_getCharPtr(referenceDomainId, &referenceDomainIdStr);

    ASSERT_STREQ(idStr, "ClockSyncInterface");
    ASSERT_STREQ(syncTypeStr, "local");
    ASSERT_STREQ(referenceDomainIdStr, "local:testDevice");
    ASSERT_TRUE(sourceSupported);
    ASSERT_EQ(mode, daqSyncModeInput);

    daqString* unknownSource = nullptr;
    daqString_createString(&unknownSource, "DoesNotExist");
    ASSERT_NE(daqSynchronization_setSource(synchronization, unknownSource), 0u);
    daqClearErrorInfo();

    daqBaseObject_releaseRef(unknownSource);
    daqBaseObject_releaseRef(referenceDomainId);
    daqBaseObject_releaseRef(syncType);
    daqBaseObject_releaseRef(id);
    daqBaseObject_releaseRef(source);
    daqBaseObject_releaseRef(referenceDomainIds);
    daqBaseObject_releaseRef(sources);
    daqBaseObject_releaseRef(interfaces);
    daqBaseObject_releaseRef(synchronization);
    daqBaseObject_releaseRef(deviceId);
    daqBaseObject_releaseRef(context);
    daqBaseObject_releaseRef(discoveryServers);
    daqBaseObject_releaseRef(options);
    daqBaseObject_releaseRef(typeManager);
    daqBaseObject_releaseRef(logger);
    daqBaseObject_releaseRef(sink);
    daqBaseObject_releaseRef(sinks);
}
