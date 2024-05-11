#include <testutils/test_comparators.h>

BEGIN_NAMESPACE_OPENDAQ

bool TestComparators::PropertyObjectEquals(const PropertyObjectPtr& a, const PropertyObjectPtr& b)
{
    auto propertiesA = a.getAllProperties();
    auto propertiesB = b.getAllProperties();

    if (propertiesA.getCount() != propertiesB.getCount())
        return false;

    for (size_t i = 0; i < propertiesA.getCount(); i++)
    {
        const auto keyA = propertiesA.getItemAt(i).getName();
        const auto keyB = propertiesB.getItemAt(i).getName();

        if (!BaseObjectPtr::Equals(keyA, keyB))
            return false;

        const auto valueA = a.getPropertyValue(keyA);
        const auto valueB = b.getPropertyValue(keyA);

        if (!BaseObjectPtr::Equals(valueA, valueB))
            return false;
    }

    return true;
}

bool TestComparators::FunctionBlockTypeEquals(const FunctionBlockTypePtr& a, const FunctionBlockTypePtr& b)
{
    if (a.getId() != b.getId())
        return false;
    if (a.getName() != b.getName())
        return false;
    if (a.getDescription() != b.getDescription())
        return false;

    const auto configA = a.createDefaultConfig();
    const auto configB = b.createDefaultConfig();

    if (!configA.assigned() && !configB.assigned())
        return true;
    else if (configA.assigned() && configB.assigned())
        return PropertyObjectEquals(configA, configB);
    else
        return false;
}

END_NAMESPACE_OPENDAQ
