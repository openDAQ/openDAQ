#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

// PropertyUpdateStack::PropertyUpdateStackItem implementation

PropertyUpdateStack::PropertyUpdateStackItem::PropertyUpdateStackItem(const BaseObjectPtr& value)
    : value(value)
    , stackLevel(1)
    , isPushed(true)
{
}

bool PropertyUpdateStack::PropertyUpdateStackItem::setValue(const BaseObjectPtr& value)
{
    if (this->value == value)
        return false;

    this->value = value;
    this->isPushed = true;
    this->stackLevel++;
    return true;
}

bool PropertyUpdateStack::PropertyUpdateStackItem::unregister()
{
    bool result = this->isPushed;
    this->isPushed = false;
    this->stackLevel--;
    return result;
}

size_t PropertyUpdateStack::PropertyUpdateStackItem::getStackLevel() const
{
    return this->stackLevel;
}

// PropertyUpdateStack implementation

bool PropertyUpdateStack::registerPropertyUpdating(const std::string& name, const BaseObjectPtr& value)
{
    auto [it, inserted] = updatePropertyStack.try_emplace(name, value);

    if (inserted)
        return true;

    auto& propertItem = it->second;
    return propertItem.setValue(value);
}

bool PropertyUpdateStack::unregisterPropertyUpdating(const std::string& name)
{
    auto it = updatePropertyStack.find(name);
    if (it == updatePropertyStack.end())
        return false;

    auto& propertItem = it->second;
    bool result = propertItem.unregister();
    if (propertItem.getStackLevel() == 0)
        updatePropertyStack.erase(it);
    return result;
}

bool PropertyUpdateStack::isBaseStackLevel(const std::string& name) const
{
    auto it = updatePropertyStack.find(name);
    if (it == updatePropertyStack.end())
        return false;

    return it->second.getStackLevel() == 1;
}

bool PropertyUpdateStack::getPropertyValue(const std::string& name, BaseObjectPtr& value) const
{
    auto it = updatePropertyStack.find(name);
    if (it == updatePropertyStack.end())
        return false;

    // if value is not assigned, it means, that property is on clearing stage (clearPropertyValue)
    value = it->second.value;
    return true;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObject)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyObject,
    IPropertyObject, createPropertyObjectWithClassAndManager,
    ITypeManager*, manager,
    IString*, className
)

template class GenericRecursiveConfigLockGuard<MutexPtr>;
template class GenericRecursiveConfigLockGuard<object_utils::NullMutex>;

template class RecursiveLockGuardImpl<MutexPtr>;
template class RecursiveLockGuardImpl<object_utils::NullMutex>;

END_NAMESPACE_OPENDAQ
