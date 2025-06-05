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
