#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqComponentTest : public testing::Test
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

TEST_F(COpendaqComponentTest, ComponentPrivate)
{
    daqString* id = nullptr;
    daqString_createString(&id, "parent");

    daqComponent* component = nullptr;
    daqComponent_createComponent(&component, ctx, nullptr, id, nullptr);

    daqString* name = nullptr;
    daqString_createString(&name, "Name");

    daqString* desc = nullptr;
    daqString_createString(&desc, "Description");

    daqComponent_setName(component, name);
    daqComponent_setDescription(component, desc);
    daqComponent_setActive(component, True);
    daqComponent_setVisible(component, True);

    daqComponentPrivate* priv = nullptr;
    daqBaseObject_borrowInterface(component, DAQ_COMPONENT_PRIVATE_INTF_ID, (void**) &priv);

    daqComponentPrivate_lockAllAttributes(priv);

    daqString* newName = nullptr;
    daqString_createString(&newName, "New Name");

    daqString* newDesc = nullptr;
    daqString_createString(&newDesc, "New Description");

    daqComponent_setName(component, newName);
    daqComponent_setDescription(component, newDesc);
    daqComponent_setActive(component, False);
    daqComponent_setVisible(component, False);

    daqString* outName = nullptr;
    daqComponent_getName(component, &outName);
    daqString* outDesc = nullptr;
    daqComponent_getDescription(component, &outDesc);
    daqBool active = False;
    daqComponent_getActive(component, &active);
    daqBool visible = False;
    daqComponent_getVisible(component, &visible);

    daqConstCharPtr nameStr = nullptr;
    daqString_getCharPtr(outName, &nameStr);
    daqConstCharPtr descStr = nullptr;
    daqString_getCharPtr(outDesc, &descStr);

    ASSERT_STREQ(nameStr, "Name");
    ASSERT_STREQ(descStr, "Description");
    ASSERT_TRUE(active);
    ASSERT_TRUE(visible);

    daqBaseObject_releaseRef(outName);
    daqBaseObject_releaseRef(outDesc);
    daqBaseObject_releaseRef(newDesc);
    daqBaseObject_releaseRef(newName);
    daqBaseObject_releaseRef(desc);
    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(component);
    daqBaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, ComponentStatusContainer)
{
    daqString* id = nullptr;
    daqString_createString(&id, "parent");

    daqComponent* component = nullptr;
    daqComponent_createComponent(&component, ctx, nullptr, id, nullptr);

    daqComponentStatusContainer* status = nullptr;
    daqComponent_getStatusContainer(component, &status);

    ASSERT_NE(status, nullptr);

    daqComponentStatusContainerPrivate* priv = nullptr;
    daqBaseObject_borrowInterface(status, DAQ_COMPONENT_STATUS_CONTAINER_PRIVATE_INTF_ID, (void**) &priv);

    ASSERT_NE(priv, nullptr);

    daqBaseObject_releaseRef(status);
    daqBaseObject_releaseRef(component);
    daqBaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, Component)
{
    daqString* id = nullptr;
    daqString_createString(&id, "parent");
    daqString* idc = nullptr;
    daqString_createString(&idc, "child");

    daqComponent* component = nullptr;
    daqComponent_createComponent(&component, ctx, nullptr, id, nullptr);

    daqComponent* child = nullptr;
    daqComponent_createComponent(&child, ctx, component, idc, nullptr);

    daqString* childLocalId = nullptr;
    daqComponent_getLocalId(child, &childLocalId);
    daqConstCharPtr childLocalIdStr = nullptr;
    daqString_getCharPtr(childLocalId, &childLocalIdStr);

    daqString* childGlobalId = nullptr;
    daqComponent_getGlobalId(child, &childGlobalId);
    daqConstCharPtr childGlobalIdStr = nullptr;
    daqString_getCharPtr(childGlobalId, &childGlobalIdStr);

    ASSERT_STREQ(childLocalIdStr, "child");
    ASSERT_STREQ(childGlobalIdStr, "/parent/child");

    daqBaseObject_releaseRef(childGlobalId);
    daqBaseObject_releaseRef(childLocalId);
    daqBaseObject_releaseRef(child);
    daqBaseObject_releaseRef(component);
    daqBaseObject_releaseRef(idc);
    daqBaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, Folder)
{
    daqString* folderId = nullptr;
    daqString_createString(&folderId, "folder");

    daqString* itemId = nullptr;
    daqString_createString(&itemId, "item");

    daqFolderConfig* folder = nullptr;
    daqFolderConfig_createFolder(&folder, ctx, nullptr, folderId);

    daqComponent* component = nullptr;
    daqComponent_createComponent(&component, ctx, nullptr, itemId, nullptr);
    daqFolderConfig_addItem(folder, component);

    daqFolder* f = nullptr;
    daqBaseObject_borrowInterface(folder, DAQ_FOLDER_INTF_ID, (void**) &f);

    daqBool empty = false;
    daqFolder_isEmpty(f, &empty);
    ASSERT_EQ(empty, false);

    daqFolderConfig_clear(folder);
    daqFolder_isEmpty(f, &empty);
    ASSERT_EQ(empty, true);

    daqBaseObject_releaseRef(component);
    daqBaseObject_releaseRef(folder);
    daqBaseObject_releaseRef(itemId);
    daqBaseObject_releaseRef(folderId);
}

TEST_F(COpendaqComponentTest, Removable)
{
    daqString* id = nullptr;
    daqString_createString(&id, "parent");

    daqComponent* component = nullptr;
    daqComponent_createComponent(&component, ctx, nullptr, id, nullptr);

    daqRemovable* rm = nullptr;
    daqBaseObject_borrowInterface(component, DAQ_REMOVABLE_INTF_ID, (void**) &rm);

    daqRemovable_remove(rm);
    daqBool isRemoved = False;
    daqRemovable_isRemoved(rm, &isRemoved);

    ASSERT_EQ(isRemoved, True);

    daqBaseObject_releaseRef(component);
    daqBaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, Tags)
{
    daqTags* tags = nullptr;
    daqTags_createTags(&tags);

    daqTagsPrivate* priv = nullptr;
    daqBaseObject_borrowInterface(tags, DAQ_TAGS_PRIVATE_INTF_ID, (void**) &priv);

    daqString* tag = nullptr;
    daqString_createString(&tag, "test");

    daqTagsPrivate_add(priv, tag);

    daqBool result = false;
    daqTags_contains(tags, tag, &result);

    ASSERT_TRUE(result);
    daqBaseObject_releaseRef(tag);
    daqBaseObject_releaseRef(tags);
}
