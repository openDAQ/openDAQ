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
#include <coretypes/intfs.h>
#include <coreobjects/mutex.h>
#include <fmt/format.h>
#include <coretypes/impl.h>
#include <atomic>
#include <mutex>
#include <thread>

BEGIN_NAMESPACE_OPENDAQ

class MutexImpl : public ImplementationOf<IMutex>
{
public:
    explicit MutexImpl();
    
    ErrCode INTERFACE_FUNC lock() override;
    ErrCode INTERFACE_FUNC tryLock(Bool* succeeded) override;
    ErrCode INTERFACE_FUNC unlock() override;
    ErrCode INTERFACE_FUNC lockRecursive() override;
    ErrCode INTERFACE_FUNC unlockRecursive() override;

    std::mutex mutex;

private:
    // Read outside the mutex to detect same-thread re-entry, so it is atomic; written only while the mutex is held
    std::atomic<std::thread::id> externalCallThreadId{};
    int externalCallDepth = 0;
};

inline MutexImpl::MutexImpl() = default;

inline ErrCode MutexImpl::lock()
{
    try
    {
        this->mutex.lock();
    }
    catch (std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to lock mutex: {}", e.what()));
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode MutexImpl::tryLock(Bool* succeeded)
{
    try
    {
        *succeeded = this->mutex.try_lock();
    }
    catch (std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to try_lock mutex: {}", e.what()));
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode MutexImpl::unlock()
{
    try
    {
        this->mutex.unlock();
    }
    catch (std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to unlock mutex: {}", e.what()));
    }

    return OPENDAQ_SUCCESS;
}

inline ErrCode MutexImpl::lockRecursive()
{
    // relaxed is sufficient because a thread only matches an id it wrote itself, and other threads' ids never compare equal
    if (externalCallThreadId.load(std::memory_order_relaxed) != std::this_thread::get_id())
    {
        const ErrCode errCode = lock();
        OPENDAQ_RETURN_IF_FAILED(errCode);
        externalCallThreadId.store(std::this_thread::get_id(), std::memory_order_relaxed);
    }

    ++externalCallDepth;
    return OPENDAQ_SUCCESS;
}

inline ErrCode MutexImpl::unlockRecursive()
{
    if (externalCallThreadId.load(std::memory_order_relaxed) != std::this_thread::get_id())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The calling thread does not hold the mutex through lockRecursive");

    if (--externalCallDepth > 0)
        return OPENDAQ_SUCCESS;

    externalCallThreadId.store(std::thread::id(), std::memory_order_relaxed);
    return unlock();
}

END_NAMESPACE_OPENDAQ
