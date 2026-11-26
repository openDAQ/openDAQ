#include <coreobjects/property_object_core_impl.h>
#include <coretypes/validation.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

ErrCode PropertyObjectCoreImpl::getRecursiveLockGuard(ILockGuard** lockGuard, IMutex* sync)
{
    OPENDAQ_PARAM_NOT_NULL(lockGuard);

    // Prevent access violation when lock is obtained during destruction.
    auto objRef = this->refCount ? this->borrowPtr<ObjectPtr<IPropertyObjectCore>>() : nullptr;
    return createObject<ILockGuard, RecursiveLockGuardImpl, IPropertyObjectCore*, MutexPtr>(lockGuard, objRef, sync);
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

RecursiveLockGuardImpl::RecursiveLockGuardImpl(IPropertyObjectCore* owner, MutexPtr lock)
        : owner(owner)
        , mutex(std::move(lock))
{
    checkErrorInfo(mutex->lockRecursive());
}

RecursiveLockGuardImpl::~RecursiveLockGuardImpl()
{
    // Fails only for a guard released on a thread other than the one that took it; a destructor cannot report it
    const ErrCode errCode = mutex->unlockRecursive();
    assert(OPENDAQ_SUCCEEDED(errCode));
    if (OPENDAQ_FAILED(errCode))
        daqClearErrorInfo();
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObjectCore)

END_NAMESPACE_OPENDAQ
