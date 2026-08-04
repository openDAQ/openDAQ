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
 * @brief An object owned by an ObjectPool and recycled rather than destroyed.
 *
 * Reaching reference count 0 returns the object to the pool's free list instead of deleting it, so a
 * single instance serves many callers in turn. The pool destroys them when it is cleaned up.
 *
 * A derived type must call markLive() from its reset(), which is what ObjectPool::get() invokes as it
 * hands the object over.
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
        // Not handed out yet.
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
    /// Counts the object as in use for debug object tracking. Call when handing it to a caller.
    void markLive()
    {
#ifndef NDEBUG
        daqTrackObject(this->getThisAsBaseObject());
#endif
    }

    /// Stops counting the object as in use, for while it sits in the pool's free list.
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

        // get() calls reset() as it hands the object over, so this is where it becomes live.
        this->markLive();
    }
};

}
