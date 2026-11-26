/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coretypes/impl.h>
#include <coreobjects/object_lock_guard.h>
#include <coreobjects/mutex_ptr.h>
#include <cassert>
#include <mutex>

BEGIN_NAMESPACE_OPENDAQ

// Holds the mutex locked without recursion until the guard is released
class LockGuardImpl : public ImplementationOf<ILockGuard>
{
public:
    explicit LockGuardImpl(MutexPtr lock)
        : mutex(std::move(lock))
        , lockGuard(mutex)
    {
    }

private:
    MutexPtr mutex;
    std::lock_guard<MutexPtr> lockGuard;
};

// Holds the mutex through lockRecursive, so its thread can take further recursive guards of any object that shares it
class RecursiveLockGuardImpl : public ImplementationOf<ILockGuard>
{
public:
    explicit RecursiveLockGuardImpl(MutexPtr lock)
        : mutex(std::move(lock))
    {
        checkErrorInfo(mutex->lockRecursive());
    }

    ~RecursiveLockGuardImpl() override
    {
        // Fails only for a guard released on a thread other than the one that took it; a destructor cannot report it
        const ErrCode errCode = mutex->unlockRecursive();
        assert(OPENDAQ_SUCCEEDED(errCode));
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
    }

private:
    MutexPtr mutex;
};

END_NAMESPACE_OPENDAQ
