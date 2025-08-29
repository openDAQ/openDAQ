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
#include <coretypes/coretypes.h>
#include "coreobjects/mutex.h"
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class MutexPtr;

template <>
struct InterfaceToSmartPtr<daq::IMutex>
{
    using SmartPtr = daq::MutexPtr;
};

class MutexPtr : public daq::ObjectPtr<IMutex>
{
public:
    using daq::ObjectPtr<IMutex>::ObjectPtr;


    MutexPtr()
        : daq::ObjectPtr<IMutex>()

    {
    }

    MutexPtr(daq::ObjectPtr<IMutex>&& ptr)
        : daq::ObjectPtr<IMutex>(std::move(ptr))

    {
    }

    MutexPtr(const daq::ObjectPtr<IMutex>& ptr)
        : daq::ObjectPtr<IMutex>(ptr)

    {
    }

    MutexPtr(const MutexPtr& other)
        : daq::ObjectPtr<IMutex>(other)

    {
    }

    MutexPtr(MutexPtr&& other) noexcept
        : daq::ObjectPtr<IMutex>(std::move(other))

    {
    }
    
    MutexPtr& operator=(const MutexPtr& other)
    {
        daq::ObjectPtr<IMutex>::operator =(other);


        return *this;
    }

    MutexPtr& operator=(MutexPtr&& other) noexcept
    {

        daq::ObjectPtr<IMutex>::operator =(std::move(other));

        return *this;
    }

    void lock() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        auto errCode = this->object->lock();
        daq::checkErrorInfo(errCode);
    }

    bool try_lock() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        Bool lock;
        auto errCode = this->object->tryLock(&lock);
        daq::checkErrorInfo(errCode);

        return lock;
    }

    void unlock() const
    {
        if (this->object == nullptr)
            DAQ_THROW_EXCEPTION(daq::InvalidParameterException);

        auto errCode = this->object->unlock();
        daq::checkErrorInfo(errCode);
    }
};

END_NAMESPACE_OPENDAQ
