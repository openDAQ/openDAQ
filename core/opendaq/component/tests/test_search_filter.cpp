#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/tags_private_ptr.h>
#include <gtest/gtest.h>

using namespace daq;

using SearchFilterTest = testing::Test;

TEST_F(SearchFilterTest, RequireTagsComponentWithoutTags)
{
    const auto component = Component(NullContext(), nullptr, "Temp");

    auto filter = search::RequireTags(List<IString>());
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("foo"));
    ASSERT_FALSE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("foo", "bar"));
    ASSERT_FALSE(filter.acceptsObject(component));
}

TEST_F(SearchFilterTest, RequireTagsComponentWithOneTag)
{
    const auto component = Component(NullContext(), nullptr, "Temp");
    component.getTags().asPtr<ITagsPrivate>(true).add("Tag");

    auto filter = search::RequireTags(List<IString>());
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("Tag"));
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("foo"));
    ASSERT_FALSE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("Tag", "foo"));
    ASSERT_FALSE(filter.acceptsObject(component));
}

TEST_F(SearchFilterTest, RequireTagsComponentWithTwoTags)
{
    const auto component = Component(NullContext(), nullptr, "Temp");
    component.getTags().asPtr<ITagsPrivate>(true).add("tag1");
    component.getTags().asPtr<ITagsPrivate>(true).add("tag2");

    auto filter = search::RequireTags(List<IString>());
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("tag1"));
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("tag2"));
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("tag1", "tag2"));
    ASSERT_TRUE(filter.acceptsObject(component));

    filter = search::RequireTags(List<IString>("tag1", "foo"));
    ASSERT_FALSE(filter.acceptsObject(component));
}
