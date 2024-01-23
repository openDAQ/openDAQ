#include <opendaq/tags_factory.h>
#include <opendaq/tags_private_ptr.h>
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
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add("test1");

    ASSERT_TRUE(tags.contains("test1"));
}

TEST_F(TagsTest, AddTagNullName)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    ASSERT_THROW(tagsPrivate.add(nullptr), ArgumentNullException);
}

TEST_F(TagsTest, DuplicateTag)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add(String("test1"));
    ErrCode err = tagsPrivate->add(String("test1"));
    ASSERT_EQ(err, OPENDAQ_IGNORED);
}

TEST_F(TagsTest, TagNotFound)
{
    auto tags = Tags();

    ASSERT_FALSE(tags.contains("test1"));
}

TEST_F(TagsTest, RemoveTag)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add("test1");
    tagsPrivate.remove("test1");
    ASSERT_FALSE(tags.contains("test1"));
}

TEST_F(TagsTest, RemoveTagNotFound)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    ErrCode err = tagsPrivate->remove(String("test1"));
    ASSERT_EQ(err, OPENDAQ_IGNORED);
}

TEST_F(TagsTest, RemoveTagNullName)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    ASSERT_THROW(tagsPrivate.remove(nullptr), ArgumentNullException);
}

TEST_F(TagsTest, MultipleTags)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add("test1");
    tagsPrivate.add("test2");
    tagsPrivate.add("test3");
    tagsPrivate.remove("test2");
    ASSERT_TRUE(tags.contains("test1"));
    ASSERT_FALSE(tags.contains("test2"));
    ASSERT_TRUE(tags.contains("test3"));

}

TEST_F(TagsTest, Query)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add("test1");
    tagsPrivate.add("test2");
    tagsPrivate.add("test3");

    ASSERT_TRUE(tags.query("test1"));
    ASSERT_TRUE(tags.query("test1 && test3"));
    ASSERT_FALSE(tags.query("test1 && test4"));
    ASSERT_TRUE(tags.query("test1 && !test4"));

    tagsPrivate.add("test4");
    ASSERT_TRUE(tags.query("test1 && test4"));
    ASSERT_FALSE(tags.query("test1 && !test4"));
    ASSERT_TRUE(tags.query("test2 || test5"));

    ASSERT_FALSE(tags.query("test2 && (test3 && (test5 || test6))"));
}

TEST_F(TagsTest, Equals)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add("jovanka");
    tagsPrivate.add("greta");
    tagsPrivate.add("marica");

    auto tagsCopy = Tags();
    auto tagsCopyPrivate = tagsCopy.asPtr<ITagsPrivate>();
    tagsCopyPrivate.add("jovanka");
    tagsCopyPrivate.add("greta");
    tagsCopyPrivate.add("marica");

    auto tagsDifferentOrder = Tags();
    auto tagsDifferentOrderPrivate = tagsDifferentOrder.asPtr<ITagsPrivate>();
    tagsDifferentOrderPrivate.add("greta");
    tagsDifferentOrderPrivate.add("jovanka");
    tagsDifferentOrderPrivate.add("marica");

    ASSERT_TRUE(BaseObjectPtr::Equals(tags, tagsCopy));
    ASSERT_TRUE(BaseObjectPtr::Equals(tags, tagsDifferentOrder));

    auto tagsDifferentValues = Tags();
    auto tagsDifferentValuesPrivate = tagsDifferentValues.asPtr<ITagsPrivate>();
    tagsDifferentValuesPrivate.add("jovanka");
    tagsDifferentValuesPrivate.add("greta");
    tagsDifferentValuesPrivate.add("andreja");

    ASSERT_FALSE(BaseObjectPtr::Equals(tags, tagsDifferentValues));

    auto tagsDifferentSize = Tags();
    auto tagsDifferentSizePrivate = tagsDifferentSize.asPtr<ITagsPrivate>();
    tagsDifferentSizePrivate.add("jovanka");
    tagsDifferentSizePrivate.add("greta");

    ASSERT_FALSE(BaseObjectPtr::Equals(tags, tagsDifferentSize));
    ASSERT_FALSE(BaseObjectPtr::Equals(tags, nullptr));
}

TEST_F(TagsTest, Serialize)
{
    auto tags = Tags();
    auto tagsPrivate = tags.asPtr<ITagsPrivate>();
    tagsPrivate.add("greta");
    tagsPrivate.add("jovanka");
    tagsPrivate.add("marica");

    const auto serializer = JsonSerializer(False);
    tags.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    const TagsPtr deserializedTags = deserializer.deserialize(serializer.getOutput());

    ASSERT_EQ(tags, deserializedTags);
}
