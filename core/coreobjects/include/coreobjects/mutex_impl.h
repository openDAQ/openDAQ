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
#include <coreobjects/mutex.h>
#include <fmt/format.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

class MutexImpl : public ImplementationOf<IMutex>
{
public:
    explicit MutexImpl();
    
    ErrCode INTERFACE_FUNC lock() override;
    ErrCode INTERFACE_FUNC tryLock(Bool* succeeded) override;
    ErrCode INTERFACE_FUNC unlock() override;

    std::mutex mutex;
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

END_NAMESPACE_OPENDAQ
