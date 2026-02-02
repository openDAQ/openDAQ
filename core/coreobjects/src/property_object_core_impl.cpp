#include <coreobjects/property_object_core_impl.h>
#include <coretypes/validation.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

ErrCode PropertyObjectCoreImpl::getRecursiveLockGuard(ILockGuard** lockGuard, LockingStrategy lockingStrategy)
{
    OPENDAQ_PARAM_NOT_NULL(lockGuard);

    auto lockOwnerPtr = lockOwner.assigned() ? lockOwner.getRef() : nullptr;
    if (lockOwnerPtr.assigned() && lockingStrategy == LockingStrategy::InheritLock)
        return lockOwnerPtr->getRecursiveLockGuard(lockGuard);
    
    // Prevent access violation when lock is obtained during destruction.
    auto objRef = this->refCount ? this->borrowPtr<ObjectPtr<IPropertyObjectCore>>() : nullptr;
    if (externalCallThreadId != std::thread::id() && externalCallThreadId == std::this_thread::get_id())
        return createObject<ILockGuard, RecursiveLockGuardImpl<NullMutex>, IPropertyObjectCore*, NullMutex, std::thread::id*, int*>
            (lockGuard, objRef, nullSync, &externalCallThreadId, &externalCallDepth);
    return createObject<ILockGuard, RecursiveLockGuardImpl<MutexPtr>, IPropertyObjectCore*, MutexPtr, std::thread::id*, int*>
        (lockGuard, objRef, sync, &externalCallThreadId, &externalCallDepth);
}

ErrCode PropertyObjectCoreImpl::getLockGuard(ILockGuard** lockGuard)
{
    OPENDAQ_PARAM_NOT_NULL(lockGuard);

    // Prevent access violation when lock is obtained during destruction.
    auto objRef = this->refCount ? this->borrowPtr<ObjectPtr<IPropertyObjectCore>>() : nullptr;
    return createObject<ILockGuard, LockGuardImpl, IPropertyObjectCore*, MutexPtr>(lockGuard, objRef, sync);
}

ErrCode PropertyObjectCoreImpl::setInternalVariable(PropObjectCoreVariableId varId, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);
    switch (varId)
    {
        case PropObjectCoreVariableId::LockOwner:
            this->lockOwner = value;
            break;
        case PropObjectCoreVariableId::Mutex:
            this->sync = value;
            break;
        case PropObjectCoreVariableId::LockingStrategy:
            break;
        default:;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectCoreImpl::setInternalVariables(IDict* varIdValueMap)
{
    OPENDAQ_PARAM_NOT_NULL(varIdValueMap);

    DictPtr<Int, IBaseObject> varsMapPtr = varIdValueMap;
    for (const auto& [id, val] : varsMapPtr)
        OPENDAQ_RETURN_IF_FAILED(setInternalVariable(id, val));

    return OPENDAQ_SUCCESS;
}


// LockGuard
LockGuardImpl::LockGuardImpl(IPropertyObjectCore* owner, MutexPtr lock)
    : owner(owner)
    , mutex(std::move(lock))
    , lock(std::lock_guard(mutex))
{
}

template <typename TMutex>
RecursiveLockGuardImpl<TMutex>::RecursiveLockGuardImpl(IPropertyObjectCore* owner, const TMutex& lock, std::thread::id* threadId, int* depth)
        : owner(owner) 
        , id(threadId)
        , depth(depth)
        , mutex(lock)
        , lockGuard(std::lock_guard(mutex))
{
    assert(this->id != nullptr);
    assert(this->depth != nullptr);
    *id = std::this_thread::get_id();
    ++(*this->depth);
}

template <typename TMutex>
RecursiveLockGuardImpl<TMutex>::~RecursiveLockGuardImpl()
{
    --(*depth);
    if (*depth == 0)
        *id = std::thread::id();
}

template class RecursiveLockGuardImpl<MutexPtr>;
template class RecursiveLockGuardImpl<NullMutex>;

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObjectCore)

END_NAMESPACE_OPENDAQ
