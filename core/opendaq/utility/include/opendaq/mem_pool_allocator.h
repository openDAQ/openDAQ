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

#include <memory>

namespace daq::details
{

struct MemPool
{
    uint8_t* startPtr;
    uint8_t* curPtr;
    uint8_t* endPtr;
};

template <class T, size_t N>
struct StaticMemPool : MemPool
{
    StaticMemPool()
    {
        startPtr = buffer;
        curPtr = startPtr;
        endPtr = startPtr + N * sizeof(T);
    }

    alignas(T) uint8_t buffer[N * sizeof(T)];
};

template <class T>
struct MemPoolAllocator
{
    using value_type = T;

    MemPool& memPool;
    std::allocator<T> fallBackAllocator;

    MemPoolAllocator(MemPool& memPool)
        : memPool(memPool)
        , fallBackAllocator(std::allocator<T>())
    {
    }

    template <class U>
    MemPoolAllocator(const MemPoolAllocator<U>& other)
        : memPool(other.memPool)
        , fallBackAllocator(other.fallBackAllocator)
    {
    }

    [[nodiscard]] T* allocate(std::size_t n)
    {
        const ptrdiff_t sizeInBytes = sizeof(T) * n;
        const auto memAvail = memPool.endPtr - memPool.curPtr;
        if (sizeInBytes <= memAvail)
        {
            T* tmpPtr = reinterpret_cast<T*>(memPool.curPtr);
            memPool.curPtr += sizeInBytes;
            return tmpPtr;
        }

        return fallBackAllocator.allocate(n);
    }

    void deallocate(T* p, std::size_t n)
    {
        if (p >= reinterpret_cast<T*>(memPool.startPtr) && p < reinterpret_cast<T*>(memPool.endPtr))
            return;

#ifndef _MSC_VER
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
        fallBackAllocator.deallocate(p, n);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
    }

};

}
