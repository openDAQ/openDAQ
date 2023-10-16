#include <opendaq/tags_factory.h>
#include <gtest/gtest.h>

using namespace daq;

using TagsTest = testing::Test;


TEST_F(TagsTest, ContainsTagNullName)
{
    auto tags = Tags();
    ASSERT_THROW(tags.contains(nullptr), ArgumentNullException);
}

TEST_F(TagsTest, AddTag)
{
    auto tags = Tags();
    tags.add("test1");

    ASSERT_TRUE(tags.contains("test1"));
}

TEST_F(TagsTest, AddTagNullName)
{
    auto tags = Tags();
    ASSERT_THROW(tags.add(nullptr), ArgumentNullException);
}

TEST_F(TagsTest, DuplicateTag)
{
    auto tags = Tags();
    tags.add("test1");
    ASSERT_THROW(tags.add("test1"), DuplicateItemException);
}

TEST_F(TagsTest, TagNotFound)
{
    auto tags = Tags();

    ASSERT_FALSE(tags.contains("test1"));
}

TEST_F(TagsTest, RemoveTag)
{
    auto tags = Tags();
    tags.add("test1");
    tags.remove("test1");
    ASSERT_FALSE(tags.contains("test1"));
}

TEST_F(TagsTest, RemoveTagNotFound)
{
    auto tags = Tags();
    ASSERT_THROW(tags.remove("test1"), NotFoundException);
}

TEST_F(TagsTest, RemoveTagNullName)
{
    auto tags = Tags();
    ASSERT_THROW(tags.remove(nullptr), ArgumentNullException);
}

TEST_F(TagsTest, MultipleTags)
{
    auto tags = Tags();
    tags.add("test1");
    tags.add("test2");
    tags.add("test3");
    tags.remove("test2");
    ASSERT_TRUE(tags.contains("test1"));
    ASSERT_FALSE(tags.contains("test2"));
    ASSERT_TRUE(tags.contains("test3"));

}

TEST_F(TagsTest, Query)
{
    auto tags = Tags();
    tags.add("test1");
    tags.add("test2");
    tags.add("test3");

    ASSERT_TRUE(tags.query("test1"));
    ASSERT_TRUE(tags.query("test1 && test3"));
    ASSERT_FALSE(tags.query("test1 && test4"));
    ASSERT_TRUE(tags.query("test1 && !test4"));

    tags.add("test4");
    ASSERT_TRUE(tags.query("test1 && test4"));
    ASSERT_FALSE(tags.query("test1 && !test4"));
    ASSERT_TRUE(tags.query("test2 || test5"));

    ASSERT_FALSE(tags.query("test2 && (test3 && (test5 || test6))"));
}

TEST_F(TagsTest, Freeze)
{
    auto tags = Tags();
    tags.freeze();
    ASSERT_TRUE(tags.isFrozen());

    ASSERT_THROW(tags.add("test"), FrozenException);
    ASSERT_THROW(tags.remove("test"), FrozenException);
}

TEST_F(TagsTest, Equals)
{
    auto tags = Tags();
    tags.add("jovanka");
    tags.add("greta");
    tags.add("marica");

    auto tagsCopy = Tags();
    tagsCopy.add("jovanka");
    tagsCopy.add("greta");
    tagsCopy.add("marica");

    auto tagsDifferentOrder = Tags();
    tagsDifferentOrder.add("greta");
    tagsDifferentOrder.add("jovanka");
    tagsDifferentOrder.add("marica");

    ASSERT_TRUE(BaseObjectPtr::Equals(tags, tagsCopy));
    ASSERT_TRUE(BaseObjectPtr::Equals(tags, tagsDifferentOrder));

    auto tagsDifferentValues = Tags();
    tagsDifferentValues.add("jovanka");
    tagsDifferentValues.add("greta");
    tagsDifferentValues.add("andreja");

    ASSERT_FALSE(BaseObjectPtr::Equals(tags, tagsDifferentValues));

    auto tagsDifferentSize = Tags();
    tagsDifferentSize.add("jovanka");
    tagsDifferentSize.add("greta");

    ASSERT_FALSE(BaseObjectPtr::Equals(tags, tagsDifferentSize));
    ASSERT_FALSE(BaseObjectPtr::Equals(tags, nullptr));
}

TEST_F(TagsTest, Serialize)
{
    auto tags = Tags();
    tags.add("greta");
    tags.add("jovanka");
    tags.add("marica");

    const auto serializer = JsonSerializer(False);
    tags.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    const TagsPtr deserializedTags = deserializer.deserialize(serializer.getOutput());

    ASSERT_EQ(tags, deserializedTags);
}
