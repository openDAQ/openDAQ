#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqServerTest = testing::Test;

TEST_F(COpendaqServerTest, ServerType)
{
    String* id = nullptr;
    String_createString(&id, "id");
    String* name = nullptr;
    String_createString(&name, "name");
    String* description = nullptr;
    String_createString(&description, "description");
    PropertyObject* defaultConfig = nullptr;
    PropertyObject_createPropertyObject(&defaultConfig);

    ServerType* obj = nullptr;
    ServerType_createServerType(&obj, id, name, description, defaultConfig);
    ASSERT_NE(obj, nullptr);

    BaseObject_releaseRef(obj);
    BaseObject_releaseRef(defaultConfig);
    BaseObject_releaseRef(description);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(id);
}
