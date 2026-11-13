#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqServerTest = testing::Test;

TEST_F(COpendaqServerTest, ServerType)
{
    daqString* id = nullptr;
    daqString_createString(&id, "id");
    daqString* name = nullptr;
    daqString_createString(&name, "name");
    daqString* description = nullptr;
    daqString_createString(&description, "description");
    daqPropertyObject* defaultConfig = nullptr;
    daqPropertyObject_createPropertyObject(&defaultConfig);

    daqServerType* obj = nullptr;
    daqServerType_createServerType(&obj, id, name, description, defaultConfig);
    ASSERT_NE(obj, nullptr);

    daqBaseObject_releaseRef(obj);
    daqBaseObject_releaseRef(defaultConfig);
    daqBaseObject_releaseRef(description);
    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(id);
}

TEST_F(COpendaqServerTest, ServerKind)
{
    daqString* id = nullptr;
    daqString_createString(&id, "id");

    daqComponentTypeBuilder* builder = nullptr;
    daqComponentTypeBuilder_createServerTypeBuilder(&builder);
    daqComponentTypeBuilder_setId(builder, id);
    daqComponentTypeBuilder_setServerKind(builder, daqServerKindMirror);

    daqServerKind builderKind = daqServerKindUnspecified;
    daqComponentTypeBuilder_getServerKind(builder, &builderKind);
    ASSERT_EQ(builderKind, daqServerKindMirror);

    daqComponentType* componentType = nullptr;
    daqComponentTypeBuilder_build(builder, &componentType);
    daqServerType* serverType = nullptr;
    daqBaseObject_queryInterface(componentType, DAQ_SERVER_TYPE_INTF_ID, (daqBaseObject**) &serverType);
    ASSERT_NE(serverType, nullptr);

    daqServerKind kind = daqServerKindUnspecified;
    daqServerType_getServerKind(serverType, &kind);
    ASSERT_EQ(kind, daqServerKindMirror);

    daqBaseObject_releaseRef(serverType);
    daqBaseObject_releaseRef(componentType);
    daqBaseObject_releaseRef(builder);
    daqBaseObject_releaseRef(id);
}
