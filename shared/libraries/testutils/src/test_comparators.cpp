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

    return true;
}

bool TestComparators::FunctionBlockTypeDictEquals(const DictPtr<IString, IFunctionBlockType>& a,
                                                  const DictPtr<IString, IFunctionBlockType>& b)
{
    if (a.getCount() != b.getCount())
        return false;

    const auto keysA = a.getKeyList();

    for (const auto& key : keysA)
    {
        if (!b.hasKey(key))
            return false;

        const auto valueA = a.get(key);
        const auto valueB = b.get(key);

        if (!BaseObjectPtr::Equals(valueA, valueB))
            return false;
    }

    return true;
}

END_NAMESPACE_OPENDAQ
