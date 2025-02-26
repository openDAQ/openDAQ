#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObject)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyObject,
    IPropertyObject, createPropertyObjectWithClassAndManager,
    ITypeManager*, manager,
    IString*, className
)

template class GenericRecursiveConfigLockGuard<std::mutex>;
template class GenericRecursiveConfigLockGuard<object_utils::NullMutex>;

template class RecursiveLockGuardImpl<std::mutex>;
template class RecursiveLockGuardImpl<object_utils::NullMutex>;


END_NAMESPACE_OPENDAQ
