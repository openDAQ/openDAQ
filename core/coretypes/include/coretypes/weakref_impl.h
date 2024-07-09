/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/weakref.h>
#include <coretypes/intfs.h>
#include <atomic>

BEGIN_NAMESPACE_OPENDAQ

class WeakRefImpl final : public ImplementationOf<IWeakRef>
{
public:
    WeakRefImpl(IBaseObject* object, RefCount* refCount);
    ~WeakRefImpl() override;

    ErrCode INTERFACE_FUNC getRef(IBaseObject** ref) override;
    ErrCode INTERFACE_FUNC getRefAs(IntfID intfID, void** obj) override;

private:
    RefCount* refCount;
    IBaseObject* object;
};

inline WeakRefImpl::WeakRefImpl(IBaseObject* object, RefCount* refCount)
    : refCount(refCount)
    , object(object)
{
}

inline WeakRefImpl::~WeakRefImpl()
{
    const auto newWeakRefCount = std::atomic_fetch_sub_explicit(&refCount->weak, 1, std::memory_order_acq_rel) - 1;
    assert(newWeakRefCount >= 0);
    if (newWeakRefCount == 0)
        delete refCount;
}

inline ErrCode WeakRefImpl::getRef(IBaseObject** ref)
{
    for (;;)
    {
        auto curRefCount = refCount->strong.load(std::memory_order_acquire);
        if (curRefCount == 0)
            return OPENDAQ_ERR_NOTASSIGNED;

        if (refCount->strong.compare_exchange_weak(curRefCount, curRefCount + 1, std::memory_order_acq_rel))
        {
            *ref = this->object;
            return OPENDAQ_SUCCESS;
        }
    }
}

inline ErrCode WeakRefImpl::getRefAs(IntfID intfID, void** obj)
{
    for (;;)
    {
        auto curRefCount = refCount->strong.load(std::memory_order_acquire);
        if (curRefCount == 0)
            return OPENDAQ_ERR_NOTASSIGNED;

        if (refCount->strong.compare_exchange_weak(curRefCount, curRefCount + 1, std::memory_order_acq_rel))
        {
            auto err = this->object->borrowInterface(intfID, obj);
            if (OPENDAQ_FAILED(err))
            {
                this->object->releaseRef();
                return err;
            }

            return OPENDAQ_SUCCESS;
        }
    }
}

END_NAMESPACE_OPENDAQ
