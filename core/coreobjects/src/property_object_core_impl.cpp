#include <coreobjects/property_object_core_impl.h>
#include <coretypes/validation.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

ErrCode PropertyObjectCoreImpl::getRecursiveLockGuard(ILockGuard** lockGuard, IMutex* sync)
{
    OPENDAQ_PARAM_NOT_NULL(lockGuard);

    // Prevent access violation when lock is obtained during destruction.
    auto objRef = this->refCount ? this->borrowPtr<ObjectPtr<IPropertyObjectCore>>() : nullptr;
    // read outside the mutex; relaxed is sufficient because a thread only matches an id it
    // wrote itself (sequenced on its own thread), and other threads' ids never compare equal
    const auto ownerThread = externalCallThreadId.load(std::memory_order_relaxed);
    if (ownerThread != std::thread::id() && ownerThread == std::this_thread::get_id())
        return createObject<ILockGuard, RecursiveLockGuardImpl<NullMutex>, IPropertyObjectCore*, NullMutex, std::atomic<std::thread::id>*, int*>
            (lockGuard, objRef, nullSync, &externalCallThreadId, &externalCallDepth);
    return createObject<ILockGuard, RecursiveLockGuardImpl<MutexPtr>, IPropertyObjectCore*, MutexPtr, std::atomic<std::thread::id>*, int*>
        (lockGuard, objRef, sync, &externalCallThreadId, &externalCallDepth);
}

ErrCode PropertyObjectCoreImpl::getLockGuard(ILockGuard** lockGuard, IMutex* sync)
{
    OPENDAQ_PARAM_NOT_NULL(lockGuard);

    // Prevent access violation when lock is obtained during destruction.
    auto objRef = this->refCount ? this->borrowPtr<ObjectPtr<IPropertyObjectCore>>() : nullptr;
    return createObject<ILockGuard, LockGuardImpl, IPropertyObjectCore*, MutexPtr>(lockGuard, objRef, sync);
}

// LockGuard
LockGuardImpl::LockGuardImpl(IPropertyObjectCore* owner, MutexPtr lock)
    : owner(owner)
    , mutex(std::move(lock))
    , lock(std::lock_guard(mutex))
{
}

template <typename TMutex>
RecursiveLockGuardImpl<TMutex>::RecursiveLockGuardImpl(IPropertyObjectCore* owner, const TMutex& lock, std::atomic<std::thread::id>* threadId, int* depth)
        : owner(owner)
        , id(threadId)
        , depth(depth)
        , mutex(lock)
        , lockGuard(std::lock_guard(mutex))
{
    assert(this->id != nullptr);
    assert(this->depth != nullptr);
    id->store(std::this_thread::get_id(), std::memory_order_relaxed);  // under the mutex
    ++(*this->depth);
}

template <typename TMutex>
RecursiveLockGuardImpl<TMutex>::~RecursiveLockGuardImpl()
{
    --(*depth);
    if (*depth == 0)
        id->store(std::thread::id(), std::memory_order_relaxed);  // under the mutex
}

template class RecursiveLockGuardImpl<MutexPtr>;
template class RecursiveLockGuardImpl<NullMutex>;

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObjectCore)

END_NAMESPACE_OPENDAQ
