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

#include <coretypes/object_pool.h>
#include <coretypes/common.h>
#include <coretypes/intfs.h>

namespace daq::object_pool
{

/*!
 * Debug object tracking normally spans construction to destruction, but a pooled object is
 * constructed once and then recycled indefinitely: it is never destroyed while the pool lives, so
 * it would stay tracked for the whole process. Every object the pool has to construct to meet peak
 * demand would then be reported as a leak by the first test that pushed the demand up, even though
 * nothing leaked. Tracking is therefore bound to the period the object is handed out, which is what
 * the leak checks mean by "alive": marked live when the pool resets it for a caller, and parked
 * again when its last reference goes. Objects sitting in the free list are not tracked.
 */
template <class Derived, class Impl>
class PooledObject : public Impl
{
public:
    Derived* next;

    PooledObject(ObjectPool<Derived>* pool)
        : Impl()
        , next(nullptr)
        , pool(pool)
    {
        // Born parked: the pool either pushes this straight onto the free list or hands it out
        // through get(), which marks it live.
        markParked();
    }

    int INTERFACE_FUNC releaseRef() override
    {
        const auto newRefCount = this->internalReleaseRef();
        assert(newRefCount >= 0);
        if (newRefCount == 0)
        {
            markParked();
            this->pool->addToFreeList(static_cast<Derived*>(this));
        }

        return newRefCount;
    }

protected:
    void markLive()
    {
#ifndef NDEBUG
        daqTrackObject(this->getThisAsBaseObject());
#endif
    }

    void markParked()
    {
#ifndef NDEBUG
        daqUntrackObject(this->getThisAsBaseObject());
#endif
    }

private:
    ObjectPool<Derived>* pool;
};

template <class T, class Derived, class Impl>
class OrdinalPooledObject : public PooledObject<Derived, Impl>
{
public:
    using PooledObject<Derived, Impl>::PooledObject;

    void reset(T value)
    {
        this->value = value;
        // Called by ObjectPool::get() as the object is handed to a caller.
        this->markLive();
    }
};

}
