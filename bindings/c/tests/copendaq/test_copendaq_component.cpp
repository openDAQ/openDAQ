#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqComponentTest : public testing::Test
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

TEST_F(COpendaqComponentTest, ComponentPrivate)
{
    String* id = nullptr;
    String_createString(&id, "parent");

    Component* component = nullptr;
    Component_createComponent(&component, ctx, nullptr, id, nullptr);

    String* name = nullptr;
    String_createString(&name, "Name");

    String* desc = nullptr;
    String_createString(&desc, "Description");

    Component_setName(component, name);
    Component_setDescription(component, desc);
    Component_setActive(component, True);
    Component_setVisible(component, True);

    ComponentPrivate* priv = nullptr;
    BaseObject_borrowInterface(component, COMPONENT_PRIVATE_INTF_ID, reinterpret_cast<void**>(&priv));

    ComponentPrivate_lockAllAttributes(priv);

    String* newName = nullptr;
    String_createString(&newName, "New Name");

    String* newDesc = nullptr;
    String_createString(&newDesc, "New Description");

    Component_setName(component, newName);
    Component_setDescription(component, newDesc);
    Component_setActive(component, False);
    Component_setVisible(component, False);

    String* outName = nullptr;
    Component_getName(component, &outName);
    String* outDesc = nullptr;
    Component_getDescription(component, &outDesc);
    Bool active = False;
    Component_getActive(component, &active);
    Bool visible = False;
    Component_getVisible(component, &visible);

    ConstCharPtr nameStr = nullptr;
    String_getCharPtr(outName, &nameStr);
    ConstCharPtr descStr = nullptr;
    String_getCharPtr(outDesc, &descStr);

    ASSERT_STREQ(nameStr, "Name");
    ASSERT_STREQ(descStr, "Description");
    ASSERT_TRUE(active);
    ASSERT_TRUE(visible);

    BaseObject_releaseRef(outName);
    BaseObject_releaseRef(outDesc);
    BaseObject_releaseRef(newDesc);
    BaseObject_releaseRef(newName);
    BaseObject_releaseRef(desc);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(component);
    BaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, ComponentStatusContainer)
{
    String* id = nullptr;
    String_createString(&id, "parent");

    Component* component = nullptr;
    Component_createComponent(&component, ctx, nullptr, id, nullptr);

    ComponentStatusContainer* status = nullptr;
    Component_getStatusContainer(component, &status);

    ASSERT_NE(status, nullptr);

    ComponentStatusContainerPrivate* priv = nullptr;
    BaseObject_borrowInterface(status, COMPONENT_STATUS_CONTAINER_PRIVATE_INTF_ID, reinterpret_cast<void**>(&priv));

    ASSERT_NE(priv, nullptr);

    BaseObject_releaseRef(status);
    BaseObject_releaseRef(component);
    BaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, Component)
{
    String* id = nullptr;
    String_createString(&id, "parent");
    String* idc = nullptr;
    String_createString(&idc, "child");

    Component* component = nullptr;
    Component_createComponent(&component, ctx, nullptr, id, nullptr);

    Component* child = nullptr;
    Component_createComponent(&child, ctx, component, idc, nullptr);

    String* childLocalId = nullptr;
    Component_getLocalId(child, &childLocalId);
    ConstCharPtr childLocalIdStr = nullptr;
    String_getCharPtr(childLocalId, &childLocalIdStr);

    String* childGlobalId = nullptr;
    Component_getGlobalId(child, &childGlobalId);
    ConstCharPtr childGlobalIdStr = nullptr;
    String_getCharPtr(childGlobalId, &childGlobalIdStr);

    ASSERT_STREQ(childLocalIdStr, "child");
    ASSERT_STREQ(childGlobalIdStr, "/parent/child");

    BaseObject_releaseRef(childGlobalId);
    BaseObject_releaseRef(childLocalId);
    BaseObject_releaseRef(child);
    BaseObject_releaseRef(component);
    BaseObject_releaseRef(idc);
    BaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, Folder)
{
    String* folderId = nullptr;
    String_createString(&folderId, "folder");

    String* itemId = nullptr;
    String_createString(&itemId, "item");

    FolderConfig* folder = nullptr;
    FolderConfig_createFolder(&folder, ctx, nullptr, folderId);

    Component* component = nullptr;
    Component_createComponent(&component, ctx, nullptr, itemId, nullptr);
    FolderConfig_addItem(folder, component);

    Folder* f = nullptr;
    BaseObject_borrowInterface(folder, FOLDER_INTF_ID, reinterpret_cast<void**>(&f));

    Bool empty = false;
    Folder_isEmpty(f, &empty);
    ASSERT_EQ(empty, false);

    FolderConfig_clear(folder);
    Folder_isEmpty(f, &empty);
    ASSERT_EQ(empty, true);

    BaseObject_releaseRef(component);
    BaseObject_releaseRef(folder);
    BaseObject_releaseRef(itemId);
    BaseObject_releaseRef(folderId);
}

TEST_F(COpendaqComponentTest, Removable)
{
    String* id = nullptr;
    String_createString(&id, "parent");

    Component* component = nullptr;
    Component_createComponent(&component, ctx, nullptr, id, nullptr);

    Removable* rm = nullptr;
    BaseObject_borrowInterface(component, REMOVABLE_INTF_ID, reinterpret_cast<void**>(&rm));

    Removable_remove(rm);
    Bool isRemoved = False;
    Removable_isRemoved(rm, &isRemoved);

    ASSERT_EQ(isRemoved, True);

    BaseObject_releaseRef(component);
    BaseObject_releaseRef(id);
}

TEST_F(COpendaqComponentTest, Tags)
{
    Tags* tags = nullptr;
    Tags_createTags(&tags);

    TagsPrivate* priv = nullptr;
    BaseObject_borrowInterface(tags, TAGS_PRIVATE_INTF_ID, reinterpret_cast<void**>(&priv));

    String* tag = nullptr;
    String_createString(&tag, "test");

    TagsPrivate_add(priv, tag);

    Bool result = false;
    Tags_contains(tags, tag, &result);

    ASSERT_TRUE(result);
    BaseObject_releaseRef(tag);
    BaseObject_releaseRef(tags);
}
