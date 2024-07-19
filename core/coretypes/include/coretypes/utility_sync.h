/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

#ifndef OPENDAQ_THREAD_SAFE

class NoLockMutex
{
public:
    void lock() {};
    void unlock() {};

    NoLockMutex(const NoLockMutex&) = delete;
    NoLockMutex& operator=(const NoLockMutex&) = delete;
};

typedef NoLockMutex mutex;

#else

#include <mutex>

typedef std::mutex mutex;

#endif

class RecursiveMutex
{
public:
    RecursiveMutex(mutex* mt)
        : mt(mt)
    {
    }

    RecursiveMutex()
        : mt(nullptr)
    {
    }

    void lock() const
    {
        if (mt != nullptr)
            mt->lock();
    };

    void unlock() const
    {
        if (mt != nullptr)
            mt->unlock();
    };

private:
    mutex* mt;
};

END_NAMESPACE_OPENDAQ
