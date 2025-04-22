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

#include <coretypes/object_pool.h>
#include <coretypes/common.h>

namespace daq::object_pool
{

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
    }

    int INTERFACE_FUNC releaseRef() override
    {
        const auto newRefCount = this->internalReleaseRef();
        assert(newRefCount >= 0);
        if (newRefCount == 0)
        {
            this->pool->addToFreeList(static_cast<Derived*>(this));
        }

        return newRefCount;
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
    }
};

}
