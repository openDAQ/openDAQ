/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <coretypes/intfs.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_core.h>
#include <coreobjects/property_object_internal.h>
#include <coreobjects/property_object_utils.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TMutex>
class GenericRecursiveConfigLockGuard;
struct LockGuardImpl;

template <typename TMutex>
class RecursiveLockGuardImpl;

struct NullMutex
{
    void lock() {}
    void unlock() noexcept {}
    bool try_lock() { return true; }
};

class PropertyObjectCoreImpl : public ImplementationOfWeak<IPropertyObjectCore>
{
public:
    explicit PropertyObjectCoreImpl() = default;

    ErrCode INTERFACE_FUNC getRecursiveLockGuard(ILockGuard** lockGuard) override;
    ErrCode INTERFACE_FUNC getLockGuard(ILockGuard** lockGuard) override;
    ErrCode INTERFACE_FUNC setInternalVariable(PropObjectCoreVariableId varId, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setInternalVariables(IDict* varIdValueMap) override;

private:
    LockingStrategy lockingStrategy;
    WeakRefPtr<IPropertyObjectInternal> lockOwner;
    NullMutex nullSync;
    MutexPtr sync;
    std::thread::id externalCallThreadId{};
    int externalCallDepth = 0;
};

struct LockGuardImpl : ImplementationOf<ILockGuard>
{
    LockGuardImpl(IPropertyObjectCore* owner, MutexPtr lock);

private:
    // to ensure that owner is destroyed after lock
    ObjectPtr<IPropertyObjectCore> owner;
    MutexPtr mutex;
    std::lock_guard<MutexPtr> lock;
};

template <typename TMutex>
class RecursiveLockGuardImpl : public ImplementationOf<ILockGuard>
{
public:
    RecursiveLockGuardImpl(IPropertyObjectCore* owner, const TMutex& lock, std::thread::id* threadId, int* depth);
    ~RecursiveLockGuardImpl() override;

private:
    // to ensure that owner is destroyed after lock
    ObjectPtr<IPropertyObjectCore> owner;
    std::thread::id* id;
    int* depth;
    TMutex mutex;
    std::lock_guard<TMutex> lockGuard;
};

END_NAMESPACE_OPENDAQ
