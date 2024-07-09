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

#include <new>
#include <unordered_set>
#include <coretypes/common.h>

#if defined(__MINGW32__) | defined(_MSC_VER)
    #define WINHEAP
#endif

BEGIN_NAMESPACE_OPENDAQ

class HeapAllocation
{
public:
    HeapAllocation();
    HeapAllocation(const HeapAllocation& other);

    ~HeapAllocation();

    void* allocMem(size_t len);
    void freeMem(void* ptr);

    HeapAllocation& operator=(const HeapAllocation& other);
private:
#ifdef WINHEAP
    void* heapHandle;
#endif
};

// make a function for global variable to ensure proper creation order (Scott Meyers, Effective C++, 3rd edition, item 3)
HeapAllocation& getHeapAlloc();

template <class T>
class CustomAllocator
{
public:
    typedef T value_type;

    CustomAllocator()
    {
        // ensure proper construction order
        getHeapAlloc();
    }

    template <class U>
    constexpr CustomAllocator(const CustomAllocator<U>& /*alloc*/) noexcept
    {
    }

    T* allocate(std::size_t n)
    {
        if (n > static_cast<std::size_t>(-1) / sizeof(T))
            throw std::bad_alloc();
        if (auto p = static_cast<T*>(getHeapAlloc().allocMem(n * sizeof(T))))
            return p;
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        getHeapAlloc().freeMem(p);
    }
};

END_NAMESPACE_OPENDAQ
