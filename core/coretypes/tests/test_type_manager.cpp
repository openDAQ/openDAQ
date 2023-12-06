#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using TypeManagerTest = testing::Test;

TEST_F(TypeManagerTest, AddType)
{
    auto manager = TypeManager();
    manager.addType(SimpleType(ctInt));
    manager.addType(RatioStructType());
    manager.addType(StructType("foo", List<IString>("field"), List<IType>(SimpleType(ctString))));
}

TEST_F(TypeManagerTest, AddTypeAlreadyAdded)
{
    auto manager = TypeManager();
    manager.addType(SimpleType(ctInt));
    ASSERT_THROW(manager.addType(SimpleType(ctInt)), AlreadyExistsException);
}

TEST_F(TypeManagerTest, RemoveType)
{
    auto manager = TypeManager();
    manager.addType(SimpleType(ctInt));
    ASSERT_NO_THROW(manager.removeType("int"));
}

TEST_F(TypeManagerTest, RemoveTypeNotExisting)
{
    auto manager = TypeManager();
    ASSERT_THROW(manager.removeType("int"), NotFoundException);
}

TEST_F(TypeManagerTest, GetType)
{
    auto manager = TypeManager();
    auto type = SimpleType(ctInt);
    manager.addType(type);
    ASSERT_EQ(manager.getType("int"), type);
}

TEST_F(TypeManagerTest, GetTypeNotExisting)
{
    auto manager = TypeManager();
    ASSERT_THROW(manager.getType("int"), NotFoundException);
}

TEST_F(TypeManagerTest, HasType)
{
    auto manager = TypeManager();
    ASSERT_FALSE(manager.hasType("int"));
    manager.addType(SimpleType(ctInt));
    ASSERT_TRUE(manager.hasType("int"));
}

TEST_F(TypeManagerTest, GetTypes)
{
    auto manager = TypeManager();
    manager.addType(SimpleType(ctInt));
    manager.addType(SimpleType(ctString));
    ASSERT_EQ(manager.getTypes().getCount(), (SizeT) 2);
}

TEST_F(TypeManagerTest, Serialization)
{
    auto manager = TypeManager();
    manager.addType(SimpleType(ctInt));
    manager.addType(SimpleType(ctString));
    manager.addType(StructType("foo", List<IString>("string"), List<IBaseObject>("foo"), List<IType>(SimpleType(ctString))));
    manager.addType(RatioStructType());

    const auto serializer = JsonSerializer();
    
    manager.serialize(serializer);
    const auto serializedJson = serializer.getOutput();
    
    const auto deserializer = JsonDeserializer();
    const TypeManagerPtr typeManagerDeserialized = deserializer.deserialize(serializedJson);

    ASSERT_EQ(manager.getTypes(), typeManagerDeserialized.getTypes());
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("ITypeManager", "daq");

TEST_F(TypeManagerTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, ITypeManager::Id);
}

TEST_F(TypeManagerTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<ITypeManager>(), "{EBD840C6-7E32-51F4-B063-63D0B09F4240}");
}
